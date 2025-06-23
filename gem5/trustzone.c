#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <time.h>

// 内存配置常量
#define BLOCK_SIZE (200UL * 1024 * 1024)      // 每个内存块200MB
#define NORMAL_WORLD_BLOCKS 10                // 普通世界10个内存块(2GB)
#define SECURE_WORLD_BLOCKS 10                // 安全世界10个内存块(2GB)
#define TEST_ITERATIONS 100000                 // 随机测试次数
#define ADDRESS_SPACE_SIZE (4UL * 1024 * 1024 * 1024) // 4GB地址空间

typedef enum {
    NORMAL_WORLD,
    SECURE_WORLD
} WorldState;

typedef struct {
    uint8_t* base_addr;
    size_t size;
    bool is_secure;
    uintptr_t virtual_base;  // 虚拟基地址
} MemoryBlock;

WorldState current_world = NORMAL_WORLD;
MemoryBlock normal_blocks[NORMAL_WORLD_BLOCKS];
MemoryBlock secure_blocks[SECURE_WORLD_BLOCKS];

// 函数声明
void init_memory_blocks();
void* virtual_to_physical(uintptr_t virtual_addr);
bool check_memory_access(uintptr_t virtual_addr);
uint8_t secure_read(uintptr_t virtual_addr);
void secure_write(uintptr_t virtual_addr, uint8_t value);
void switch_world(WorldState new_world);
void cleanup_memory();
void secure_world_random_access_test();

// 切换世界状态
void switch_world(WorldState new_world) {
    printf("切换世界: %s -> %s\n",
           current_world == NORMAL_WORLD ? "普通世界" : "安全世界",
           new_world == NORMAL_WORLD ? "普通世界" : "安全世界");
    current_world = new_world;
}

// 初始化内存块
void init_memory_blocks() {
    // 分配普通世界内存块
    for (int i = 0; i < NORMAL_WORLD_BLOCKS; i++) {
        normal_blocks[i].base_addr = mmap(NULL, BLOCK_SIZE, 
                                       PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS, 
                                       -1, 0);
        if (normal_blocks[i].base_addr == MAP_FAILED) {
            perror("普通世界内存分配失败");
            exit(EXIT_FAILURE);
        }
        normal_blocks[i].size = BLOCK_SIZE;
        normal_blocks[i].is_secure = false;
        normal_blocks[i].virtual_base = 0x00000000 + i * BLOCK_SIZE;
        printf("分配普通世界块 %d: 物理 %p - 虚拟 0x%08lx\n", 
               i, normal_blocks[i].base_addr, normal_blocks[i].virtual_base);
    }

    // 分配安全世界内存块
    for (int i = 0; i < SECURE_WORLD_BLOCKS; i++) {
        secure_blocks[i].base_addr = mmap(NULL, BLOCK_SIZE,
                                       PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS,
                                       -1, 0);
        if (secure_blocks[i].base_addr == MAP_FAILED) {
            perror("安全世界内存分配失败");
            exit(EXIT_FAILURE);
        }
        secure_blocks[i].size = BLOCK_SIZE;
        secure_blocks[i].is_secure = true;
        secure_blocks[i].virtual_base = 0x80000000 + i * BLOCK_SIZE;
        printf("分配安全世界块 %d: 物理 %p - 虚拟 0x%08lx\n", 
               i, secure_blocks[i].base_addr, secure_blocks[i].virtual_base);
    }
}

// 将虚拟地址转换为物理地址
void* virtual_to_physical(uintptr_t virtual_addr) {
    // 检查普通世界内存
    for (int i = 0; i < NORMAL_WORLD_BLOCKS; i++) {
        uintptr_t block_start = normal_blocks[i].virtual_base;
        uintptr_t block_end = block_start + normal_blocks[i].size;
        if (virtual_addr >= block_start && virtual_addr < block_end) {
            uintptr_t offset = virtual_addr - block_start;
            return normal_blocks[i].base_addr + offset;
        }
    }

    // 检查安全世界内存
    for (int i = 0; i < SECURE_WORLD_BLOCKS; i++) {
        uintptr_t block_start = secure_blocks[i].virtual_base;
        uintptr_t block_end = block_start + secure_blocks[i].size;
        if (virtual_addr >= block_start && virtual_addr < block_end) {
            uintptr_t offset = virtual_addr - block_start;
            return secure_blocks[i].base_addr + offset;
        }
    }

    printf("地址转换失败: 虚拟地址 0x%08lx 未映射\n", virtual_addr);
    return NULL;
}

// 检查内存访问权限
bool check_memory_access(uintptr_t virtual_addr) {
    // 检查普通世界内存
    for (int i = 0; i < NORMAL_WORLD_BLOCKS; i++) {
        uintptr_t block_start = normal_blocks[i].virtual_base;
        uintptr_t block_end = block_start + normal_blocks[i].size;
        if (virtual_addr >= block_start && virtual_addr < block_end) {
            return true; // 普通世界内存对所有世界可访问
        }
    }

    // 检查安全世界内存
    for (int i = 0; i < SECURE_WORLD_BLOCKS; i++) {
        uintptr_t block_start = secure_blocks[i].virtual_base;
        uintptr_t block_end = block_start + secure_blocks[i].size;
        if (virtual_addr >= block_start && virtual_addr < block_end) {
            return (current_world == SECURE_WORLD); // 仅安全世界可访问
        }
    }

    return false; // 地址不在任何分配的内存区域
}

