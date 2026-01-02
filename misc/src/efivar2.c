#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <endian.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

// UEFI Variable Store structures
#define EFI_VARIABLE_STORE_SIGNATURE 0x4853565F  // "_VSH"
#define EFI_VARIABLE_STORE_FORMATTED 0x5F
#define EFI_VARIABLE_STORE_HEALTHY 0xFE

#define EFI_VARIABLE_HEADER_VALID 0x3F
#define EFI_VARIABLE_HEADER_INVALID 0x3C
#define EFI_VARIABLE_HEADER_DELETED 0x3A

#define EFI_BOOT_MANAGER_GUID \
    {0x8BE4DF61, 0x93CA, 0x11D2, {0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C}}

#define EFI_GLOBAL_VARIABLE_GUID \
    {0x8BE4DF61, 0x93CA, 0x11D2, {0xAA, 0x0D, 0xE0, 0x98, 0x03, 0x2B, 0x8C}}

// EFI GUID structure
typedef struct {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} efi_guid_t;

// Variable header structure
typedef struct {
    uint16_t start_id;      // 0x55AA
    uint8_t state;          // 0x3F = valid, 0x3C = invalid, 0x3A = deleted
    uint8_t reserved;
    uint32_t attributes;
    uint32_t name_size;
    uint32_t data_size;
    efi_guid_t vendor_guid;
    // Followed by: name (Unicode), data
} __attribute__((packed)) efi_variable_header_t;

// EFI Load Option structure
typedef struct {
    uint32_t attributes;
    uint16_t file_path_list_length;
    // Followed by: description (Unicode string), file_path_list, optional_data
} __attribute__((packed)) efi_load_option_t;

// Device Path Protocol header
typedef struct {
    uint8_t type;
    uint8_t sub_type;
    uint16_t length;
} __attribute__((packed)) efi_device_path_t;

// Boot entry structure
typedef struct {
    uint16_t boot_num;
    uint32_t attributes;
    char *description;
    char *device_path;
    int is_active;
} boot_entry_t;

// Global variables
static boot_entry_t *boot_entries = NULL;
static int boot_entry_count = 0;
static uint16_t boot_current = 0;
static uint16_t *boot_order = NULL;
static int boot_order_count = 0;

// BootOrder location for modification
static uint8_t *boot_order_data_ptr = NULL;
static uint32_t *boot_order_size_ptr = NULL;
static uint32_t boot_order_original_size = 0;
static int boot_order_format = 0; // 1 = Format 1, 2 = Format 2

// GUID comparison
static int guid_equal(const efi_guid_t *a, const efi_guid_t *b) {
    return memcmp(a, b, sizeof(efi_guid_t)) == 0;
}

// Read little-endian uint16
static uint16_t read_le16(const uint8_t *p) {
    return le16toh(*(uint16_t *)p);
}

// Read little-endian uint32
static uint32_t read_le32(const uint8_t *p) {
    return le32toh(*(uint32_t *)p);
}

// Read 64-bit little-endian
static uint64_t read_le64(const uint8_t *p) {
    return le64toh(*(uint64_t *)p);
}

// Convert Unicode to ASCII (simplified)
static void unicode_to_ascii(char *dest, const uint16_t *src, size_t len) {
    size_t i;
    for (i = 0; i < len / 2 && src[i] != 0; i++) {
        dest[i] = (char)(src[i] & 0xFF);
    }
    dest[i] = '\0';
}

