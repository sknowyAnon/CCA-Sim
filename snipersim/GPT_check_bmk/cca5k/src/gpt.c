#include "../include/gpt.h"
typedef struct pas_region
{
    uintptr_t pas_base;
    size_t size;
    unsigned int attr;
} pas_region_t;
static pas_region_t *pas_regions;

typedef struct
{
    pas_region_t *pas_region_base_array;
    size_t pas_region_count;
    uintptr_t *l0_base;
    uintptr_t *l1_base;
    size_t l0_size;
    size_t l1_size;
    pps_size pps; /* PPS */
    pgs_size pgs; /* PGS */
    int p;
    int t;
} arm_gpt_config;
static arm_gpt_config arm_gpt;

static const gpt_t gpt_t_lookup[] = {PPS_4GB_T, PPS_64GB_T,
                                     PPS_1TB_T, PPS_4TB_T,
                                     PPS_16TB_T, PPS_256TB_T,
                                     PPS_4PB_T};
static const gpt_p gpt_p_lookup[] = {PGS_4KB_P, PGS_64KB_P, PGS_16KB_P};

static const gpi_value gpi_lookup[] = {GPI_ROOT, GPI_NS, GPI_SECURE, GPI_REALM, GPI_ANY, GPI_NO_ACCESS};

static uintptr_t *l1_gpt_tbl;

static uint64_t gpt_l1_index_mask;
#define GPT_L1_INDEX(_pa) \
    (((_pa) >> (unsigned int)GPT_L1_IDX_SHIFT(arm_gpt.p)) & gpt_l1_index_mask)

static unsigned int l1_entry_count = 0;

void gpt_run_init()
{
    arm_gpt.pps = PPS_4GB;
    arm_gpt.pgs = PGS_4K;
    arm_gpt.t = gpt_t_lookup[arm_gpt.pps];
    arm_gpt.p = gpt_p_lookup[arm_gpt.pgs];
    // arm_gpt.l0_base = new uintptr_t[GPT_L0_REGION_COUNT(arm_gpt.t) * 2];
    // arm_gpt.l1_base = new uintptr_t[L1_GPT_MAX_NUM];
    arm_gpt.l0_base = (uintptr_t *)malloc(GPT_L0_REGION_COUNT(arm_gpt.t) * sizeof(uintptr_t));
    arm_gpt.l1_base = (uintptr_t *)malloc(L1_GPT_MAX_NUM * sizeof(uintptr_t));
    l1_gpt_tbl = arm_gpt.l1_base;
    // printf("%lx %lx %d", arm_gpt.l0_base, arm_gpt.l1_base, arm_gpt.l0_base - arm_gpt.l1_base);
    // pas_regions = new pas_region_t[PAS_REGION_COUNT];
    pas_regions = (pas_region_t *)malloc(L1_GPT_MAX_NUM * sizeof(pas_region_t));
    return;
}

/******************************************************************************/
/* PAS region初始化                                                            */
/******************************************************************************/
int pas_region_init(void *pas_region_base, size_t pas_region_size, int i)
{
    // printf("[GPT]Get pas base: 0x%lx\n", pas_region_base);
    // pas_regions[i].pas_base = (uintptr_t)pas_region_base + i * pas_region_size;
    pas_regions[i].pas_base = (uintptr_t)pas_region_base;
    pas_regions[i].size = pas_region_size;
    switch (i)
    {
    case 0:
        pas_regions[i].attr = GPT_PAS_ATTR_MAP_TYPE_GRANULE << 4 | GPT_GPI_ROOT;
        break;
    case 1:
        pas_regions[i].attr = GPT_PAS_ATTR_MAP_TYPE_GRANULE << 4 | GPT_GPI_NS;
        break;
    case 2:
        pas_regions[i].attr = GPT_PAS_ATTR_MAP_TYPE_GRANULE << 4 | GPT_GPI_SECURE;
        break;
    case 3:
        pas_regions[i].attr = GPT_PAS_ATTR_MAP_TYPE_GRANULE << 4 | GPT_GPI_REALM;
        break;
    default:
        break;
    }
    
    // pas_regions[i].attr = gpi_lookup[i];
    printf("[GPT] PAS[%d] region base: 0x%lx, size: 0x%lx, ATTR: 0x%x\n", i, pas_regions[i].pas_base, pas_regions[i].size, pas_regions[i].attr);
    return 0;
}

