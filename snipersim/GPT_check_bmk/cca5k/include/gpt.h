#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define ERR -1

#define TEST 0

/* PAS region个数 */
#define PAS_REGION_COUNT 4

#define SIZE_4KB 0x1000
#define SIZE_16KB (4 * SIZE_4KB)
#define SIZE_1GB 0x40000000

#define SIZE_4GB 0x100000000
#define PPS_REGION_BASE 0x100000000
#define ULONG_MAX 0xFFFFFFFFFFFFFFFFUL
#define SZ_2M 0x00200000

#define L1_GPT_MAX_NUM 0xFFFFFF

/* GPI 定义 */
#define GPT_GPI_NO_ACCESS 0x0U
#define GPT_GPI_SECURE 0x8U
#define GPT_GPI_NS 0x9U
#define GPT_GPI_ROOT 0xAU
#define GPT_GPI_REALM 0xBU
#define GPT_GPI_ANY 0xFU
#define GPT_GPI_VAL_MASK 0xFUL

typedef enum
{
    GPI_ROOT = 0xAU,
    GPI_NS = 0x9U,
    GPI_SECURE = 0x8U,
    GPI_REALM = 0xBU,
    GPI_ANY = 0xFU,
    GPI_NO_ACCESS = 0x0U
} gpi_value;

/* GPT L0表中table与block descriptor标志，，1为Block，3为Table */
#define GPT_L0_TYPE_TBL_DESC 3UL
#define GPT_L0_TYPE_BLK_DESC 1UL
/* 构造L0表中block descriptor */
#define GPT_L0_BLK_DESC_GPI_MASK 0xFUL
#define GPT_L0_BLK_DESC_GPI_SHIFT 4UL
#define GPT_L0_BLK_DESC(_gpi) (GPT_L0_TYPE_BLK_DESC | (((_gpi) & GPT_L0_BLK_DESC_GPI_MASK) << GPT_L0_BLK_DESC_GPI_SHIFT))
/* 从L0 descriptor中获取GPI */
#define GPT_L0_BLKD_GPI(_desc) (((_desc) >> GPT_L0_BLK_DESC_GPI_SHIFT) & GPT_L0_BLK_DESC_GPI_MASK)
/* 从L0 descriptor中获取映射类型 */
#define GPT_L0_TYPE_SHIFT 0U
#define GPT_L0_TYPE_MASK 0xFUL
#define GPT_L0_TYPE(_desc) (((_desc) >> GPT_L0_TYPE_SHIFT) & GPT_L0_TYPE_MASK)

/*
 * 定义L0GPTSZ
 * 该值本来应该由寄存器GPCCR_EL3.L0GPTSZ解码读出，在这里给它手动赋值
 * 0表示1GB
 */
#define GPT_L0GPTSZ 0
/* 定义L0GPTSZ对应的s，表示位宽，根据RME Manual其大小为L0GPTSZ+30，最小为1GB也就是2^30 */
#define GPT_S_VAL (GPT_L0GPTSZ + 30U)
/*
 * L0 entry index宽度
 * 如果t<=s，L0索引PA[t-1:s]没有意义，表示一个单独的L0表映射到整片被保护物理地址，因此L0索引宽度为0
 * 如果t>s，PA[t-1:s]表示L0索引，宽度为t-s
 */
#define GPT_L0_IDX_WIDTH(_t) (((unsigned int)(_t) > GPT_S_VAL) ? ((unsigned int)(_t) - GPT_S_VAL) : (0U))
/*
 * L0 index掩码
 * t_max - s_min = 52 - 30 = 22，因此L0 index最大位宽为22
 * 0x3FFFFF为22bits，再根据实际宽度进行右移
 */
#define GPT_L0_IDX_MASK(_t) (0x3FFFFFUL >> (22U - (GPT_L0_IDX_WIDTH(_t))))
/* 物理地址中L0 index的偏移PA[t-1:s]，也就是s */
#define GPT_L0_IDX_SHIFT (GPT_S_VAL)
/* L0region总数，根据掩码+1得到 */
#define GPT_L0_REGION_COUNT(_t) ((GPT_L0_IDX_MASK(_t)) + 1U)
/* 以byte计算L0表总大小 */
#define GPT_L0_TABLE_SIZE(_t) ((GPT_L0_REGION_COUNT(_t)) << 3U)

/* L0GPTSIZE（单位byte） */
#define GPT_L0GPTSZ_ACTUAL_SIZE (1UL << GPT_S_VAL)

/* 以byte获得PPS的实际大小 */
#define GPT_PPS_ACTUAL_SIZE(_t) (1UL << (unsigned int)(_t))