// Parse device path to string
static char *parse_device_path(const uint8_t *data, uint16_t length) {
    char *result = malloc(4096);
    char *p = result;
    const uint8_t *end = data + length;
    int first = 1;

    while (data < end) {
        efi_device_path_t *dp = (efi_device_path_t *)data;
        if (dp->length == 0) break;
        if (dp->length < 4) break;

        if (!first) {
            *p++ = '/';
        }
        first = 0;

        uint8_t type = dp->type;
        uint8_t sub_type = dp->sub_type;
        const uint8_t *dp_data = data + 4;
        uint16_t dp_len = read_le16((uint8_t *)&dp->length) - 4;

        switch (type) {
            case 0x01: // Hardware Device Path
                switch (sub_type) {
                    case 0x01: // PCI
                        if (dp_len >= 4) {
                            uint8_t function = dp_data[0] & 0x07;
                            uint8_t device = (dp_data[0] >> 3) & 0x1F;
                            p += sprintf(p, "PciRoot(0x%x)/Pci(0x%x,0x%x)", 0, device, function);
                        }
                        break;
                }
                break;

            case 0x02: // ACPI Device Path
                break;

            case 0x03: // Messaging Device Path
                switch (sub_type) {
                    case 0x01: // ATAPI
                        break;
                    case 0x05: // USB
                        break;
                    case 0x06: // USB Class
                        break;
                    case 0x09: // MAC Address
                        if (dp_len >= 7) {
                            uint8_t mac[6];
                            memcpy(mac, dp_data, 6);
                            p += sprintf(p, "MAC(%02x%02x%02x%02x%02x%02x,0x%x)",
                                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                                dp_data[6]);
                        }
                        break;
                    case 0x0A: // IPv4
                        if (dp_len >= 24) {
                            uint32_t local_ip = read_le32(dp_data);
                            uint16_t local_port = read_le16(dp_data + 8);
                            uint8_t static_addr = dp_data[13];
                            uint32_t gateway_ip = read_le32(dp_data + 14);
                            uint32_t subnet_mask = read_le32(dp_data + 18);
                            uint32_t server_ip = read_le32(dp_data + 22);

                            p += sprintf(p, "IPv4(%d.%d.%d.%d,0x%x,%s,%d.%d.%d.%d,%d.%d.%d.%d,%d.%d.%d.%d)",
                                (local_ip >> 0) & 0xFF, (local_ip >> 8) & 0xFF,
                                (local_ip >> 16) & 0xFF, (local_ip >> 24) & 0xFF,
                                local_port,
                                static_addr ? "Static" : "DHCP",
                                (gateway_ip >> 0) & 0xFF, (gateway_ip >> 8) & 0xFF,
                                (gateway_ip >> 16) & 0xFF, (gateway_ip >> 24) & 0xFF,
                                (subnet_mask >> 0) & 0xFF, (subnet_mask >> 8) & 0xFF,
                                (subnet_mask >> 16) & 0xFF, (subnet_mask >> 24) & 0xFF,
                                (server_ip >> 0) & 0xFF, (server_ip >> 8) & 0xFF,
                                (server_ip >> 16) & 0xFF, (server_ip >> 24) & 0xFF);
                        }
                        break;
                    case 0x0B: // IPv6
                        if (dp_len >= 56) {
                            p += sprintf(p, "IPv6(");
                            for (int i = 0; i < 16; i += 2) {
                                if (i > 0) p += sprintf(p, ":");
                                uint16_t val = read_le16(dp_data + i);
                                p += sprintf(p, "%04x", val);
                            }
                            uint16_t local_port = read_le16(dp_data + 16);
                            uint8_t static_addr = dp_data[18];
                            p += sprintf(p, ",0x%x,%s,", local_port, static_addr ? "Static" : "DHCP");
                            // Gateway IP
                            for (int i = 0; i < 16; i += 2) {
                                if (i > 0) p += sprintf(p, ":");
                                uint16_t val = read_le16(dp_data + 20 + i);
                                p += sprintf(p, "%04x", val);
                            }
                            uint8_t prefix_length = dp_data[36];
                            p += sprintf(p, ",0x%x,", prefix_length);
                            // Server IP
                            for (int i = 0; i < 16; i += 2) {
                                if (i > 0) p += sprintf(p, ":");
                                uint16_t val = read_le16(dp_data + 40 + i);
                                p += sprintf(p, "%04x", val);
                            }
                            p += sprintf(p, ")");
                        }
                        break;
                    case 0x0C: // UART
                        break;
                    case 0x0F: // USB WWID
                        break;
                    case 0x10: // Device Logical Unit
                        break;
                    case 0x11: // SATA
                        break;
                    case 0x12: // iSCSI
                        break;
                    case 0x13: // VLAN
                        break;
                    case 0x14: // Fibre Channel Ex
                        break;
                    case 0x15: // SAS Ex
                        break;
                    case 0x16: // NVMe
                        break;
                    case 0x17: // URI
                        break;
                    case 0x18: // UFS
                        break;
                    case 0x19: // SD
                        break;
                    case 0x1A: // Bluetooth
                        break;
                    case 0x1B: // Wi-Fi
                        break;
                    case 0x1C: // EMMC
                        break;
                    case 0x1D: // Bluetooth LE
                        break;
                    case 0x1E: // DNS
                        break;
                }
                break;

            case 0x04: // Media Device Path
                switch (sub_type) {
                    case 0x01: // Hard Drive
                        if (dp_len >= 42) {
                            uint32_t partition_number = read_le32(dp_data);
                            uint64_t partition_start = read_le64(dp_data + 4);
                            uint64_t partition_size = read_le64(dp_data + 12);
                            uint8_t signature_type = dp_data[20];
                            uint8_t *signature = (uint8_t *)(dp_data + 21);

                            if (signature_type == 0x02) { // GPT
                                efi_guid_t *guid = (efi_guid_t *)signature;
                                p += sprintf(p, "HD(%u,GPT,", partition_number);
                                p += sprintf(p, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                    le32toh(guid->data1), le16toh(guid->data2), le16toh(guid->data3),
                                    guid->data4[0], guid->data4[1], guid->data4[2], guid->data4[3],
                                    guid->data4[4], guid->data4[5], guid->data4[6], guid->data4[7]);
                                p += sprintf(p, ",0x%llx,0x%llx)", 
                                    (unsigned long long)partition_start,
                                    (unsigned long long)partition_size);
                            } else {
                                p += sprintf(p, "HD(%u,", partition_number);
                            }
                        }
                        break;
                    case 0x04: // File Path
                        {
                            // File path is Unicode string
                            char file_path[512];
                            unicode_to_ascii(file_path, (uint16_t *)dp_data, dp_len);
                            p += sprintf(p, "File(%s)", file_path);
                        }
                        break;
                    case 0x05: // FV (Firmware Volume)
                        if (dp_len >= 16) {
                            efi_guid_t *guid = (efi_guid_t *)dp_data;
                            p += sprintf(p, "Fv(%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)",
                                le32toh(guid->data1), le16toh(guid->data2), le16toh(guid->data3),
                                guid->data4[0], guid->data4[1], guid->data4[2], guid->data4[3],
                                guid->data4[4], guid->data4[5], guid->data4[6], guid->data4[7]);
                        }
                        break;
                }
                break;

            case 0x05: // BBS Device Path
                break;

            case 0x7F: // End of Hardware Device Path
                if (sub_type == 0xFF) {
                    // End of entire device path
                    break;
                }
                break;
        }

        data += read_le16((uint8_t *)&dp->length);
    }

    *p = '\0';
    return result;
}