/******************************************************************************/
/* L0表初始化                                                                  */
/******************************************************************************/

int check_l0_gpt_param(pps_size pps)
{
    if (pps > 0x6U || pps < 0)
    {
        printf("[GPT] Illegal PPS value: %d\n", pps);
        return ERR;
    }

    return 0;
}

/**
 * @description: 初始化L0 table
 * @param {pps_size} pps
 * @return {*}
 */
int l0_gpt_init()
{
    if (check_l0_gpt_param(arm_gpt.pps) != 0)
    {
        printf("[GPT] L0 GPT initialization param illegal...");
        return ERR;
    }

    /* 初始L1表项全部指向GPI为ANY的Block，由此设置descriptor
     * 0001 | (1111&1111 << 4) = 1111 0001
     */
    uint64_t gpt_l0_desc = GPT_L0_BLK_DESC(GPT_GPI_ANY);

    /* 通过T和S获得L0 region个数，遍历赋值 */
    uint64_t l0_region_count = GPT_L0_REGION_COUNT(arm_gpt.t);
    // size_t l0_base_ptr[l0_region_count];
    for (size_t i = 0; i < l0_region_count; i++)
    {
        arm_gpt.l0_base[i] = gpt_l0_desc;
    }
    // arm_gpt.l0_base = l0_base_ptr;
    printf("[GPT] L0 table initialized\n");
    printf("      L0 region number: %llu\n", l0_region_count);
    printf("      L0 descriptor: 0x%llx\n", gpt_l0_desc);
    printf("      L0 table base address: 0x%lx\n", arm_gpt.l0_base);
    return 0;
}

/******************************************************************************/
/* L1表初始化                                                                  */
/******************************************************************************/
static int check_pas_overlap(uintptr_t base_1, size_t size_1,
                             uintptr_t base_2, size_t size_2)
{
    if (((base_1 + size_1) > base_2) && ((base_2 + size_2) > base_1))
    {
        return 1;
    }
    return 0;
}

static int does_previous_pas_exist_here(unsigned int l0_idx,
                                        pas_region_t *pas_regions,
                                        unsigned int pas_idx)
{
    for (unsigned int i = 0U; i < pas_idx; i++)
    {
        if (check_pas_overlap((GPT_L0GPTSZ_ACTUAL_SIZE * l0_idx), GPT_L0GPTSZ_ACTUAL_SIZE,
                              pas_regions[i].pas_base, pas_regions[i].size))
        {
#if TEST
            printf("%d pas overlap, \n", i);
            unsigned long size = (GPT_L0GPTSZ_ACTUAL_SIZE * l0_idx);
            printf("l0 idx: %lx \n", l0_idx);
            printf("l0gptszbase[%d]: %lx ;l0gptsz[%d]: %lx\n", i, size, i, GPT_L0GPTSZ_ACTUAL_SIZE);
            size += GPT_L0GPTSZ_ACTUAL_SIZE;
            printf("actual+actualsize = %lx\n", size);
            printf("pas base[%d]: %lx ; pas size[%d]: %lx\n", i, pas_regions[i].pas_base, i, pas_regions[i].size);
            size = pas_regions[i].size + pas_regions[i].pas_base;
            printf("pas+passize = %lx\n", size);
#endif
            return 1;
        }
    }
    return 0;
}