/* GPT L0表中table与block descriptor标志，，1为Block，3为Table */
#define GPT_L0_TYPE_TBL_DESC 3UL
#define GPT_L0_TYPE_BLK_DESC 1UL

/* PAS Block映射为0，Granule映射为1 */
#define GPT_PAS_ATTR_MAP_TYPE_BLOCK 0x0U
#define GPT_PAS_ATTR_MAP_TYPE_GRANULE 0x1U
/* 从PAS ATTR中获取映射类型 */
#define GPT_PAS_ATTR_MAP_TYPE_SHIFT 4U
#define GPT_PAS_ATTR_MAP_TYPE_MASK 0x1U
#define GPT_PAS_ATTR_MAP_TYPE(_attrs) (((_attrs) >> GPT_PAS_ATTR_MAP_TYPE_SHIFT) & GPT_PAS_ATTR_MAP_TYPE_MASK)

/* L0 region大小为1<<s（byte） */
#define GPT_L0_REGION_SIZE (1UL << (GPT_L0_IDX_SHIFT))
/* 判断物理地址是否在L0对齐 */
#define GPT_IS_L0_ALIGNED(_pa) (((_pa) & (GPT_L0_REGION_SIZE - 1UL)) == 0UL)

/* 判断物理地址是否在L1对齐 */
#define GPT_PGS_ACTUAL_SIZE(_p) (1UL << (unsigned int)(_p))
#define GPT_IS_L1_ALIGNED(_p, _pa) (((_pa) & (GPT_PGS_ACTUAL_SIZE(_p) - 1UL)) == 0UL)

/* 根据PA获取其对应的L0页表项索引 */
#define GPT_L0_IDX(_pa) ((_pa) >> GPT_L0_IDX_SHIFT)

/* 从pas attr中获取gpi */
#define GPT_PAS_ATTR_GPI_SHIFT 0U
#define GPT_PAS_ATTR_GPI_MASK 0xFU
#define GPT_PAS_ATTR_GPI(_attrs) (((_attrs) >> GPT_PAS_ATTR_GPI_SHIFT) & GPT_PAS_ATTR_GPI_MASK)

/* 构造l0 block descriptor */
#define GPT_L0_BLK_DESC(_gpi) (GPT_L0_TYPE_BLK_DESC | \
                               (((_gpi) & GPT_L0_BLK_DESC_GPI_MASK) << GPT_L0_BLK_DESC_GPI_SHIFT))

// ==========================================================================================================================

#define GPT_L0_TBL_DESC_L1ADDR_SHIFT 4U
/* 根据L1表地址构造L0表descriptor */
#define GPT_L0_TBL_DESC(_pa) (GPT_L0_TYPE_TBL_DESC | ((uint64_t)(_pa) << GPT_L0_TBL_DESC_L1ADDR_SHIFT))
// /* 从L0表descriptor中获取L1表地址 */
#define GPT_L0_TBLD_ADDR(_desc) ((uint64_t *)(((_desc) >> GPT_L0_TBL_DESC_L1ADDR_SHIFT)))

// #define GPT_L0_TBL_DESC_L1ADDR_MASK 0xFFFFFFFFFFUL
// #define GPT_L0_TBL_DESC_L1ADDR_SHIFT 12U
// #define GPT_L0_TBL_DESC(_pa) (GPT_L0_TYPE_TBL_DESC | ((uint64_t)(_pa) & \
//                                                       (GPT_L0_TBL_DESC_L1ADDR_MASK << GPT_L0_TBL_DESC_L1ADDR_SHIFT)))
// #define GPT_L0_TBLD_ADDR(_desc) ((uint64_t *)(((_desc) & \
//                                                (GPT_L0_TBL_DESC_L1ADDR_MASK << GPT_L0_TBL_DESC_L1ADDR_SHIFT))))

/*
 * L1 index宽度
 * (s-1) - (p+3)
 */
#define GPT_L1_IDX_WIDTH(_p) ((GPT_S_VAL - 1U) - ((unsigned int)(_p) + 3U))
/*
 * L1 index mask
 * 0x7FFFFF 23bits 为L1 index最大宽度
 * ((s_max - 1) - (p_min + 4) + 1) s_max = 39（512GB）最大L0GPTSZ
 * p_min = 12（4KB granules）最小PGS.
 */
#define GPT_L1_IDX_MASK(_p) (0x7FFFFFUL >> (23U - (GPT_L1_IDX_WIDTH(_p))))
/* 每个L1表中最多表项数 */
#define GPT_L1_ENTRY_COUNT(_p) ((GPT_L1_IDX_MASK(_p)) + 1UL)
/* 以字节计算每个L1表大小 */
#define GPT_L1_TABLE_SIZE(_p) ((GPT_L1_ENTRY_COUNT(_p)) << 3U)

