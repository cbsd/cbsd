#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <endian.h>

#define EFI_VARIABLE_START_ID 0x55AA
#define EFI_VARIABLE_VALID_STATE 0x3F

// EFI Global Variable GUID: 8BE4DF61-93CA-11D2-AA0D-00E098032B8C

typedef struct {
    uint32_t attributes;
    uint16_t file_path_list_length;
    // description (null-terminated string) follows
    // file_path_list follows
    // optional_data follows
} __attribute__((packed)) efi_load_option;

static uint32_t read_le32(const uint8_t *data) {
    return le32toh(*(uint32_t *)data);
}

static uint16_t read_le16(const uint8_t *data) {
    return le16toh(*(uint16_t *)data);
}

static void write_le16(uint8_t *data, uint16_t value) {
    uint16_t le_value = htole16(value);
    memcpy(data, &le_value, 2);
}

static void write_le32(uint8_t *data, uint32_t value) {
    uint32_t le_value = htole32(value);
    memcpy(data, &le_value, 4);
}

// Convert UTF-16 (little-endian) to ASCII string
static int utf16_to_ascii(const uint8_t *utf16_data, size_t max_len, char *ascii_out, size_t ascii_size) {
    size_t i;
    for (i = 0; i < max_len / 2 && i < ascii_size - 1; i++) {
        uint16_t wchar = read_le16(utf16_data + i * 2);
        if (wchar == 0) {
            break;
        }
        ascii_out[i] = (char)(wchar & 0xFF); // Simple conversion
    }
    ascii_out[i] = '\0';
    return i;
}

// Search for UTF-16 string in data
static size_t find_utf16_string(const uint8_t *data, size_t size, const char *search_str) {
    size_t search_len = strlen(search_str);
    if (search_len == 0) return 0;
    
    // Convert search string to UTF-16 pattern
    uint8_t pattern[256];
    for (size_t i = 0; i < search_len && i < 128; i++) {
        pattern[i * 2] = search_str[i];
        pattern[i * 2 + 1] = 0;
    }
    
    // Search for pattern
    for (size_t i = 0; i < size - search_len * 2; i++) {
        if (memcmp(data + i, pattern, search_len * 2) == 0) {
            return i;
        }
    }
    return 0;
}

// Structure to hold BootOrder variable location
typedef struct {
    size_t name_offset;
    size_t data_size_offset;
    size_t data_offset;
    uint32_t data_size;
    int found;
} boot_order_location;