// 安全内存读取
uint8_t secure_read(uintptr_t virtual_addr) {
    if (!check_memory_access(virtual_addr)) {
        printf("安全违规: 非法读取地址 0x%08lx\n", virtual_addr);
        return 0xFF; // 返回默认值而不是退出，便于测试
    }
    void* phys_addr = virtual_to_physical(virtual_addr);
    if (phys_addr == NULL) {
        return 0xFF;
    }
    return *((uint8_t*)phys_addr);
}

// 安全内存写入
void secure_write(uintptr_t virtual_addr, uint8_t value) {
    if (!check_memory_access(virtual_addr)) {
        printf("安全违规: 非法写入地址 0x%08lx\n", virtual_addr);
        return; // 不退出，便于测试
    }
    void* phys_addr = virtual_to_physical(virtual_addr);
    if (phys_addr != NULL) {
        *((uint8_t*)phys_addr) = value;
    }
}

// 安全世界随机访问测试
void secure_world_random_access_test() {
    printf("\n开始安全世界随机访问测试(4GB地址空间,%d次迭代)...\n", TEST_ITERATIONS);
    
    unsigned int seed = (unsigned int)time(NULL);
    int valid_access = 0;
    int secure_access = 0;
    int normal_access = 0;
    int invalid_access = 0;
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        // 生成随机地址 (0x00000000 到 0xFFFFFFFF)
        uintptr_t addr = (uintptr_t)(rand_r(&seed) % ADDRESS_SPACE_SIZE);
        
        // 随机决定是读还是写
        bool is_write = (rand_r(&seed) % 2) == 0;
        
        if (is_write) {
            uint8_t value = (uint8_t)(rand_r(&seed) % 256);
            secure_write(addr, value);
        } else {
            secure_read(addr);
        }
        
        // 统计访问结果
        if (addr >= 0x80000000 && addr < 0x80000000 + SECURE_WORLD_BLOCKS * BLOCK_SIZE) {
            secure_access++;
            valid_access++;
        } else if (addr < NORMAL_WORLD_BLOCKS * BLOCK_SIZE) {
            normal_access++;
            valid_access++;
        } else {
            invalid_access++;
        }
        
        // 每1000次打印进度
        if ((i + 1) % 1000 == 0) {
            printf("已完成 %d/%d 次测试...\n", i + 1, TEST_ITERATIONS);
        }
    }
    
    printf("\n测试结果:\n");
    printf("有效访问: %d (%.2f%%)\n", valid_access, (float)valid_access * 100 / TEST_ITERATIONS);
    printf("  - 安全内存访问: %d\n", secure_access);
    printf("  - 普通内存访问: %d\n", normal_access);
    printf("无效访问: %d (%.2f%%)\n", invalid_access, (float)invalid_access * 100 / TEST_ITERATIONS);
}

// 释放所有内存块
void cleanup_memory() {
    for (int i = 0; i < NORMAL_WORLD_BLOCKS; i++) {
        if (normal_blocks[i].base_addr != NULL) {
            munmap(normal_blocks[i].base_addr, normal_blocks[i].size);
        }
    }
    for (int i = 0; i < SECURE_WORLD_BLOCKS; i++) {
        if (secure_blocks[i].base_addr != NULL) {
            munmap(secure_blocks[i].base_addr, secure_blocks[i].size);
        }
    }
}

int main() {
    atexit(cleanup_memory);
    init_memory_blocks();

    // 测试地址使用虚拟地址空间
    uintptr_t normal_addr = 0x10000000;  // 普通世界地址
    uintptr_t secure_addr = 0x90000000;  // 安全世界地址

    // 普通世界操作
    printf("\n在普通世界操作:\n");
    secure_write(normal_addr, 0xAB);
    printf("读取普通内存(0x%08lx): 0x%02X\n", normal_addr, secure_read(normal_addr));

    // 尝试访问安全内存 - 应该失败
    printf("尝试从普通世界访问安全内存:\n");
    secure_write(secure_addr, 0xCD);  // 会打印安全违规信息但不会退出

    // 切换到安全世界
    switch_world(SECURE_WORLD);

    printf("\n在安全世界操作:\n");
    secure_write(secure_addr, 0xEF);  // 写入安全内存 - 允许
    printf("读取安全内存(0x%08lx): 0x%02X\n", secure_addr, secure_read(secure_addr));

    // 安全世界也可以访问普通内存
    secure_write(normal_addr, 0x99);
    printf("读取普通内存(0x%08lx): 0x%02X\n", normal_addr, secure_read(normal_addr));

    // 执行随机访问测试
    secure_world_random_access_test();

    // 返回普通世界
    switch_world(NORMAL_WORLD);

    printf("\n返回普通世界:\n");
    printf("读取之前写入的普通内存(0x%08lx): 0x%02X\n", normal_addr, secure_read(normal_addr));

    return 0;
}