// Parse Boot#### variable
static void parse_boot_entry(const char *name, const uint8_t *data, uint32_t data_size) {
    if (data_size < sizeof(efi_load_option_t)) return;

    efi_load_option_t *load_option = (efi_load_option_t *)data;
    uint32_t attributes = read_le32((uint8_t *)&load_option->attributes);
    uint16_t file_path_list_length = read_le16((uint8_t *)&load_option->file_path_list_length);

    // Extract boot number from name (e.g., "Boot0006" -> 6)
    uint16_t boot_num = 0;
    if (sscanf(name, "Boot%hx", &boot_num) != 1) return;

    // Find description (Unicode string after load_option)
    const uint16_t *desc_ptr = (uint16_t *)(data + sizeof(efi_load_option_t));
    char description[256];
    unicode_to_ascii(description, desc_ptr, 512);

    // Find file path list
    const uint8_t *file_path_start = data + sizeof(efi_load_option_t);
    // Skip description (find null terminator)
    while (*file_path_start != 0 || *(file_path_start + 1) != 0) {
        file_path_start += 2;
        if (file_path_start >= data + data_size) break;
    }
    file_path_start += 2; // Skip null terminator

    // Parse device path
    char *device_path = parse_device_path(file_path_start, file_path_list_length);

    // Add to boot entries
    boot_entries = realloc(boot_entries, (boot_entry_count + 1) * sizeof(boot_entry_t));
    boot_entries[boot_entry_count].boot_num = boot_num;
    boot_entries[boot_entry_count].attributes = attributes;
    boot_entries[boot_entry_count].description = strdup(description);
    boot_entries[boot_entry_count].device_path = device_path;
    boot_entries[boot_entry_count].is_active = (attributes & 0x00000001) != 0;
    boot_entry_count++;
}

// Parse BootOrder variable
static void parse_boot_order(const uint8_t *data, uint32_t data_size) {
    boot_order_count = data_size / sizeof(uint16_t);
    boot_order = malloc(boot_order_count * sizeof(uint16_t));
    for (int i = 0; i < boot_order_count; i++) {
        boot_order[i] = read_le16(data + i * sizeof(uint16_t));
    }
}