// Find BootOrder variable structure
// Based on analysis: data may come directly after name, or have data_size field
static boot_order_location find_boot_order(const uint8_t *data, size_t size) {
    boot_order_location loc = {0};
    
    // Search for ALL BootOrder occurrences and find the LAST one with valid data
    // The last BootOrder is typically the most recent/active one
    size_t name_offset = 0;
    size_t best_offset = 0;
    size_t best_data_size_offset = 0;
    size_t best_data_offset = 0;
    uint32_t best_data_size = 0;
    int best_found = 0;
    int best_has_data_size_field = 0;
    
    while (1) {
        size_t search_start = name_offset;
        name_offset = find_utf16_string(data + search_start, size - search_start, "BootOrder");
        if (name_offset == 0) {
            break;
        }
        name_offset += search_start;
        
        size_t name_end = name_offset + 20; // "BootOrder" = 9 chars * 2 + null (2 bytes) = 20 bytes
        
        // Try to find data_size field first (standard UEFI format)
        int found_with_size = 0;
        for (size_t search_offset = name_end; search_offset < name_end + 50 && search_offset + 4 < size; search_offset++) {
            uint32_t test_size = read_le32(data + search_offset);
            
            // BootOrder should have an even number of bytes (UINT16 array)
            if (test_size >= 2 && test_size <= 100 && test_size % 2 == 0) {
                size_t data_offset = search_offset + 4;
                if (data_offset + test_size <= size) {
                    // Verify it looks like boot IDs
                    int valid_count = 0;
                    uint32_t count = test_size / 2;
                    for (uint32_t i = 0; i < count && i < 50; i++) {
                        uint16_t boot_id = read_le16(data + data_offset + i * 2);
                        if (boot_id <= 0x03E7) {
                            valid_count++;
                        } else {
                            break;
                        }
                    }
                    
                    if ((uint32_t)valid_count == count && count > 0) {
                        // Found valid BootOrder with data_size field
                        // Store it but continue searching to find the last one
                        best_offset = name_offset;
                        best_data_size_offset = search_offset;
                        best_data_offset = data_offset;
                        best_data_size = test_size;
                        best_found = 1;
                        best_has_data_size_field = 1;
                        found_with_size = 1;
                        break;
                    }
                }
            }
        }
        
        // If no data_size field found, check if data comes directly after name
        if (!found_with_size) {
            size_t direct_data_offset = name_end;
            if (direct_data_offset + 2 <= size) {
                // Read boot IDs until we hit 0xFFFF or invalid value
                uint32_t data_size = 0;
                int valid = 1;
                for (size_t i = 0; i < 50 && direct_data_offset + i * 2 + 2 <= size; i++) {
                    uint16_t val = read_le16(data + direct_data_offset + i * 2);
                    if (val == 0xFFFF) {
                        // End marker found
                        break;
                    }
                    if (val <= 0x03E7) {
                        data_size += 2;
                    } else {
                        // Might be next variable marker (0x55AA) or invalid
                        if (val == 0x55AA && i > 0) {
                            // Next variable starts
                            break;
                        }
                        valid = 0;
                        break;
                    }
                }
                
                if (valid && data_size >= 2) {
                    // Found valid BootOrder with direct data (no data_size field)
                    // Store it but continue searching to find the last one
                    best_offset = name_offset;
                    best_data_size_offset = name_end; // Will write data_size here when modifying
                    best_data_offset = name_end;      // Data is currently at name_end (direct format)
                    best_data_size = data_size;
                    best_found = 1;
                    best_has_data_size_field = 0;
                }
            }
        }
        
        // Continue searching for next BootOrder
        name_offset += 2;
    }
    
    if (best_found) {
        loc.name_offset = best_offset;
        if (best_has_data_size_field) {
            // Has data_size field (standard format)
            loc.data_size_offset = best_data_size_offset;
            loc.data_offset = best_data_offset;
        } else {
            // Direct data format - data is at name_end, no data_size field
            // When reading, use name_end directly
            // When writing, we'll write data_size at name_end, then data after it
            loc.data_size_offset = best_data_size_offset; // name_end
            loc.data_offset = best_data_offset;          // name_end (current data location)
        }
        loc.data_size = best_data_size;
        loc.found = 1;
    } else if (name_offset > 0) {
        // Found BootOrder name but no valid data
        loc.name_offset = name_offset - 2; // Last found position
    }
    
    return loc;
}

// Parse BootOrder variable and return as shell-compatible string
// Returns: boot_order string (e.g., "Boot0003" or "Boot0000,Boot0001")
static void parse_boot_order(const uint8_t *data, size_t size, char *boot_order_str, size_t boot_order_size) {
    boot_order_location loc = find_boot_order(data, size);
    
    boot_order_str[0] = '\0';
    
    if (!loc.found) {
        return;
    }
    
    uint32_t count = loc.data_size / 2;
    size_t offset = 0;
    int first = 1;
    
    for (uint32_t i = 0; i < count && offset < boot_order_size - 10; i++) {
        uint16_t boot_id = read_le16(data + loc.data_offset + i * 2);
        // Only include valid boot IDs (0-999)
        if (boot_id <= 0x03E7) {
            if (!first) {
                offset += snprintf(boot_order_str + offset, boot_order_size - offset, ",");
            }
            offset += snprintf(boot_order_str + offset, boot_order_size - offset, "Boot%04X", boot_id);
            first = 0;
        } else {
            // Invalid boot ID, stop reading
            break;
        }
    }
}

