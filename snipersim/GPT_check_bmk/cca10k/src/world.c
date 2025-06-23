/*
 * @Author: Skyyyy
 * @Date: 2025-04-20 09:35:33
 * @Description: Ciallo(∠・ω< )⌒★
 */
#include "../include/world.h"
#include "../include/gpt.h"
#include "../include/memory.h"
#include <stdio.h>
#include <stdlib.h>

void *normal_world(void *arg)
{
    printf("[CCA] Normal World is running.\n");
    return NULL;
}
void *secure_world(void *arg)
{
    printf("[CCA] Secure World is running.\n");
    return NULL;
}

void *root_world(void *arg)
{
    printf("[CCA] Root World is running.\n");
    gpt_run_init();
    printf("[GPT] PAS region initialization starting...\n");
    
    for (int i = 0; i < WORLD_COUNT; i++)
    {
        void *base_ptr = get_base_ptr(i);
        if (pas_region_init((void*)(base_ptr), SIZE_1GB, i) != 0)
        {
            printf("[GPT] PAS region initialization failed...\n");
            return 0;
        }
    }

    printf("[GPT] L0 GPT initialization starting...\n");
    if (l0_gpt_init() != 0)
    {
        printf("[GPT] L0 GPT initialization failed...\n");
        return 0;
    }

    printf("[GPT] L1 GPT initialization starting...\n");
    if (l1_gpt_init() != 0)
    {
        printf("[GPT] L1 GPT initialization failed...\n");
        return 0;
    }
    return NULL;
}

void *realm_world(void *arg)
{
    printf("[CCA] Realm World is running.\n");

    //printf("[CCA] Realm VM1 is running.\n");

    //printf("[CCA] Realm VM2 is running.\n");

    //printf("[CCA] Realm VM3 is running.\n");
    system("chmod +x ./benchmark/realmvm.sh");
    int ret = system("./benchmark/realmvm.sh");
    
    // 检查返回值
    if (ret == -1) {
        printf("[GPT] Realm VM creatation failed...\n");
    } else {
        // 返回值为脚本的退出状态
    }
    return NULL;
}
/**
 * @description: 初始化单个世界
 * @param {ccaWorld} *world
 * @param {worldType} type
 * @param {void} *
 * @param {int} id
 * @return {*}
 */
void initialize_world(ccaWorld *world, worldType type, void *(*entry)(void *), int id)
{
    if (world != NULL)
    {
        world->worldid = id;
        world->type = type;
        world->state = INITIALIZED;
        world->entry = entry;
    }
}

/**
 * @description: 初始化所有世界，每个世界调用 initialize_world()
 * @param {ccaWorld} *worlds
 * @param {worldType} type
 * @param {void} *
 * @param {int} count
 * @return {*}
 */
void initialize_all_worlds(ccaWorld *worlds, worldType type[], void *(*entries[])(void *), int count)
{
    for (int i = 0; i < count; i++)
    {
        initialize_world(&worlds[i], type[i], entries[type[i]], i);
    }
}

/**
 * @description: 启动世界，为每个世界分配一个线程后将状态切换为running
 * @param {ccaWorld} *world
 * @return {*}
 */
void start_world(ccaWorld *world)
{
    if (world != NULL && world->state == INITIALIZED)
    {
        if (pthread_create(&world->thread, NULL, world->entry, NULL) == 0)
        {
            // 状态切换为运行
            world->state = RUNNING;
        }
        else
        {
            fprintf(stderr, "[CCA] Failed to start world of type %d\n", world->type);
        }
    }
}