// Parse BootCurrent variable
static void parse_boot_current(const uint8_t *data, uint32_t data_size) {
    if (data_size >= sizeof(uint16_t)) {
        boot_current = read_le16(data);
    }
}

// Find variable in the file
static void find_variables(uint8_t *data, size_t size, int track_boot_order) {
    efi_guid_t boot_manager_guid = EFI_BOOT_MANAGER_GUID;
    uint8_t *p = data;
    const uint8_t *end = data + size;

    // Variables can be stored in different formats:
    // Format 1: [NameSize][DataSize][GUID][Name][Data][StartId][State]...
    // Format 2: [field][GUID][Name][Data][padding][StartId][State]...
    // Look for Boot Manager GUID
    while (p < end - 20) {
        // Try Format 1: Check if there's NameSize and DataSize before GUID
        if (p + 8 <= end) {
            uint32_t name_size = read_le32(p);
            uint32_t data_size = read_le32(p + 4);
            if (name_size > 0 && name_size < 256 && data_size > 0 && data_size < 65536) {
                const uint8_t *guid_pos = p + 8;
                if (guid_pos + 16 <= end) {
                    efi_guid_t *guid = (efi_guid_t *)guid_pos;
                    if (guid_equal(guid, &boot_manager_guid)) {
                        // Found Format 1 variable
                        const uint8_t *name_start = guid_pos + 16;
                        const uint8_t *var_data = name_start + name_size;
                        
                        if (var_data + data_size <= end) {
                            // Extract name
                            char name[256];
                            unicode_to_ascii(name, (uint16_t *)name_start, name_size);
                            
                            // Process boot variables
                            if (strncmp(name, "Boot", 4) == 0 && strlen(name) == 8) {
                                parse_boot_entry(name, var_data, data_size);
                            } else if (strcmp(name, "BootOrder") == 0) {
                                parse_boot_order(var_data, data_size);
                                // Track location for modification
                                if (track_boot_order) {
                                    boot_order_data_ptr = (uint8_t *)var_data;
                                    boot_order_size_ptr = (uint32_t *)(p + 4);
                                    boot_order_original_size = data_size;
                                    boot_order_format = 1;
                                }
                            } else if (strcmp(name, "BootCurrent") == 0) {
                                parse_boot_current(var_data, data_size);
                            }
                        }
                    }
                }
            }
        }
        
        // Try Format 2: Check if GUID is at offset 4
        const uint8_t *guid_pos = p + 4;
        if (guid_pos + 16 <= end) {
            efi_guid_t *guid = (efi_guid_t *)guid_pos;
            if (guid_equal(guid, &boot_manager_guid)) {
                // Found GUID, name starts after it
                const uint8_t *name_start = guid_pos + 16;
                const uint8_t *name_ptr = name_start;
                
                // Find name end (null-terminated Unicode string)
                while (name_ptr < end - 1 && (name_ptr[0] != 0 || name_ptr[1] != 0)) {
                    name_ptr += 2;
                }
                name_ptr += 2; // Skip null terminator
                
                // Look for data before header (Format 2 with data before header)
                // Check if there's data between name and header
                const uint8_t *search = name_ptr;
                const uint8_t *header = NULL;
                uint32_t data_size = 0;
                
                // Look for StartId (0x55AA) to find header
                while (search < end - 2 && search < name_ptr + 256) {
                    if (read_le16(search) == 0x55AA) {
                        header = search;
                        uint8_t state = header[2];
                        if (state == EFI_VARIABLE_HEADER_VALID) {
                            // Calculate data size (from name end to header)
                            data_size = search - name_ptr;
                            // Skip padding (look for 0xFFFF or 0x0000)
                            while (data_size > 2 && ((name_ptr[data_size-2] == 0xFF && name_ptr[data_size-1] == 0xFF) ||
                                   (name_ptr[data_size-2] == 0x00 && name_ptr[data_size-1] == 0x00))) {
                                data_size -= 2;
                            }
                            break;
                        }
                    }
                    search++;
                }
                
                if (header != NULL && data_size > 0) {
                    // Extract name
                    uint32_t name_size = name_ptr - name_start;
                    char name[256];
                    unicode_to_ascii(name, (uint16_t *)name_start, name_size);
                    
                    // Data is between name and header
                    const uint8_t *var_data = name_ptr;
                    
                    if (var_data + data_size <= end) {
                        // Process boot variables
                        if (strncmp(name, "Boot", 4) == 0 && strlen(name) == 8) {
                            parse_boot_entry(name, var_data, data_size);
                        } else if (strcmp(name, "BootOrder") == 0) {
                            parse_boot_order(var_data, data_size);
                            // Track location for modification
                            if (track_boot_order) {
                                boot_order_data_ptr = (uint8_t *)var_data;
                                boot_order_size_ptr = NULL; // Format 2 doesn't have explicit size field
                                boot_order_original_size = data_size;
                                boot_order_format = 2;
                            }
                        } else if (strcmp(name, "BootCurrent") == 0) {
                            parse_boot_current(var_data, data_size);
                        }
                    }
                }
            }
        }
        p++;
    }
}