// Structure to store boot entry info
typedef struct {
    char name[16];
    char description[512];
    uint32_t attributes;
    uint16_t file_path_list_length;
    int found;
} boot_entry_info;

// Parse Boot#### entries by finding descriptions
// Returns number of entries found
static int parse_boot_entries(const uint8_t *data, size_t size, boot_entry_info *entries) {
    int entry_count = 0;
    
    // Initialize entries array
    for (int i = 0; i < 256; i++) {
        entries[i].found = 0;
        entries[i].name[0] = '\0';
        entries[i].description[0] = '\0';
    }
    
    // Search for all "Boot####" patterns and extract their descriptions
    size_t offset = 0;
    while (offset < size - 16) {
        char test_name[16];
        utf16_to_ascii(data + offset, 18, test_name, sizeof(test_name));
        
        if (strncmp(test_name, "Boot", 4) == 0 && strlen(test_name) == 8 &&
            test_name[4] >= '0' && test_name[4] <= '9' &&
            test_name[5] >= '0' && test_name[5] <= '9' &&
            test_name[6] >= '0' && test_name[6] <= '9' &&
            test_name[7] >= '0' && test_name[7] <= '9') {
            
            // Check if we already found this entry
            int already_found = 0;
            for (int i = 0; i < entry_count; i++) {
                if (strcmp(entries[i].name, test_name) == 0) {
                    already_found = 1;
                    break;
                }
            }
            
            if (!already_found) {
                // Found a Boot#### entry, try to find its description
            size_t name_end = offset + 18; // 8 chars * 2 + null terminator
            size_t search_start = name_end;
            
            // Look for description after the variable data structure
            // Skip potential data_size and other fields
            for (size_t i = 0; i < 200 && search_start + i * 2 < size; i++) {
                size_t test_offset = search_start + i * 2;
                if (test_offset + sizeof(efi_load_option) < size) {
                    const efi_load_option *load_opt = (const efi_load_option *)(data + test_offset);
                    
                    if (load_opt->file_path_list_length < 65535) {
                        // Try to extract description
                        size_t desc_offset = test_offset + sizeof(efi_load_option);
                        char description[512];
                        size_t desc_len = 0;
                        
                        while (desc_offset + (desc_len + 1) * 2 < size && desc_len < 255) {
                            uint16_t wchar = read_le16(data + desc_offset + desc_len * 2);
                            if (wchar == 0) break;
                            desc_len++;
                        }
                        
                        if (desc_len > 4) { // Reasonable description length
                            utf16_to_ascii(data + desc_offset, desc_len * 2, description, sizeof(description));
                            
                            // Check if it looks like a real description
                            if (description[0] != 0 && strcspn(description, " \t\n") < strlen(description)) {
                                // Store this entry
                                if (entry_count < 256) {
                                    strcpy(entries[entry_count].name, test_name);
                                    strcpy(entries[entry_count].description, description);
                                    entries[entry_count].attributes = load_opt->attributes;
                                    entries[entry_count].file_path_list_length = load_opt->file_path_list_length;
                                    entries[entry_count].found = 1;
                                    entry_count++;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            
            }
            
            offset += 2; // Continue searching
        } else {
            offset++;
        }
    }
    
    // Store entries for output (we'll output them in shell format)
    // This function now just collects the entries, output is done in parse_uefi_vars
    // Return entry_count for use by caller
    (void)entries; // Keep entries for now, will be used by caller
    return entry_count;
}

// Output boot entries in shell-compatible format
static void output_boot_entries_shell(boot_entry_info *entries, int entry_count) {
    // Output boot_items (all available boot entries)
    printf("boot_items=\"");
    int first = 1;
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].found) {
            if (!first) printf(" ");
            printf("%s", entries[i].name);
            first = 0;
        }
    }
    printf("\"\n");
    
    // Output items_Boot####="description" for each entry
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].found) {
            // Escape special characters in description for shell compatibility
            printf("items_%s=\"", entries[i].name);
            const char *desc = entries[i].description;
            for (int j = 0; desc[j] != '\0'; j++) {
                // Escape quotes and backslashes
                if (desc[j] == '"' || desc[j] == '\\' || desc[j] == '$' || desc[j] == '`') {
                    printf("\\%c", desc[j]);
                } else {
                    printf("%c", desc[j]);
                }
            }
            printf("\"\n");
        }
    }
}

