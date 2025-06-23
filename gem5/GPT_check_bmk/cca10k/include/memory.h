/*
 * @Author: Skyyyy
 * @Date: 2025-04-20 09:40:15
 * @Description: Ciallo(∠・ω< )⌒★
 */
#pragma once
#include "world.h"
#include "head.h"
#include <stdlib.h>

/* 跟踪每个世界的内存分配 */
typedef struct
{
    void *memory_ptr;
    size_t size;
} MemoryAllocation;
static MemoryAllocation allocations[WORLD_COUNT];

/* 初始化内存分配跟踪数组 */
void initialize_memory_management();
/* 为世界分配内存 */
void *allocate_world_memory(ccaWorld *world, size_t world_size);
void *get_allocated_memory_ptr(ccaWorld *world);
void *get_allocated_memory_ptr_id(int i);
size_t get_allocated_memory_size(ccaWorld *world);
void free_memory(ccaWorld *world, void *memory);