// Compare boot entries for sorting
static int compare_boot_entries(const void *a, const void *b) {
    const boot_entry_t *ea = (const boot_entry_t *)a;
    const boot_entry_t *eb = (const boot_entry_t *)b;
    return ea->boot_num - eb->boot_num;
}

// Check if boot_num is in BootOrder
static int is_in_boot_order(uint16_t boot_num) {
    for (int i = 0; i < boot_order_count; i++) {
        if (boot_order[i] == boot_num) {
            return 1;
        }
    }
    return 0;
}

// Write little-endian uint16
static void write_le16(uint8_t *p, uint16_t value) {
    *(uint16_t *)p = htole16(value);
}

// Write little-endian uint32
static void write_le32(uint8_t *p, uint32_t value) {
    *(uint32_t *)p = htole32(value);
}

// Modify BootOrder variable using tracked location
static int modify_boot_order(uint16_t new_boot_num) {
    if (boot_order_data_ptr == NULL) {
        return 0; // BootOrder not found
    }
    
    if (boot_order_format == 1) {
        // Format 1: has explicit data size field
        if (boot_order_size_ptr != NULL) {
            // Modify data size to 2 (single uint16_t)
            write_le32((uint8_t *)boot_order_size_ptr, 2);
        }
        // Write new boot number (little-endian)
        write_le16(boot_order_data_ptr, new_boot_num);
        // Zero out remaining data area
        for (uint32_t i = 2; i < boot_order_original_size; i++) {
            boot_order_data_ptr[i] = 0;
        }
        return 1;
    } else if (boot_order_format == 2) {
        // Format 2: data size is implicit (distance to header)
        // Write new boot number (little-endian)
        write_le16(boot_order_data_ptr, new_boot_num);
        // Zero out the rest of the data area
        for (uint32_t i = 2; i < boot_order_original_size; i++) {
            boot_order_data_ptr[i] = 0;
        }
        return 1;
    }
    
    return 0;
}

// Print output similar to efibootmgr
static void print_output(void) {
    // Print BootCurrent
    printf("BootCurrent: %04X\n", boot_current);

    // Print BootOrder
    if (boot_order_count > 0) {
        printf("BootOrder: ");
        for (int i = 0; i < boot_order_count; i++) {
            if (i > 0) printf(", ");
            printf("%04X", boot_order[i]);
        }
        printf("\n");
    }

    // Print boot entries in BootOrder
    if (boot_order_count > 0) {
        for (int i = 0; i < boot_order_count; i++) {
            uint16_t boot_num = boot_order[i];
            for (int j = 0; j < boot_entry_count; j++) {
                if (boot_entries[j].boot_num == boot_num) {
                    boot_entry_t *entry = &boot_entries[j];
                    printf("%cBoot%04X%c %s %s\n",
                        (entry->boot_num == boot_current) ? '+' : ' ',
                        entry->boot_num,
                        entry->is_active ? '*' : ' ',
                        entry->description,
                        entry->device_path ? entry->device_path : "");
                    break;
                }
            }
        }
    } else {
        // Sort boot entries by boot number if no BootOrder
        qsort(boot_entries, boot_entry_count, sizeof(boot_entry_t), compare_boot_entries);
        
        // Print all entries
        for (int i = 0; i < boot_entry_count; i++) {
            boot_entry_t *entry = &boot_entries[i];
            printf("%cBoot%04X%c %s %s\n",
                (entry->boot_num == boot_current) ? '+' : ' ',
                entry->boot_num,
                entry->is_active ? '*' : ' ',
                entry->description,
                entry->device_path ? entry->device_path : "");
        }
    }

    // Print unreferenced variables (entries not in BootOrder)
    if (boot_order_count > 0) {
        int unreferenced_count = 0;
        for (int i = 0; i < boot_entry_count; i++) {
            if (!is_in_boot_order(boot_entries[i].boot_num)) {
                unreferenced_count++;
            }
        }
        
        if (unreferenced_count > 0) {
            printf("\nUnreferenced Variables:\n");
            // Sort unreferenced entries by boot number
            boot_entry_t *unreferenced = malloc(unreferenced_count * sizeof(boot_entry_t));
            int idx = 0;
            for (int i = 0; i < boot_entry_count; i++) {
                if (!is_in_boot_order(boot_entries[i].boot_num)) {
                    unreferenced[idx++] = boot_entries[i];
                }
            }
            qsort(unreferenced, unreferenced_count, sizeof(boot_entry_t), compare_boot_entries);
            
            // Print unreferenced entries
            for (int i = 0; i < unreferenced_count; i++) {
                boot_entry_t *entry = &unreferenced[i];
                printf("Boot%04X%c %s %s\n",
                    entry->boot_num,
                    entry->is_active ? '*' : ' ',
                    entry->description,
                    entry->device_path ? entry->device_path : "");
            }
            
            free(unreferenced);
        }
    }
}