static void parse_uefi_vars(const uint8_t *data, size_t size) {
    char boot_order_str[1024] = {0};
    boot_entry_info entries[256];
    
    // Initialize entries array
    for (int i = 0; i < 256; i++) {
        entries[i].found = 0;
        entries[i].name[0] = '\0';
        entries[i].description[0] = '\0';
    }
    
    // Parse boot order
    parse_boot_order(data, size, boot_order_str, sizeof(boot_order_str));
    
    // Parse boot entries
    int entry_count = parse_boot_entries(data, size, entries);
    
    // Output in shell-compatible format
    if (boot_order_str[0] != '\0') {
        printf("boot_order=\"%s\"\n", boot_order_str);
    } else {
        printf("boot_order=\"\"\n");
    }
    
    output_boot_entries_shell(entries, entry_count);
}

// Parse boot order string (e.g., "0002,0003,0004" or "2,3,4")
static int parse_boot_order_string(const char *order_str, uint16_t *boot_ids, int max_count) {
    char *str_copy = strdup(order_str);
    if (!str_copy) {
        return -1;
    }
    
    int count = 0;
    char *token = strtok(str_copy, ",");
    
    while (token != NULL && count < max_count) {
        // Trim whitespace
        while (*token == ' ' || *token == '\t') token++;
        
        // Parse the number (handle both "0002" and "2" formats)
        unsigned long val = strtoul(token, NULL, 16);
        if (val > 0xFFFF) {
            free(str_copy);
            return -1;
        }
        
        boot_ids[count++] = (uint16_t)val;
        token = strtok(NULL, ",");
    }
    
    free(str_copy);
    return count;
}

// Find all BootOrder instances and return their offsets
static int find_all_boot_orders(const uint8_t *data, size_t size, size_t *offsets, int max_count) {
    int count = 0;
    size_t search_offset = 0;
    
    while (count < max_count) {
        size_t found = find_utf16_string(data + search_offset, size - search_offset, "BootOrder");
        if (found == 0) {
            break;
        }
        found += search_offset;
        offsets[count++] = found;
        search_offset = found + 2; // Continue searching
    }
    
    return count;
}

// Remove duplicate BootOrder entries, keeping only the last one
static int cleanup_duplicate_boot_orders(uint8_t *data, size_t size) {
    size_t offsets[256];
    int count = find_all_boot_orders(data, size, offsets, 256);
    
    if (count <= 1) {
        // No duplicates to clean up
        return 0;
    }
    
    printf("Found %d BootOrder instances, removing %d duplicates...\n", count, count - 1);
    
    // Keep the last one (most recent), invalidate all previous ones
    size_t last_offset = offsets[count - 1];
    
    // Invalidate all previous BootOrder entries by zeroing out the name
    // This makes them unrecognizable as BootOrder variables
    for (int i = 0; i < count - 1; i++) {
        size_t offset = offsets[i];
        // Zero out the "BootOrder" name (18 bytes: 9 chars * 2)
        // This effectively removes the variable from being recognized
        memset(data + offset, 0, 18);
        printf("  Invalidated BootOrder at offset 0x%zx\n", offset);
    }
    
    printf("Kept BootOrder at offset 0x%zx (latest entry)\n", last_offset);
    return 0;
}

