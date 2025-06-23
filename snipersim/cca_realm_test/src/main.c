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

int main()
{
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
    start_world(&worlds[1]);
    start_world(&worlds[2]);
    start_world(&worlds[3]);
    sleep(5);
        // 等待一段时间，让世界线程运行
        //sleep(2); // 简单等待，实际应用中可能需要更复杂的同步机制
    //}

    free_memory(&worlds[0], world_memories[0]);
    free_memory(&worlds[1], world_memories[1]);
    free_memory(&worlds[2], world_memories[2]);
    free_memory(&worlds[3], world_memories[3]);
}
