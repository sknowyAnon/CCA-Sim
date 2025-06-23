#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

// 定义Realm ID类型
typedef uint64_t realm_id_t;

// Realm内存区域结构
typedef struct {
    void *base;
    size_t size;
    bool is_shared;
    int shm_fd;
    char shm_name[32];
} realm_memory_t;

// Realm上下文结构
typedef struct {
    realm_id_t id;
    realm_memory_t private_memory;
    realm_memory_t shared_memory;
    pid_t pid;
    uint8_t* key;
    size_t key_length;
    char *program;
    char **argv;
    bool is_malicious; // 标记是否为恶意Realm
} realm_context_t;

// 模拟RMM
typedef struct {
    realm_context_t *realms;
    int realm_count;
    realm_memory_t global_shared_memory;
} realm_monitor_t;

// 创建共享内存
void* create_shared_memory(realm_memory_t *mem, const char *name, size_t size);

// 销毁共享内存
void destroy_shared_memory(realm_memory_t *mem);

// 创建Realm
realm_id_t realm_create(realm_monitor_t *monitor, const char *program, 
    char *const argv[], size_t private_mem_size, 
    size_t shared_mem_size, bool is_malicious);

// 恶意行为：尝试访问其他Realm的内存
void attempt_malicious_access(realm_monitor_t *monitor, realm_id_t attacker_id);

// 启动Realm进程
bool realm_start(realm_monitor_t *monitor, realm_id_t id);

// 停止Realm
bool realm_stop(realm_monitor_t *monitor, realm_id_t id);

// 销毁Realm
void realm_destroy(realm_monitor_t *monitor, realm_id_t id);

void encrypt_memory_64(uint64_t base_address, size_t size, uint8_t* key, size_t key_length);

uint8_t* generate_random_key(size_t key_length);

void print_memory_64(uint64_t base_address, size_t size);