// Modify BootOrder in the buffer
static int modify_boot_order(uint8_t *data, size_t size, const uint16_t *boot_ids, int count) {
    boot_order_location loc = find_boot_order(data, size);
    
    // If BootOrder name not found, we can't proceed
    if (loc.name_offset == 0) {
        fprintf(stderr, "Error: BootOrder variable name not found in file\n");
        return 1;
    }
    
    // Calculate new data size
    uint32_t new_data_size = count * 2; // Each boot ID is 2 bytes (UINT16)
    
    // If we found a valid BootOrder structure, use it
    if (loc.found) {
        // Check if data_size field exists (offset should be right after name)
        size_t name_end = loc.name_offset + 20;
        int has_data_size_field = (loc.data_size_offset == name_end);
        
        if (has_data_size_field) {
            // Standard format: has data_size field
            // Check if we need to resize
            if (new_data_size != loc.data_size) {
                // Check if we have space to expand
                if (new_data_size > loc.data_size) {
                    // Check if there's enough space after current data
                    size_t current_end = loc.data_offset + loc.data_size;
                    if (current_end + (new_data_size - loc.data_size) > size) {
                        fprintf(stderr, "Error: Not enough space to expand boot order (%d > %u entries)\n", 
                                count, loc.data_size / 2);
                        return 1;
                    }
                }
                // Update data_size field
                write_le32(data + loc.data_size_offset, new_data_size);
            }
            // Write new boot order data
            for (int i = 0; i < count; i++) {
                write_le16(data + loc.data_offset + i * 2, boot_ids[i]);
            }
        } else {
            // Direct data format: data comes right after name, no data_size field
            // Write data_size field at standard location, then data
            write_le32(data + name_end, new_data_size);
            size_t data_offset = name_end + 4;
            for (int i = 0; i < count; i++) {
                write_le16(data + data_offset + i * 2, boot_ids[i]);
            }
            // Write end marker if space available
            if (data_offset + new_data_size + 2 <= size) {
                write_le16(data + data_offset + new_data_size, 0xFFFF);
            }
        }
    } else {
        // BootOrder name exists but no valid data structure found
        // Data likely comes directly after the name (as seen in analyzed files)
        size_t name_end = loc.name_offset + 20; // "BootOrder" = 9 chars * 2 + null (2 bytes) = 20 bytes
        
        // Write data_size field at standard location (right after name)
        // Even if it wasn't there before, we'll write it for UEFI compliance
        size_t data_size_offset = name_end;
        size_t data_offset = name_end + 4;
        
        if (data_offset + new_data_size > size) {
            fprintf(stderr, "Error: Not enough space to write BootOrder data\n");
            return 1;
        }
        
        // Write data_size
        write_le32(data + data_size_offset, new_data_size);
        
        // Write boot order data
        for (int i = 0; i < count; i++) {
            write_le16(data + data_offset + i * 2, boot_ids[i]);
        }
        
        // Write end marker (0xFFFF) after data if there's space
        if (data_offset + new_data_size + 2 <= size) {
            write_le16(data + data_offset + new_data_size, 0xFFFF);
        }
    }
    
    printf("Boot order updated successfully: ");
    for (int i = 0; i < count; i++) {
        printf("Boot%04X", boot_ids[i]);
        if (i < count - 1) printf(", ");
    }
    printf("\n");
    
    return 0;
}