int l1_region_count()
{
    /* 所有pas region所需的L1表总数 */
    unsigned long l1_cnt = 0;
    unsigned long current_l1_cnt = 0;
    for (unsigned int idx = 0U; idx < PAS_REGION_COUNT; idx++)
    {
#if TEST
        printf("================%d====================\n", idx);
#endif
        /* 检查最大地址是否越界 */
   
        if ((ULONG_MAX - pas_regions[idx].pas_base) < pas_regions[idx].size)
        {
            printf("[GPT] Address overflow in PAS[%u]\n", idx);
            return ERR;
        }
        /* 检查PAS是否超过GPT_PPS_ACTUAL_SIZE */
        if ((pas_regions[idx].pas_base + pas_regions[idx].size) > GPT_PPS_ACTUAL_SIZE(arm_gpt.t))
        {
            printf("%lx %lx\n",pas_regions[idx].pas_base + pas_regions[idx].size,GPT_PPS_ACTUAL_SIZE(arm_gpt.t));
            printf("[GPT] PAS[%u] size invalid\n", idx);
            return ERR;
        }
        /* 检查PAS之间是否重叠，将当前idx的pas region与所有idx在其后面的pas region比较 */
 
        for (unsigned int i = idx + 1U; i < PAS_REGION_COUNT; i++)
        {
            if (check_pas_overlap(pas_regions[idx].pas_base, pas_regions[idx].size,
                                  pas_regions[i].pas_base, pas_regions[i].size))
            {
                printf("[GPT] PAS[%u] overlaps with PAS[%u]\n", idx, i);
                return ERR;
            }
        }

        for (unsigned int i = (unsigned int)GPT_L0_IDX(pas_regions[idx].pas_base);
             i <= (unsigned int)GPT_L0_IDX(pas_regions[idx].pas_base + pas_regions[idx].size - 1UL);
             i++)
        {
            
            /* 未映射过，则continue检查下一个索引 */
            if ((GPT_L0_TYPE(arm_gpt.l0_base[i]) == GPT_L0_TYPE_BLK_DESC) && (GPT_L0_BLKD_GPI(arm_gpt.l0_base[i]) == GPT_GPI_ANY))
            {
                continue;
            }
            printf("[GPT] PAS[%u] overlaps with previous L0[%u]!\n", idx, i);
            return ERR;
        }

        /*
         * 检查PA类型是否为block（L0映射），如果是，则检查base和size是否block-aligned
         * attrs>>4 & 0x1，block映射结果为0
         */

        if (GPT_PAS_ATTR_MAP_TYPE(pas_regions[idx].attr) == GPT_PAS_ATTR_MAP_TYPE_BLOCK)
        {
            if (!GPT_IS_L0_ALIGNED(pas_regions[idx].pas_base) ||
                !GPT_IS_L0_ALIGNED(pas_regions[idx].size))
            {
                printf("[GPT] PAS[%u] is not block-aligned\n", idx);
                return ERR;
            }
            continue;
        }

        /*
         * 检查PA类型是否为granule（L1映射），如果是，则检查base和size是否granule-aligned
         * 并且计算这块pas占多少L1表
         */
  
        if (GPT_PAS_ATTR_MAP_TYPE(pas_regions[idx].attr) == GPT_PAS_ATTR_MAP_TYPE_GRANULE)
        {
            /* 检查base和size是否granule-aligned */
            if (!GPT_IS_L1_ALIGNED(arm_gpt.p, pas_regions[idx].pas_base) ||
                !GPT_IS_L1_ALIGNED(arm_gpt.p, pas_regions[idx].size))
            {
                printf("[GPT] PAS[%u] is not granule-aligned\n", idx);
                return ERR;
            }

            current_l1_cnt = (GPT_L0_IDX(pas_regions[idx].pas_base + pas_regions[idx].size - 1UL) -
                              GPT_L0_IDX(pas_regions[idx].pas_base) + 1U);
#if TEST
            uintptr_t end_l0_idx = GPT_L0_IDX(pas_regions[idx].pas_base + pas_regions[idx].size - 1UL);
            uintptr_t starting_l0_index = GPT_L0_IDX(pas_regions[idx].pas_base);
            printf("end l0 index: %lx   start l0 index: %lx   current_l1_cnt: %d\n", end_l0_idx, starting_l0_index, current_l1_cnt);
#endif
            if (current_l1_cnt > 1)
            {
                if (does_previous_pas_exist_here(GPT_L0_IDX(pas_regions[idx].pas_base + pas_regions[idx].size - 1UL),
                                                 pas_regions, idx))
                {
                    current_l1_cnt--;
                }
            }
            if (does_previous_pas_exist_here(GPT_L0_IDX(pas_regions[idx].pas_base), pas_regions, idx))
            {
                current_l1_cnt--;
            }
             //printf("cur: %d\n", current_l1_cnt);
        }

        l1_cnt += current_l1_cnt;
        
#if TEST
        printf("current[%d] cnt: 0x%lx\n", idx, current_l1_cnt);
#endif
        continue;
    }
#if TEST
    printf("total cnt: 0x%lx\n", l1_cnt);
#endif
    return l1_cnt;
}

