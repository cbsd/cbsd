#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include "simplexml.h"

#define MAX_CPUS 512
#define MAX_SOCKETS 8

typedef struct {
    int cpus[MAX_CPUS];
    int count;
} cpu_list_t;

typedef struct {
    cpu_list_t sockets[MAX_SOCKETS];
    int socket_count;
    int cores_per_socket;
    int threads_per_core;
    int total_cores;
} topology_t;

topology_t topology = {0};

void* handler_handlers(SimpleXmlParser parser,
    SimpleXmlEvent event, const char *szName, const char *szAttribute,
    const char *szValue);

int parse_int(const char *str) {
    char *end;
    return strtol(str, &end, 10);
}

void parse_cpu_list(const char *content, cpu_list_t *list) {
    const char *p = content;
    list->count = 0;
    
    while (*p && list->count < MAX_CPUS) {
        while (*p && (*p == ' ' || *p == ',' || *p == '\n' || *p == '\r' || *p == '\t')) {
            p++;
        }
        if (!*p) break;
        
        int num = 0;
        int sign = 1;
        if (*p == '-') {
            sign = -1;
            p++;
        }
        while (isdigit(*p)) {
            num = num * 10 + (*p - '0');
            p++;
        }
        num *= sign;
        list->cpus[list->count++] = num;
    }
}

void* handler_handlers(SimpleXmlParser parser,
    SimpleXmlEvent event, const char *szName, const char *szAttribute,
    const char *szValue) {
    
    static int current_level = 0;
    static int current_cache_level = 0;
    static int parsing_socket = 0;
    static int socket_idx = -1;
    static int in_thread_group = 0;
    
    if (event == ADD_SUBTAG) {
        if (strcmp(szName, "groups") == 0 || 
            strcmp(szName, "group") == 0 || 
            strcmp(szName, "children") == 0 ||
            strcmp(szName, "cpu") == 0 || 
            strcmp(szName, "flags") == 0 || 
            strcmp(szName, "flag") == 0) {
            return handler_handlers;
        }
        return NULL;
    }
    
    if (event == ADD_ATTRIBUTE) {
        if (strcmp(szName, "group") == 0 && szAttribute && strcmp(szAttribute, "level") == 0) {
            current_level = parse_int(szValue);
        } else if (strcmp(szName, "group") == 0 && szAttribute && strcmp(szAttribute, "cache-level") == 0) {
            current_cache_level = parse_int(szValue);
            if (((current_level == 2 && current_cache_level == 2) || (current_level == 3 && current_cache_level == 2)) && parsing_socket) {
                in_thread_group = 1;
            }
        } else if (strcmp(szName, "cpu") == 0 && szAttribute && strcmp(szAttribute, "count") == 0) {
            if (in_thread_group) {
                int thread_count = parse_int(szValue);
                if (thread_count > topology.threads_per_core) {
                    topology.threads_per_core = thread_count;
                }
            }
        }
    }
    
    if (event == FINISH_ATTRIBUTES) {
        if (strcmp(szName, "group") == 0) {
            if (current_level == 2 && current_cache_level == 3) {
                parsing_socket = 1;
                socket_idx++;
                if (socket_idx >= MAX_SOCKETS) socket_idx = MAX_SOCKETS - 1;
            } else if (current_level == 1 && current_cache_level == 3) {
                parsing_socket = 1;
                socket_idx++;
                if (socket_idx >= MAX_SOCKETS) socket_idx = MAX_SOCKETS - 1;
            }
        }
    }
    
    if (event == ADD_CONTENT) {
        if (strcmp(szName, "cpu") == 0) {
            if (parsing_socket && socket_idx >= 0 && socket_idx < MAX_SOCKETS && !in_thread_group) {
                cpu_list_t list;
                parse_cpu_list(szValue, &list);
                
                cpu_list_t *sock = &topology.sockets[socket_idx];
                for (int i = 0; i < list.count && sock->count < MAX_CPUS; i++) {
                    sock->cpus[sock->count++] = list.cpus[i];
                }
            }
        }
    }
    
    if (event == FINISH_TAG) {
        if (strcmp(szName, "group") == 0) {
            int saved_cache = current_cache_level;
            int saved_level = current_level;
            if (saved_cache == 3) {
                parsing_socket = 0;
            } else if ((saved_level == 2 && saved_cache == 2) || (saved_level == 3 && saved_cache == 2)) {
                in_thread_group = 0;
            }
            current_level = 0;
            current_cache_level = 0;
        }
    }
    
    return handler_handlers;
}