int readNVRAM(const char *vars, const char *boot_order_str, int cleanup_duplicates) {
    FILE *ptr = fopen(vars, (boot_order_str || cleanup_duplicates) ? "r+b" : "rb");
    if (!ptr) {
        fprintf(stderr, "Error: Cannot open file %s\n", vars);
        return 1;
    }

    // Get file size
    fseek(ptr, 0, SEEK_END);
    long fsize = ftell(ptr);
    fseek(ptr, 0, SEEK_SET);

    if (fsize <= 0) {
        fprintf(stderr, "Error: Invalid file size\n");
        fclose(ptr);
        return 1;
    }

    uint8_t *buffer = malloc(fsize);
    if (!buffer) {
        fprintf(stderr, "Error: Cannot allocate memory\n");
        fclose(ptr);
        return 1;
    }

    size_t read = fread(buffer, 1, fsize, ptr);
    
    if (read != (size_t)fsize) {
        fprintf(stderr, "Error: Could not read entire file\n");
        free(buffer);
        fclose(ptr);
        return 1;
    }

    // If cleanup is requested, remove duplicate BootOrder entries
    if (cleanup_duplicates) {
        if (cleanup_duplicate_boot_orders(buffer, fsize) != 0) {
            free(buffer);
            fclose(ptr);
            return 1;
        }
        
        // Write cleaned buffer back to file
        fseek(ptr, 0, SEEK_SET);
        size_t written = fwrite(buffer, 1, fsize, ptr);
        fclose(ptr);
        
        if (written != (size_t)fsize) {
            fprintf(stderr, "Error: Could not write entire file\n");
            free(buffer);
            return 1;
        }
        
        printf("Cleanup completed successfully\n");
        free(buffer);
        return 0;
    }
    
    // If boot order string is provided, modify it
    if (boot_order_str) {
        // Clean up duplicates first if there are many
        size_t offsets[256];
        int boot_order_count = find_all_boot_orders(buffer, fsize, offsets, 256);
        if (boot_order_count > 1) {
            printf("Warning: Found %d BootOrder instances. Cleaning up duplicates first...\n", boot_order_count);
            if (cleanup_duplicate_boot_orders(buffer, fsize) != 0) {
                free(buffer);
                fclose(ptr);
                return 1;
            }
        }
        
        uint16_t boot_ids[256];
        int count = parse_boot_order_string(boot_order_str, boot_ids, 256);
        
        if (count <= 0) {
            fprintf(stderr, "Error: Invalid boot order string format\n");
            fprintf(stderr, "Expected format: -o 0002,0003,0004 or -o 2,3,4\n");
            free(buffer);
            fclose(ptr);
            return 1;
        }
        
        if (modify_boot_order(buffer, fsize, boot_ids, count) != 0) {
            free(buffer);
            fclose(ptr);
            return 1;
        }
        
        // Write modified buffer back to file
        fseek(ptr, 0, SEEK_SET);
        size_t written = fwrite(buffer, 1, fsize, ptr);
        fclose(ptr);
        
        if (written != (size_t)fsize) {
            fprintf(stderr, "Error: Could not write entire file\n");
            free(buffer);
            return 1;
        }
        
        free(buffer);
        return 0;
    } else {
        // Just parse and display
        fclose(ptr);
        parse_uefi_vars(buffer, fsize);
        free(buffer);
        return 0;
    }
}

int main(int argc, char *argv[]) {
    const char *boot_order_str = NULL;
    const char *vars_file = NULL;
    int cleanup_duplicates = 0;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                boot_order_str = argv[++i];
            } else {
                fprintf(stderr, "Error: -o requires a boot order string\n");
                fprintf(stderr, "Usage: %s [-o boot_order] [-z] <path_to_vars>\n", argv[0]);
                fprintf(stderr, "Example: %s -o 2,3,4 BHYVE_UEFI_VARS.fd\n", argv[0]);
                exit(1);
            }
        } else if (strcmp(argv[i], "-z") == 0) {
            cleanup_duplicates = 1;
        } else {
            vars_file = argv[i];
        }
    }
    
    if (!vars_file) {
        fprintf(stderr, "Usage: %s [-o boot_order] [-z] <path_to_vars>\n", argv[0]);
        fprintf(stderr, "  -o boot_order  Set boot order (e.g., -o 2,3,4 or -o 0002,0003,0004)\n");
        fprintf(stderr, "  -z             Remove duplicate BootOrder entries, keep only the latest\n");
        fprintf(stderr, "  path_to_vars   Path to UEFI VARS file\n");
        exit(1);
    }
    
    return readNVRAM(vars_file, boot_order_str, cleanup_duplicates);
}