void generate_l0_blk_desc(pas_region_t *pas)
{
    uintptr_t *l0_gpt_arr = arm_gpt.l0_base;
    unsigned long gpt_desc = GPT_L0_BLK_DESC(GPT_PAS_ATTR_GPI(pas->attr));
    unsigned long idx = GPT_L0_IDX(pas->pas_base);
    unsigned long end_idx = GPT_L0_IDX(pas->pas_base + pas->size);
    for (; idx < end_idx; idx++)
    {
        l0_gpt_arr[idx] = gpt_desc;
        printf("[GPT] L0 entry (BLOCK) index[%lu] Addr: %p GPI: 0x%lx Desc: 0x%lx\n",
               idx, &l0_gpt_arr[idx],
               (gpt_desc >> GPT_L0_BLK_DESC_GPI_SHIFT) &
                   GPT_L0_BLK_DESC_GPI_MASK,
               l0_gpt_arr[idx]);
    }
    return;
}

static uint64_t *get_new_l1_tbl()
{
    /* 找到下一个可用的L1表 */
    uint64_t *l1 = (uint64_t *)l1_gpt_tbl;

    l1_gpt_tbl += GPT_L1_TABLE_SIZE(arm_gpt.p);

#if TEST
    printf("0x%lx 0x%lx\n", GPT_L1_ENTRY_COUNT(arm_gpt.p), GPT_L1_TABLE_SIZE(arm_gpt.p));
#endif
    /* 所有GPI初始化为GPT_GPI_ANY */
    for (unsigned int i = 0U; i < GPT_L1_ENTRY_COUNT(arm_gpt.p); i++)
    {
        l1[i] = GPT_L1_ANY_DESC;
    }
    return l1;
}

static uintptr_t get_l1_end_pa(uintptr_t cur_pa, uintptr_t end_pa)
{
    uintptr_t cur_idx;
    uintptr_t end_idx;

    cur_idx = GPT_L0_IDX(cur_pa);
    end_idx = GPT_L0_IDX(end_pa);
    /* 当前地址的l0 index为最后一块地址的l0 index */
    if (cur_idx == end_idx)
    {
        return end_pa;
    }
    return (cur_idx + 1UL) << GPT_L0_IDX_SHIFT;
}

static uint64_t build_l1_desc(unsigned int gpi)
{
    uint64_t l1_desc = (uint64_t)gpi | ((uint64_t)gpi << 4);

    l1_desc |= (l1_desc << 8);
    l1_desc |= (l1_desc << 16);
    return (l1_desc | (l1_desc << 32));
}

static uintptr_t fill_l1_gran_desc(uint64_t *l1, uintptr_t first,
                                   uintptr_t last, unsigned int gpi)
{
    uint64_t l1_desc = build_l1_desc(gpi);
    // printf("l1_desc: %p\n", l1_desc);
    uint64_t gpi_mask = ULONG_MAX << (GPT_L1_GPI_IDX(arm_gpt.p, first) << 2);

    /* Fill out each L1 entry for this region */
    for (unsigned long i = GPT_L1_INDEX(first); i <= GPT_L1_INDEX(last); i++)
    {
        /* Account for stopping in the middle of an L1 entry */
        if (i == GPT_L1_INDEX(last))
        {
            gpi_mask &= (gpi_mask >> ((15U - GPT_L1_GPI_IDX(arm_gpt.p, last)) << 2));
        }

        /* Write GPI values */
        l1[i] = (l1[i] & ~gpi_mask) | (l1_desc & gpi_mask);

        l1_entry_count++;
        /* Reset mask */
        gpi_mask = ULONG_MAX;
    }
    return last + GPT_PGS_ACTUAL_SIZE(arm_gpt.p);
}

static void fill_l1_tbl(uint64_t *l1, uintptr_t first, uintptr_t last,
                        unsigned int gpi)
{
    // printf("%p %p\n", first, last);
    // while (first <= last)
    // {
    //     printf("ok3\n");
    //     size_t length = last - first + GPT_PGS_ACTUAL_SIZE(arm_gpt.p);
    //     if (length < SZ_2M)
    //     {
    //         printf("length: %d\n", length);
    //         // first = fill_l1_gran_desc(l1, first, last, gpi);
    //     }
    // }
    first = fill_l1_gran_desc(l1, first, last, gpi);
    return;
}