void analyze_topology(void) {
    topology.socket_count = 0;
    
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (topology.sockets[i].count > 0) {
            topology.socket_count++;
        }
    }
    
    if (topology.socket_count == 0) {
        printf("sockets_num=\"0\"\n");
        printf("cores_num=\"0\"\n");
        printf("threads_num=\"0\"\n");
        printf("cores_max=\"0\"\n");
        return;
    }
    
    int cpus_per_socket = topology.sockets[0].count;
    for (int i = 1; i < topology.socket_count; i++) {
        if (topology.sockets[i].count < cpus_per_socket) {
            cpus_per_socket = topology.sockets[i].count;
        }
    }
    
    if (cpus_per_socket == 0) {
        return;
    }
    
    if (topology.threads_per_core == 0) {
        topology.threads_per_core = 1;
    }
    
    topology.cores_per_socket = cpus_per_socket / topology.threads_per_core;
}

void print_topology(void) {
    analyze_topology();
    
    printf("sockets_num=\"%d\"\n", topology.socket_count);
    printf("cores_num=\"%d\"\n", topology.cores_per_socket);
    printf("threads_num=\"%d\"\n", topology.threads_per_core > 1 ? topology.threads_per_core : 0);
    
    int cores_max = topology.socket_count * topology.cores_per_socket;
    if (topology.threads_per_core > 1) {
        cores_max *= topology.threads_per_core;
    }
    printf("cores_max=\"%d\"\n", cores_max);
    
    for (int sock = 0; sock < topology.socket_count; sock++) {
        cpu_list_t *sock_cpus = &topology.sockets[sock];
        
        printf("cores_by_socket%d=\"", sock);
        int first = 1;
        for (int i = 0; i < sock_cpus->count; i++) {
            if (topology.threads_per_core > 1) {
                if (i % topology.threads_per_core == 0) {
                    if (!first) printf(" ");
                    printf("%d", sock_cpus->cpus[i]);
                    first = 0;
                }
            } else {
                if (!first) printf(" ");
                printf("%d", sock_cpus->cpus[i]);
                first = 0;
            }
        }
        printf("\"\n");
        
        if (topology.threads_per_core > 1) {
            printf("threads_by_socket%d=\"", sock);
            first = 1;
            for (int i = 0; i < sock_cpus->count; i++) {
                if (i % topology.threads_per_core == 1) {
                    if (!first) printf(" ");
                    printf("%d", sock_cpus->cpus[i]);
                    first = 0;
                }
            }
            printf("\"\n");
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <xml_file>\n", argv[0]);
        return 1;
    }
    
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *data = malloc(size + 1);
    if (!data) {
        perror("malloc");
        fclose(f);
        return 1;
    }
    
    size_t read = fread(data, 1, size, f);
    fclose(f);
    data[read] = '\0';
    
    SimpleXmlParser parser = simpleXmlCreateParser(data, size);
    if (!parser) {
        fprintf(stderr, "Failed to create parser\n");
        free(data);
        return 1;
    }
    
    int result = simpleXmlParse(parser, handler_handlers);
    if (result != 0) {
        char *err = simpleXmlGetErrorDescription(parser);
        fprintf(stderr, "Parse error: %s\n", err);
        simpleXmlDestroyParser(parser);
        free(data);
        return 1;
    }
    
    print_topology();
    
    simpleXmlDestroyParser(parser);
    free(data);
    return 0;
}