int main(int argc, char *argv[]) {
    const char *filename = NULL;
    const char *boot_num_str = NULL;
    uint16_t new_boot_num = 0;
    int modify_mode = 0;

    // Parse command-line arguments
    if (argc < 2) {
        printf("%s: path_to_vars [-b boot_num]\n", argv[0]);
        return 1;
    }

    filename = argv[1];
    
    // Check for -b option
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0) {
            if (i + 1 < argc) {
                boot_num_str = argv[i + 1];
                modify_mode = 1;
                i++; // Skip the value
            } else {
                fprintf(stderr, "Error: -b option requires a value\n");
                return 1;
            }
        }
    }

    // Parse boot number if provided
    if (modify_mode) {
        // Parse as hex (supports both "0001" and "1")
        char *endptr;
        unsigned long val = strtoul(boot_num_str, &endptr, 16);
        if (*endptr != '\0' || val > 0xFFFF) {
            fprintf(stderr, "Error: Invalid boot number '%s'\n", boot_num_str);
            return 1;
        }
        new_boot_num = (uint16_t)val;
    }

    // Open and map file
    int open_flags = modify_mode ? O_RDWR : O_RDONLY;
    int prot_flags = modify_mode ? (PROT_READ | PROT_WRITE) : PROT_READ;
    int map_flags = modify_mode ? MAP_SHARED : MAP_PRIVATE;

    int fd = open(filename, open_flags);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return 1;
    }

    size_t size = st.st_size;
    uint8_t *data = mmap(NULL, size, prot_flags, map_flags, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // Reset BootOrder tracking
    boot_order_data_ptr = NULL;
    boot_order_size_ptr = NULL;
    boot_order_original_size = 0;
    boot_order_format = 0;

    // Parse variables (track BootOrder location if modifying)
    find_variables(data, size, modify_mode ? 1 : 0);

    // Modify BootOrder if requested
    if (modify_mode) {
        if (modify_boot_order(new_boot_num)) {
            // Sync changes to disk
            if (msync(data, size, MS_SYNC) < 0) {
                perror("msync");
                munmap(data, size);
                close(fd);
                return 1;
            }
        } else {
            fprintf(stderr, "Error: BootOrder variable not found in file\n");
            munmap(data, size);
            close(fd);
            return 1;
        }
    }

    // Re-parse variables after modification to show updated values
    if (modify_mode) {
        // Reset state
        for (int i = 0; i < boot_entry_count; i++) {
            free(boot_entries[i].description);
            free(boot_entries[i].device_path);
        }
        free(boot_entries);
        free(boot_order);
        boot_entries = NULL;
        boot_entry_count = 0;
        boot_order = NULL;
        boot_order_count = 0;
        boot_current = 0;
        
        // Reset BootOrder tracking
        boot_order_data_ptr = NULL;
        boot_order_size_ptr = NULL;
        boot_order_original_size = 0;
        boot_order_format = 0;
        
        // Re-parse
        find_variables(data, size, 0);
    }

    // Print output
    print_output();

    // Cleanup
    for (int i = 0; i < boot_entry_count; i++) {
        free(boot_entries[i].description);
        free(boot_entries[i].device_path);
    }
    free(boot_entries);
    free(boot_order);

    munmap(data, size);
    close(fd);

    return 0;
}