#define GPT_L1_GPI_BYTE(_gpi) (uint64_t)((_gpi) | ((_gpi) << 4))
#define GPT_L1_GPI_HALF(_gpi) (GPT_L1_GPI_BYTE(_gpi) | (GPT_L1_GPI_BYTE(_gpi) << 8))
#define GPT_L1_GPI_WORD(_gpi) (GPT_L1_GPI_HALF(_gpi) | (GPT_L1_GPI_HALF(_gpi) << 16))
/*
 * 根据GPI生成相同的Granules descriptor
 */
#define GPT_BUILD_L1_DESC(_gpi) (GPT_L1_GPI_WORD(_gpi) | (GPT_L1_GPI_WORD(_gpi) << 32))
#define GPT_L1_SECURE_DESC GPT_BUILD_L1_DESC(GPT_GPI_SECURE)
#define GPT_L1_NS_DESC GPT_BUILD_L1_DESC(GPT_GPI_NS)
#define GPT_L1_REALM_DESC GPT_BUILD_L1_DESC(GPT_GPI_REALM)
#define GPT_L1_ANY_DESC GPT_BUILD_L1_DESC(GPT_GPI_ANY)

/* Granule大小 */
#define GPT_PGS_ACTUAL_SIZE(_p) (1UL << (unsigned int)(_p))

/* Bit shift for the index of the L1 GPI in a PA */
#define GPT_L1_GPI_IDX_SHIFT(_p) (_p)
/* Mask for the index of the L1 GPI in a PA */
#define GPT_L1_GPI_IDX_MASK (0xF)
/* Get the index of the GPI within an L1 table entry from a physical address */
#define GPT_L1_GPI_IDX(_p, _pa) \
    (((_pa) >> GPT_L1_GPI_IDX_SHIFT(_p)) & GPT_L1_GPI_IDX_MASK)

/* Bit shift for the L1 index field */
#define GPT_L1_IDX_SHIFT(_p) ((unsigned int)(_p) + 4U)

typedef enum
{
    PPS_4GB = 0x0U,
    PPS_64GB = 0x1U,
    PPS_1TB = 0x2U,
    PPS_4TB = 0x3U,
    PPS_16TB = 0x4U,
    PPS_256TB = 0x5U,
    PPS_4PB = 0x6U,
} pps_size;

typedef enum
{
    PGS_4K = 0x0U,
    PGS_64K = 0x1U,
    PGS_16K = 0x2U
} pgs_size;

/*
 * 每个PPS大小对应一个唯一的T值
 *   PPS    Size    T
 *   0b000  4GB     32
 *   0b001  64GB    36
 *   0b010  1TB     40
 *   0b011  4TB     42
 *   0b100  16TB    44
 *   0b101  256TB   48
 *   0b110  4PB     52
 */
typedef enum
{
    PPS_4GB_T = 32U,
    PPS_64GB_T = 36U,
    PPS_1TB_T = 40U,
    PPS_4TB_T = 42U,
    PPS_16TB_T = 44U,
    PPS_256TB_T = 48U,
    PPS_4PB_T = 52U,
} gpt_t;

/*
 * 每个PGS大小对应一个唯一的P值
 *   PGS    Size    P
 *   0b00   4KB     12
 *   0b10   16KB    14
 *   0b01   64KB    16
 */
typedef enum
{
    PGS_4KB_P = 12U,
    PGS_16KB_P = 14U,
    PGS_64KB_P = 16U
} gpt_p;

void gpt_run_init();
int pas_region_init(void *pas_region_base, size_t pas_region_size, int i);
// int check_l0_gpt_param(pps_size pps);
int l0_gpt_init();
// static int check_pas_overlap(uintptr_t base_1, size_t size_1,
//                              uintptr_t base_2, size_t size_2);

// static int does_previous_pas_exist_here(unsigned int l0_idx,
//                                         pas_region_t *pas_regions,
//                                         unsigned int pas_idx);

// int l1_region_count();
// void generate_l0_blk_desc(pas_region_t *pas);
// static uint64_t *get_new_l1_tbl();
// static uintptr_t get_l1_end_pa(uintptr_t cur_pa, uintptr_t end_pa);
// static uint64_t build_l1_desc(unsigned int gpi);
// static uintptr_t fill_l1_gran_desc(uint64_t *l1, uintptr_t first,
//                                    uintptr_t last, unsigned int gpi);
// static void fill_l1_tbl(uint64_t *l1, uintptr_t first, uintptr_t last,
//                         unsigned int gpi);
// void generate_l0_tlb_desc(pas_region_t *pas);
int l1_gpt_init();
void *get_base_ptr(int cnt);
uint64_t check_pas_gpi(uintptr_t pas_address);
