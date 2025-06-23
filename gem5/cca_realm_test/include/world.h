/*
 * @Author: Skyyyy
 * @Date: 2025-04-20 09:35:52
 * @Description: Ciallo(∠・ω< )⌒★
 */
#pragma once

#include <pthread.h>
#include <stdbool.h>

/* 世界类型 normal secure root realm */
typedef enum
{
    WORLD_ROOT,
    WORLD_NORMAL,
    WORLD_SECURE,
    WORLD_REALM
} worldType;

/* 世界状态 已初始化 运行中 结束 */
typedef enum
{
    INITIALIZED,
    RUNNING,
    TERMINATED
} worldState;

typedef struct
{
    pthread_t thread;       // 每个世界分配一个线程
    int worldid;            // 世界的编号，用于确认特定的世界
    worldType type;         // 世界类型 normal secure root realm
    worldState state;       // 世界状态 已初始化 运行中 结束
    void *(*entry)(void *); // 指向每个世界入口函数的指针
} ccaWorld;

/* 初始化世界 */
void initialize_world(ccaWorld *world, worldType type, void *(*entry)(void *), int id);
void initialize_all_worlds(ccaWorld *worlds, worldType type[], void *(*entries[])(void *), int count);

/* 启动世界 */
void start_world(ccaWorld *world);

/* 各个世界的入口函数 */
void *normal_world(void *arg);
void *secure_world(void *arg);
void *root_world(void *arg);
void *realm_world(void *arg);