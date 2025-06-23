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

void root_world_sim()
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
        }
    }

    printf("[GPT] L0 GPT initialization starting...\n");
    if (l0_gpt_init() != 0)
    {
        printf("[GPT] L0 GPT initialization failed...\n");
    }

    printf("[GPT] L1 GPT initialization starting...\n");
    if (l1_gpt_init() != 0)
    {
        printf("[GPT] L1 GPT initialization failed...\n");
    }
    return NULL;
}
// 模拟认证过程
void simulate_authentication(ccaWorld *world)
{
    printf("[CCA] Authenticating World %d...\n", world->type);
    sleep(1); // 模拟认证耗时
    printf("[CCA] Authentication successful for World %d.\n", world->type);
}

int main()
{
    root_world_sim();
    const unsigned long long GB = 1024ULL * 1024 * 1024;
    const unsigned long long max_size = 4ULL * GB;
    srand(time(NULL));
    for(int i = 0; i < 10000; i++){
    	uint64_t gpi;
    	clock_t start, end;
    	unsigned long long addr = ((unsigned long long)rand() << 32) | ((unsigned long long)rand() << 16) | (unsigned long long)rand();
    	addr %= (max_size+1);
    	
    	start = clock();
    	gpi = check_pas_gpi(addr);
    	end = clock();
    	printf("[GPT] [%d]cycle: %lf addr: %p gpi: %p\n",i,(double)(end-start),addr,gpi);
    
    }
}
