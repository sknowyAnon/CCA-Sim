/*
 * @Author: Skyyyy
 * @Date: 2025-04-20 09:35:33
 * @Description: Ciallo(∠・ω< )⌒★
 */
#include "../include/world.h"
#include "../include/gpt.h"
#include "../include/memory.h"
#include "../include/realm.h"
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
    printf("[GPT] ========== Realm VM Simulation ==========\n");
    printf("[CCA] Realm World is running.\n");

    //printf("[CCA] Realm VM1 is running.\n");

    //printf("[CCA] Realm VM2 is running.\n");

    //printf("[CCA] Realm VM3 is running.\n");
    //system("chmod +x ./benchmark/realmvm.sh");
    //int ret = system("./benchmark/realmvm.sh");
    
    // 检查返回值
    //if (ret == -1) {
       // printf("[GPT] Realm VM creatation failed...\n");
    //} else {
        // 返回值为脚本的退出状态
   // }
    //return NULL;

    // 初始化随机数生成器
    srand(time(NULL));

    realm_monitor_t monitor = {0};
    
    // 创建两个Realm：一个正常，一个恶意
    char *normal_argv[] = {"normal_realm", "--role", "normal", NULL};
    char *malicious_argv[] = {"malicious_realm", "--role", "malicious", NULL};
    
    //realm_id_t normal_realm1 = realm_create(&monitor, "./benchmark/realm1", 
    //                                     normal_argv, 4096, 2048, false);
    realm_id_t malicious_realm0 = realm_create(&monitor, "./benchmark/realm1", 
        malicious_argv, 4096, 2048, true);
    realm_id_t malicious_realm1 = realm_create(&monitor, "./benchmark/realm1", 
                                            malicious_argv, 4096, 2048, true);
    realm_id_t malicious_realm2 = realm_create(&monitor, "./benchmark/realm1", 
                                                malicious_argv, 4096, 2048, true);
    realm_id_t malicious_realm3 = realm_create(&monitor, "./benchmark/realm1", 
                                                    malicious_argv, 4096, 2048, true);
    realm_id_t malicious_realm4 = realm_create(&monitor, "./benchmark/realm1", 
                                                        malicious_argv, 4096, 2048, true);
    realm_id_t malicious_realm5 = realm_create(&monitor, "./benchmark/realm1", 
                                                            malicious_argv, 4096, 2048, true);                                                
    realm_id_t malicious_realm6 = realm_create(&monitor, "./benchmark/realm1", 
                                                                malicious_argv, 4096, 2048, true); 
    realm_id_t malicious_realm7 = realm_create(&monitor, "./benchmark/realm1", 
                                                                    malicious_argv, 4096, 2048, true); 
    realm_id_t malicious_realm8 = realm_create(&monitor, "./benchmark/realm1", 
                                                                        malicious_argv, 4096, 2048, true);
    realm_id_t malicious_realm9 = realm_create(&monitor, "./benchmark/realm1", 
                                                                            malicious_argv, 4096, 2048, true);

    // if (!normal_realm || !malicious_realm) {
    //     fprintf(stderr, "Failed to create realms\n");
    //     return 1;
    // }
    
    // printf("[REALM] Created Realms: %lu (normal) and %lu (malicious)\n", 
    //        normal_realm, malicious_realm);
    // printf("[REALM] Realm VM Attestation...\n");
    // sleep(1);
    // 先启动正常Realm
    // if (!realm_start(&monitor, normal_realm)) {
    //     fprintf(stderr, "Failed to start normal realm\n");
    //     realm_destroy(&monitor, normal_realm);
    //     realm_destroy(&monitor, malicious_realm);
    //     return 1;
    // }
    
    // 然后启动恶意Realm
    // if (!realm_start(&monitor, malicious_realm)) {
    //     fprintf(stderr, "Failed to start malicious realm\n");
    //     realm_stop(&monitor, normal_realm);
    //     realm_destroy(&monitor, normal_realm);
    //     realm_destroy(&monitor, malicious_realm);
    //     return 1;
    // }

    if (!realm_start(&monitor, malicious_realm0)) {
        // fprintf(stderr, "Failed to start malicious realm\n");
        // realm_stop(&monitor, normal_realm);
        // realm_destroy(&monitor, normal_realm);
        // realm_destroy(&monitor, malicious_realm);
        return 1;
    }
    if (!realm_start(&monitor, malicious_realm1)) {
        // fprintf(stderr, "Failed to start malicious realm\n");
        // realm_stop(&monitor, normal_realm);
        // realm_destroy(&monitor, normal_realm);
        // realm_destroy(&monitor, malicious_realm);
        return 1;
    }
    if (!realm_start(&monitor, malicious_realm2)) {
        // fprintf(stderr, "Failed to start malicious realm\n");
        // realm_stop(&monitor, normal_realm);
        // realm_destroy(&monitor, normal_realm);
        // realm_destroy(&monitor, malicious_realm);
        return 1;
    }
    if (!realm_start(&monitor, malicious_realm3)) {
        // fprintf(stderr, "Failed to start malicious realm\n");
        // realm_stop(&monitor, normal_realm);
        // realm_destroy(&monitor, normal_realm);
        // realm_destroy(&monitor, malicious_realm);
        return 1;
    }
    if (!realm_start(&monitor, malicious_realm4)) {
        // fprintf(stderr, "Failed to start malicious realm\n");
        // realm_stop(&monitor, normal_realm);
        // realm_destroy(&monitor, normal_realm);
        // realm_destroy(&monitor, malicious_realm);
        return 1;
    }
    if (!realm_start(&monitor, malicious_realm5)) {
        // fprintf(stderr, "Failed to start malicious realm\n");
        // realm_stop(&monitor, normal_realm);
        // realm_destroy(&monitor, normal_realm);
        // realm_destroy(&monitor, malicious_realm);
        return 1;
    }
    if (!realm_start(&monitor, malicious_realm6)) {
        // fprintf(stderr, "Failed to start malicious realm\n");
        // realm_stop(&monitor, normal_realm);
        // realm_destroy(&monitor, normal_realm);
        // realm_destroy(&monitor, malicious_realm);
        return 1;
    }
    if (!realm_start(&monitor, malicious_realm7)) {
        // fprintf(stderr, "Failed to start malicious realm\n");
        // realm_stop(&monitor, normal_realm);
        // realm_destroy(&monitor, normal_realm);
        // realm_destroy(&monitor, malicious_realm);
        return 1;
    }
    if (!realm_start(&monitor, malicious_realm8)) {
        // fprintf(stderr, "Failed to start malicious realm\n");
        // realm_stop(&monitor, normal_realm);
        // realm_destroy(&monitor, normal_realm);
        // realm_destroy(&monitor, malicious_realm);
        return 1;
    }
    if (!realm_start(&monitor, malicious_realm9)) {
        // fprintf(stderr, "Failed to start malicious realm\n");
        // realm_stop(&monitor, normal_realm);
        // realm_destroy(&monitor, normal_realm);
        // realm_destroy(&monitor, malicious_realm);
        return 1;
    }
    
    // 等待所有Realm进程结束
    for (int i = 0; i < monitor.realm_count; i++) {
        if (monitor.realms[i].pid != -1) {
            int status;
            waitpid(monitor.realms[i].pid, &status, 0);
            printf("Realm %lu exited with status %d\n", 
                   monitor.realms[i].id, WEXITSTATUS(status));
        }
    }
    
    // 清理
    //realm_destroy(&monitor, normal_realm);
    realm_destroy(&monitor, malicious_realm0);
    realm_destroy(&monitor, malicious_realm1);
    realm_destroy(&monitor, malicious_realm2);
    realm_destroy(&monitor, malicious_realm3);
    realm_destroy(&monitor, malicious_realm4);
    realm_destroy(&monitor, malicious_realm5);
    realm_destroy(&monitor, malicious_realm6);
    realm_destroy(&monitor, malicious_realm7);
    realm_destroy(&monitor, malicious_realm8);
    realm_destroy(&monitor, malicious_realm9);

    
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
