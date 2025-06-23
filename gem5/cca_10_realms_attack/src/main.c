/*
 * @Author: Skyyyy
 * @Date: 2025-04-20 09:14:14
 * @Description: Ciallo(∠・ω< )⌒★
 */
#include <stdio.h>
#include <unistd.h>
#include "../include/head.h"
#include "../include/memory.h"
#include "../include/world.h"
#include "../include/gpt.h"

#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// 模拟认证过程
void simulate_authentication(ccaWorld *world)
{
    printf("[CCA] Attestation World %d...\n", world->type);
    sleep(0.1); // 模拟认证耗时
    printf("[CCA] Attestation successful for World %d.\n", world->type);
}

int visitMatrix[4][4] = {
    {1, 1, 0, 0}, // Secure对其它世界的访问权限
    {0, 1, 0, 0}, // Normal对其它世界的访问权限
    {1, 1, 1, 1}, // Root对其它世界的访问权限
    {0, 1, 0, 1}  // Realm对其它世界的访问权限
    };

// 使用PCG随机数生成器（一种高质量PRNG）
typedef struct { uint64_t state;  uint64_t inc; } pcg32_random_t;

uint32_t pcg32_random_r(pcg32_random_t* rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

int main()
{
    srand(time(NULL));
    /*----------------------------------- 初始化部分 ----------------------------------------*/
    /* 初始化世界 */
    ccaWorld worlds[WORLD_COUNT] = {0}; // 创建world实例
    worldType types[] = {WORLD_ROOT, WORLD_NORMAL, WORLD_SECURE, WORLD_REALM};
    void *(*entries[])(void *) = {root_world, normal_world, secure_world, realm_world};
    initialize_all_worlds(worlds, types, entries, WORLD_COUNT);

    /*----------------------------------- 内存分配部分 ----------------------------------------*/
    /* 为每个世界分配内存 */
    printf("[GPT]========== CCA World Memory Allocation ==========\n");
    printf("[CCA] Memory Allocation Starting...\n");
    /* 四个世界size数组 */
    void *world_memories[4];
    world_memories[0] = allocate_world_memory(&worlds[0], SIZE_1GB);
    world_memories[1] = allocate_world_memory(&worlds[1], SIZE_1GB);
    world_memories[2] = allocate_world_memory(&worlds[2], SIZE_1GB);
    world_memories[3] = allocate_world_memory(&worlds[3], SIZE_1GB);

    // 获取并打印每个世界已分配的内存大小
    printf("Root World allocated memory size: 0x%lx Bytes, ptr: 0x%lx\n", get_allocated_memory_size(&worlds[0]), get_allocated_memory_ptr(&worlds[0]));
    printf("Normal World allocated memory size: 0x%lx Bytes, ptr: 0x%lx\n", get_allocated_memory_size(&worlds[1]), get_allocated_memory_ptr(&worlds[1]));
    printf("Secure World allocated memory size: 0x%lx Bytes, ptr: 0x%lx\n", get_allocated_memory_size(&worlds[2]), get_allocated_memory_ptr(&worlds[2]));
    printf("Realm World allocated memory size: 0x%lx Bytes, ptr: 0x%lx\n", get_allocated_memory_size(&worlds[3]), get_allocated_memory_ptr(&worlds[3]));
    /*----------------------------------- 认证部分 ----------------------------------------*/
    printf("[GPT]========== CCA World Attestation ==========\n");
    printf("[CCA] Attestation Starting...\n");
    // 对每个世界进行模拟认证
    for (int i = 0; i < 4; i++)
    {
        simulate_authentication(&worlds[i]);
    }

    //printf("[CCA] World Starting...\n");
    printf("[GPT]========== CCA World Starting ==========\n");
    // 启动每个世界
    //for (int i = 0; i < 4; i++)
    //{
    start_world(&worlds[0]);
    sleep(2);
    
    // int current_gpi = 0xb;
    // int allowed = 0;
    // int denied = 0;
    // int root = 0;
    // int secure = 0;
    // int normal = 0;
    // int realm = 0;

    // pcg32_random_t rng;
    // rng.state = time(NULL);
    // rng.inc = 1;

    // for (int i = 0; i < 1000000; i++)
    // {

    //     //uint32_t random_num = ((uint32_t)rand() << 16) | rand();
    //     uint32_t random_num = pcg32_random_r(&rng);
    //     int target_gpi = check_pas_gpi(random_num) >> 60;
    //     switch (target_gpi)
    //     {
    //     case 8:
    //         secure++;
    //         break;
    //     case 9:
    //         normal++;
    //         break;
    //     case 10:
    //         root++;
    //         break;
    //     case 11:
    //         realm++;
    //         break;
    //     default:
    //         break;
    //     }
    //     if (visitMatrix[current_gpi-8][target_gpi-8])
    //     {
    //         printf("vist allowed\n");
    //         allowed++;
    //     }
    //     else{
    //         printf("visit denied\n");
    //         denied++;
    //     }

    // }
    // printf("allowed: %d, denied: %d\n",allowed, denied);
    // printf("root: %d, normal: %d, secure: %d, realm: %d\n",root, normal,secure,realm);
    
    // start_world(&worlds[1]);
    // start_world(&worlds[2]);
     start_world(&worlds[3]);
     sleep(10);
    //     // 等待一段时间，让世界线程运行
    //     //sleep(2); // 简单等待，实际应用中可能需要更复杂的同步机制
    // //}

    free_memory(&worlds[0], world_memories[0]);
    free_memory(&worlds[1], world_memories[1]);
    free_memory(&worlds[2], world_memories[2]);
    free_memory(&worlds[3], world_memories[3]);
}