void generate_l0_tlb_desc(pas_region_t *pas)
{
    uintptr_t end_pa = pas->pas_base + pas->size;
    uintptr_t *l0_gpt_base = arm_gpt.l0_base;
    unsigned long gpi = GPT_PAS_ATTR_GPI(pas->attr);
    uint64_t *l1_gpt_arr;
    uintptr_t last_gran_pa;

    uintptr_t cur_pa = pas->pas_base;

    for (unsigned int l0_idx = (unsigned int)GPT_L0_IDX(pas->pas_base);
         l0_idx <= (unsigned int)GPT_L0_IDX(end_pa - 1UL);
         l0_idx++)
    {
        if (GPT_L0_TYPE(l0_gpt_base[l0_idx]) == GPT_L0_TYPE_TBL_DESC)
        {
            l1_gpt_arr = GPT_L0_TBLD_ADDR(l0_gpt_base[l0_idx]);
            // l1_gpt_arr = (uint64_t *)l0_gpt_base[l0_idx];
        }
        else
        {
            /* 获取可使用的L1表地址，并对其进行初始化 */
            l1_gpt_arr = get_new_l1_tbl();

            /* 根据L1表地址构造L0表descriptor */
            l0_gpt_base[l0_idx] = GPT_L0_TBL_DESC(l1_gpt_arr);
            // l0_gpt_base[l0_idx] = (uintptr_t)l1_gpt_arr;
#if TEST
            printf("l1 arr addr: %p\n", l1_gpt_arr);
            printf("content: %p\n", l0_gpt_base[l0_idx]);
            l1_gpt_arr = GPT_L0_TBLD_ADDR(l0_gpt_base[l0_idx]);
            printf("get arr addr: %p\n\n", l1_gpt_arr);
#endif
        }
        // uint64_t *addr = GPT_L0_TBLD_ADDR(l0_gpt_base[l0_idx]);
        printf("[GPT] L0 entry (TABLE) index[%u] Addr: %p ==> L1 Addr: %p L0 TLB DESC: %p\n",
               l0_idx, &l0_gpt_base[l0_idx], l1_gpt_arr, l0_gpt_base[l0_idx]);
        

        /*
         * Determine the PA of the last granule in this L0 descriptor.
         */
        last_gran_pa = get_l1_end_pa(cur_pa, end_pa) - GPT_PGS_ACTUAL_SIZE(arm_gpt.p);

        fill_l1_tbl(l1_gpt_arr, cur_pa, last_gran_pa, gpi);

        cur_pa = get_l1_end_pa(cur_pa, end_pa);

    }
}

int l1_gpt_init()
{
    unsigned long l1_region_cnt = l1_region_count();
    //unsigned long l1_region_cnt = 0xFFFFFF;
    if (l1_region_cnt < 0)
    {
        printf("[GPT] L1 region number error: %lu\n", l1_region_cnt);
        return ERR;
    }
    printf("[GPT] Total L1 table count: 0x%lx\n", l1_region_cnt);

    gpt_l1_index_mask = GPT_L1_IDX_MASK(arm_gpt.p);

    printf("[GPT]========== GPT Configuration ==========\n");
    printf("     PPS/T:            0x%x/%u\n", arm_gpt.pps, arm_gpt.t);
    printf("     PGS/P:            0x%x/%u\n", arm_gpt.pgs, arm_gpt.p);
    printf("     L0GPTSZ/S:        0x%x/%u\n", GPT_L0GPTSZ, GPT_S_VAL);
    printf("     PAS region count: %u\n", PAS_REGION_COUNT);
    printf("     L0 base:          0x%lx\n", arm_gpt.l0_base);

    /* 构造table */
    for (unsigned int idx = 0U; idx < PAS_REGION_COUNT; idx++)
    {

        if (GPT_PAS_ATTR_MAP_TYPE(pas_regions[idx].attr) ==
            GPT_PAS_ATTR_MAP_TYPE_BLOCK)
        {
            printf("[GPT] ========== GPT L0 BLOCK Desc Generating ==========\n");
            printf("[GPT] PAS[%d] base: 0x%lx size: 0x%lx GPI: 0x%lx MapType: 0x%lx\n", idx, pas_regions[idx].pas_base, pas_regions[idx].size, GPT_PAS_ATTR_GPI(pas_regions[idx].attr), GPT_PAS_ATTR_MAP_TYPE(pas_regions[idx].attr));
            generate_l0_blk_desc(&pas_regions[idx]);
        }
        else
        {
            printf("[GPT] ========== GPT L0 TABLE Desc Generating ==========\n");
            printf("[GPT] PAS[%d] base: 0x%lx size: 0x%lx GPI: 0x%lx MapType: 0x%lx\n", idx, pas_regions[idx].pas_base, pas_regions[idx].size, GPT_PAS_ATTR_GPI(pas_regions[idx].attr), GPT_PAS_ATTR_MAP_TYPE(pas_regions[idx].attr));
            generate_l0_tlb_desc(&pas_regions[idx]);
        }
    }

    return 0;
}
void* get_base_ptr(int cnt){
    void* ptr = (void*)0;
    for(int i = 0; i < cnt; i++){
        ptr += 0x40000000;
    }
    return ptr;
}

