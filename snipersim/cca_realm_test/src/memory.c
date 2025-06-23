/*
 * @Author: Skyyyy
 * @Date: 2025-04-20 09:40:09
 * @Description: Ciallo(∠・ω< )⌒★
 */
#include "../include/memory.h"
#include <stdio.h>

/**
 * @description: 初始化内存分配跟踪数组
 * @return {*}
 */
void initialize_memory_management()
{
    for (int i = 0; i < WORLD_COUNT; i++)
    {
        allocations[i].memory_ptr = NULL;
        allocations[i].size = 0;
    }
}

/**
 * @description: 为指定世界分配指定大小的内存
 * @param {ccaWorld} *world
 * @param {size_t} world_size
 * @return {*}
 */
void *allocate_world_memory(ccaWorld *world, size_t world_size)
{
    if (world == NULL)
    {
        fprintf(stderr, "[ERR] Invalid world pointer.\n");
        return NULL;
    }

    void *new_memory = malloc(world_size);
    if (new_memory == NULL)
    {
        fprintf(stderr, "[ERR] Memory allocation failed.\n");
        return NULL;
    }

    for (int i = 0; i < WORLD_COUNT; i++)
    {
        if (allocations[i].memory_ptr == NULL)
        {
            allocations[i].memory_ptr = new_memory;
            allocations[i].size = world_size;
            break;
        }
    }

    printf("[CCA] Allocated %zu bytes for world type %d\n", world_size, world->type);
    return new_memory;
}

// 获取指定世界已分配内存的指针
void *get_allocated_memory_ptr(ccaWorld *world)
{
    if (world == NULL)
    {
        fprintf(stderr, "Invalid world pointer.\n");
        return 0;
    }

    if (allocations[world->type].memory_ptr != NULL)
    {
        return allocations[world->type].memory_ptr;
    }
    return 0; // 未找到分配的内存
}

void *get_allocated_memory_ptr_id(int i)
{
    if (allocations[i].memory_ptr != NULL)
    {
        return allocations[i].memory_ptr;
    }
    return 0; // 未找到分配的内存
}

size_t get_allocated_memory_size(ccaWorld *world)
{
    if (world == NULL)
    {
        fprintf(stderr, "Invalid world pointer.\n");
        return 0;
    }

    if (allocations[world->type].memory_ptr != NULL)
    {
        return allocations[world->type].size;
    }
    return 0; // 未找到分配的内存
}

// 释放指定的世界的内存
void free_memory(ccaWorld *world, void *memory)
{
    if (world == NULL || memory == NULL)
    {
        fprintf(stderr, "Invalid world or memory pointer.\n");
        return;
    }

    // 找到对应的内存分配并释放
    for (int i = 0; i < WORLD_COUNT; i++)
    {
        if (allocations[i].memory_ptr == memory)
        {
            free(allocations[i].memory_ptr);
            allocations[i].memory_ptr = NULL;
            allocations[i].size = 0;
            printf("Freed memory for world type %d\n", world->type);
            return;
        }
    }

    fprintf(stderr, "Memory not found for world type %d\n", world->type);
}