uint64_t check_pas_gpi(uintptr_t pas_address)
{
    unsigned int l0_index = (unsigned int)GPT_L0_IDX(pas_address);
    unsigned int l1_index = (unsigned int)GPT_L1_INDEX(pas_address);
    uintptr_t l0_addr = arm_gpt.l0_base[l0_index];
    uint64_t *l1_addr = GPT_L0_TBLD_ADDR(l0_addr);

    // printf("l0 index: %p l0 addr: %p l1 addr: %p l1 content: %p\n", l0_index, arm_gpt.l0_base[l0_index], l1_addr, l1_addr[l1_index]);
    return l1_addr[l1_index];
}

// int main()
// {
//     printf("[CCA] Memory Allocation starting...\n");
//     printf("[CCA] Attestation starting...\n");
//     printf("[CCA] Root World is running.\n");
//     printf("[CCA] Normal World is running.\n");
//     printf("[CCA] Secure World is running.\n");
//     printf("[CCA] Realm World is running.\n");
//     run_init();
//     printf("[GPT] PAS region initialization starting...\n");
//     if (pas_region_init(PPS_REGION_BASE, SIZE_4GB) != 0)
//     {
//         printf("[GPT] PAS region initialization failed...\n");
//         return 0;
//     }
//     printf("[GPT] L0 GPT initialization starting...\n");
//     if (l0_gpt_init() != 0)
//     {
//         printf("[GPT] L0 GPT initialization failed...\n");
//         return 0;
//     }

//     printf("[GPT] L1 GPT initialization starting...\n");
//     if (l1_gpt_init() != 0)
//     {
//         printf("[GPT] L1 GPT initialization failed...\n");
//         return 0;
//     }
//     printf("l1 entry count: %d\n", l1_entry_count);

//     pid_t pid = fork();

//     if (pid == 0)
//     {
//         // 子进程：执行 Shell 脚本
//         execl("/bin/sh", "sh", "./script.sh", (char *)NULL);
//         printf("Failed to execute the script.\n");
//         return 1;
//     }
//     else if (pid > 0)
//     {
//         // 父进程：等待子进程结束
//         int status;
//         waitpid(pid, &status, 0);
//         printf("Script executed with return code: %d\n", WEXITSTATUS(status));
//     }
//     else
//     {
//         printf("Failed to fork.\n");
//         return 1;
//     }

//     // srand(time(NULL));
//     // for (int i = 0; i < 2; i++)
//     // {
//     //     uint64_t gpi;
//     //     clock_t start, end;
//     //     uintptr_t addr = PPS_REGION_BASE + rand() % SIZE_4GB;
//     //     start = clock();
//     //     gpi = check_pas_gpi(addr);
//     //     end = clock();
//     //     printf("[GPT] [%d]cycle: %lf addr: %p gpi: %p\n", i, (double)(end - start), addr, gpi);
//     // }
//     return 0;
// }
