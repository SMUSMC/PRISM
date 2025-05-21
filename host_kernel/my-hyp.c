// prism hyp part functions
unsigned long VAToPA_EL1(unsigned long va);
unsigned long VAToPA_EL1_S2(unsigned long va);
unsigned long getHCR_EL2(void);
unsigned long getTTBR0_EL2(void);
unsigned long VAToPA_EL2(unsigned long addr);
unsigned long VAToPA_EL2_W(unsigned long addr);
void DrawLock(unsigned char* vaddr, int x, int y, int locked) ;
void DrawString(unsigned char* vaddr, int x, int y, char* draw_str);



#if 0
	drmdecon0: drmdecon@0x1C240000 {
		compatible = "samsung,exynos-decon";
		reg = <0x0 0x1C240000 0x6000>,	/* DECON0_MAIN */
		      <0x0 0x1C250000 0x6000>,	/* DECON_WIN */
		      <0x0 0x1C260000 0x10000>,	/* DECON_SUB */
		      <0x0 0x1C270000 0x6000>,	/* DECON0_WINCON */
		      <0x0 0x1C2A0000 0xF000>,	/* DQE0 */
		      <0x0 0x1C0BE000 0x1000>;	/* CGC_DMA */
#endif

//
//	we maitain a pa <-> va mapping list, which we create mapping from pa.
//	so next time we access the same pa we do need to mapping it again.
//

#define MAX_PROTECTED_REG_PA (16)
#define MAX_PROTECTED_FB_PA (4096)
phys_addr_t protectPaList[MAX_PROTECTED_REG_PA] = {0};
phys_addr_t protectfbPaList[MAX_PROTECTED_FB_PA] = {0};


typedef struct mapped_pages_
{
	unsigned long pa;
	unsigned long va_el2;
} mapped_pages;

static mapped_pages mPageArray[MAX_PROTECTED_FB_PA+MAX_PROTECTED_REG_PA] ;
static arraysize = MAX_PROTECTED_FB_PA+MAX_PROTECTED_REG_PA;
unsigned long * map_fb_to_el2(phys_addr_t *fb_list, int cnt) ;
unsigned long* map_l1_va (void) ;
static unsigned long* fb_va_el2 = NULL ;

void *find_in_map_by_pa(phys_addr_t pa)
{
	int i = 0;
	void *va_el2;
	unsigned int offset = pa & 0xfff;

	pa &= (~0xff00000000000fffUL);

	while (i < arraysize)
	{
		if (mPageArray[i].pa == 0)
			break;

		if (mPageArray[i].pa == pa)
		{
			if (mPageArray[i].va_el2 != 0)
				return (void *)(mPageArray[i].va_el2 | offset);
		}
		i++;
	}

	if (i < arraysize)
	{
		va_el2 = (void *)__pkvm_create_private_mapping(pa,
			0x1000, KVM_PGTABLE_PROT_DEVICE | KVM_PGTABLE_PROT_R | KVM_PGTABLE_PROT_W);

		if (va_el2)
		{
			mPageArray[i].pa = pa;
			mPageArray[i].va_el2 = (unsigned long)va_el2;
			return (void *)((unsigned long)va_el2 | offset);
		}
	}

	return NULL;
}

static unsigned int phy_readl(phys_addr_t pa)
{
	unsigned int *va_el2 = (unsigned int *)find_in_map_by_pa(pa);
	if (va_el2)
		return readl(va_el2);
	else
		return 0;
}
//
// registers' defination come from:
// kernel/private/gs-google/arch/arm64/boot/dts/google/gs201-drm-dpu.dtsi
//
#define WIN_OFFSET(id) (0x1000 * (id))
// decon win registers.
#define DECON_WIN_START (0x1C250000UL)
#define DECON_WIN_END (0x1C256000UL)
#define DECON_WIN (0x1000UL)
#define DECON_WIN_FUNC_CON_0(id) (DECON_WIN_START + WIN_OFFSET(id) + 0x4) // alpha blending related
#define DECON_WIN_FUNC_CON_1(id) (DECON_WIN_START + WIN_OFFSET(id) + 0x8) // alpha blending related
#define DECON_WIN_START_POSITION(id) (DECON_WIN_START + WIN_OFFSET(id) + 0xC)
#define DECON_WIN_END_POSITION(id) (DECON_WIN_START + WIN_OFFSET(id) + 0x10)

#define WIN_STRPTR_Y_F(_v) (((_v) & 0x3FFF) << 16)
#define WIN_STRPTR_X_F(_v) (((_v) & 0x3FFF) << 0)

#define WIN_ENDPTR_Y_F(_v) (((_v) & 0x3FFF) << 16)
#define WIN_ENDPTR_X_F(_v) (((_v) & 0x3FFF) << 0)

// decon win control registers.
#define WIN_CONFIG_START (0x1C270000UL)
#define WIN_CONFIG_END (0x1C276000UL)
#define WIN_CONFIG_SIZE (0x1000UL) // Size of single win control reg set !!!

#define WIN_CHMAP_MASK (0x7 << 4)

#define WIN_CONFIG(id) (WIN_CONFIG_START + WIN_CONFIG_SIZE * (id))

// kernel/private/google-modules/display/samsung/cal_9845/regs-dpp.h
// DPP DPP registers.
#define DPP_DPP_REG_START (0x1C0D0000UL)
#define DPP_DPP_REG_END (0x1C0D6000UL)
#define DPP_DPP_SIZE (0x1000UL) // Size of single DPP DPP reg set !!!

// this id should with in 0~5
#define DPP_DPP(id) (DPP_DPP_REG_START + DPP_DPP_SIZE * (id))
#define TO_DPP_DPP_ID(addr) ((addr - DPP_DPP_REG_START) / DPP_DPP_SIZE)

#define DPP_COM_IMG_SIZE (0x003C)
#define DPP_IMG_HEIGHT(_v) ((_v) << 16)
#define DPP_IMG_HEIGHT_MASK (0x3FFF << 16)
#define DPP_IMG_WIDTH(_v) ((_v) << 0)
#define DPP_IMG_WIDTH_MASK (0x3FFF << 0)

#define DPP_SCL_SCALED_IMG_SIZE (0x0200)
#define DPP_SCALED_IMG_HEIGHT(_v) ((_v) << 16)
#define DPP_SCALED_IMG_HEIGHT_MASK (0x3FFF << 16)
#define DPP_SCALED_IMG_WIDTH(_v) ((_v) << 0)
#define DPP_SCALED_IMG_WIDTH_MASK (0x3FFF << 0)

#define RDMA_IN_CTRL_0 (0x0008)
#define IDMA_ALPHA(_v) ((_v) << 24)
#define IDMA_ALPHA_MASK (0xFF << 24)
#define IDMA_IC_MAX(_v) ((_v) << 16)
#define IDMA_IC_MAX_MASK (0xFF << 16)
#define IDMA_SBWC_LOSSY (1 << 14)
#define IDMA_IMG_FORMAT(_v) ((_v) << 8)
#define IDMA_IMG_FORMAT_MASK (0x3F << 8)
#define IDMA_ROT(_v) ((_v) << 4)
#define IDMA_ROT_MASK (0x7 << 4)

// DPP DMA registers.
#define DPP_DMA_REG_START (0x1C0B0000UL)
#define DPP_DMA_REG_END (0x1C0B6000UL)
#define DPP_DMA_SIZE (0x1000UL) // Size of single DPP DMA reg set !!!

// this id should with in 0~5
#define DPP_DMA(id) (DPP_DMA_REG_START + DPP_DMA_SIZE * (id))
#define TO_DPP_DMA_ID(addr) ((addr - DPP_DMA_REG_START) / DPP_DMA_SIZE)
#define RDMA_BASEADDR_Y8 (0x0040)
#define RDMA_BASEADDR_C8 (0x0044)

#define WIN5_SRC_W 64
#define WIN5_SRC_H 64

// #define WIN5_SRC_W 1088
// #define WIN5_SRC_H 2400

#define WIN5_DEST_W WIN5_SRC_W
#define WIN5_DEST_H WIN5_SRC_H

struct decon_frame
{
	int x;
	int y;
	unsigned int w;
	unsigned int h;
	unsigned int f_w;
	unsigned int f_h;
};

typedef struct drm_config_
{
	unsigned int dma_iova;

	struct decon_frame src;
	struct decon_frame dst;

	unsigned int rot;

	int h_ratio;
	int v_ratio;
} drm_config;

static drm_config drm_config_hyp = {
	0, // dma_iova
	{
		0,			// win_config_src_x
		0,			// win_config_src_y
		WIN5_SRC_W, // win_config_src_w
		WIN5_SRC_H, // win_config_src_h
		1088,		// win_config_src_f_w
		WIN5_SRC_H, // win_config_src_f_h
	},
	{
		0,			 // win_config_dst_x
		0,			 // win_config_dst_y
		WIN5_DEST_W, // win_config_dst_w
		WIN5_DEST_H, // win_config_dst_h
		1088,		 // win_config_dst_f_w
		WIN5_DEST_H, // win_config_dst_f_h
	},
	0, // win_config_rot
	0, // h_ration
	0, // v_ration
};

#define RDMA_SRC_SIZE (0x0010)
#define IDMA_SRC_HEIGHT(_v) ((_v) << 16)
#define IDMA_SRC_HEIGHT_MASK (0xFFFF << 16)
#define IDMA_SRC_WIDTH(_v) ((_v) << 0)
#define IDMA_SRC_WIDTH_MASK (0xFFFF << 0)

#define RDMA_SRC_OFFSET (0x0014)
#define IDMA_SRC_OFFSET_Y(_v) ((_v) << 16)
#define IDMA_SRC_OFFSET_Y_MASK (0x3FFF << 16)
#define IDMA_SRC_OFFSET_X(_v) ((_v) << 0)
#define IDMA_SRC_OFFSET_X_MASK (0x3FFF << 0)

#define RDMA_IMG_SIZE (0x0018)
#define IDMA_IMG_HEIGHT(_v) ((_v) << 16)
#define IDMA_IMG_HEIGHT_MASK (0x3FFF << 16)
#define IDMA_IMG_WIDTH(_v) ((_v) << 0)
#define IDMA_IMG_WIDTH_MASK (0x3FFF << 0)

#define TO_DPP_ID(pa) ((pa & 0xF000) / (0x1000))
static unsigned long testvalu = 0x55aaaa55UL;
static unsigned int start_protect = 0;
static unsigned long WriteWithCheck(unsigned int *va_el2, unsigned int val, unsigned long pa)
{

	unsigned long ret = 0;

	if (start_protect)
	{
		switch (pa)
		{
		case WIN_CONFIG(5):
			// val = phy_readl(WIN_CONFIG(5)) | 1 ;
			val = val | 1;
			break;

		case DECON_WIN_FUNC_CON_0(5):
			val = 0x00ff0d02;
			break;

		case DECON_WIN_FUNC_CON_1(5):
			val = 0x060b060b;
			break;

		case DECON_WIN_START_POSITION(5):
			val = (WIN_STRPTR_Y_F(0) | WIN_STRPTR_X_F(0));
			break;

		case DECON_WIN_END_POSITION(5):
			val = (WIN_ENDPTR_Y_F(drm_config_hyp.src.h - 1) | WIN_ENDPTR_X_F(drm_config_hyp.src.w - 1));
			break;

		default:
		{

			if ((pa > DPP_DMA_REG_START) && (pa < DPP_DMA_REG_END))
			{
				unsigned int dpp_id = TO_DPP_ID(pa);
				unsigned int win_dpp = phy_readl(WIN_CONFIG(5));
				win_dpp = (win_dpp & WIN_CHMAP_MASK) >> 4;

				if (dpp_id == win_dpp)
				{
					// dpp dma registers;
					if (((pa & 0xfff) == RDMA_BASEADDR_Y8) || ((pa & 0xfff) == RDMA_BASEADDR_C8))
					{
						// don't over write my frame buffer address.
						val = drm_config_hyp.dma_iova;
					}
					else if ((pa & 0xfff) == RDMA_SRC_OFFSET)
					{
						// don't over write my source offset
						val = IDMA_SRC_OFFSET_Y(drm_config_hyp.src.y) | IDMA_SRC_OFFSET_X(drm_config_hyp.src.x);
					}
					else if ((pa & 0xfff) == RDMA_SRC_SIZE)
					{
						// don't over write my source size
						val = IDMA_SRC_HEIGHT(drm_config_hyp.src.f_h) | IDMA_SRC_WIDTH(drm_config_hyp.src.f_w);
					}
					else if ((pa & 0xfff) == RDMA_IMG_SIZE)
					{
						// don't over write my image size
						val = IDMA_IMG_HEIGHT(drm_config_hyp.src.h) | IDMA_IMG_WIDTH(drm_config_hyp.src.w);
					}
					else if ((pa & 0xfff) == RDMA_IN_CTRL_0)
					{

						val = (val & (~IDMA_ROT_MASK)) | (IDMA_ROT(drm_config_hyp.rot) & IDMA_ROT_MASK);
					}
				}
			}
			else if ((pa > DPP_DPP_REG_START) && (pa < DPP_DPP_REG_END))
			{
				// dpp dpp registers;
				unsigned int dpp_id = TO_DPP_ID(pa);
				unsigned int win_dpp = phy_readl(WIN_CONFIG(5));
				win_dpp = (win_dpp & WIN_CHMAP_MASK) >> 4;

				if (dpp_id == win_dpp)
				{
					if ((pa & 0xfff) == DPP_COM_IMG_SIZE)
					{

						val = DPP_IMG_HEIGHT(drm_config_hyp.src.h) | DPP_IMG_WIDTH(drm_config_hyp.src.w);
					}
					else if ((pa & 0xfff) == DPP_SCL_SCALED_IMG_SIZE)
					{

						val = DPP_SCALED_IMG_HEIGHT(drm_config_hyp.src.h) | DPP_SCALED_IMG_WIDTH(drm_config_hyp.src.w);
					}
				}
			}
		}
		}
	}
	ret = (unsigned long)val << 32 | pa;
	writel(val, va_el2);
	return ret;
}

static void handle___hvc_write_memory(struct kvm_cpu_context *host_ctxt)
{
	DECLARE_REG(unsigned long, va_el1, host_ctxt, 1);
	DECLARE_REG(unsigned long, val, host_ctxt, 2);
	DECLARE_REG(unsigned long, size, host_ctxt, 3);

	size = size & 0xFF;
	if (size == sizeof(unsigned int))
	{
		phys_addr_t pa = VAToPA_EL1_S2(va_el1);
		//		phys_addr_t ipa = VAToPA_EL1 (va_el1) ;
		unsigned int *va_el2 = NULL;

		if ((pa & 0x1) != 0x1)
		{
			pa &= (~0xff00000000000fffUL);
			pa = pa | (va_el1 & 0xfff);

			va_el2 = (unsigned int *)find_in_map_by_pa(pa);

			if (va_el2)
			{
				unsigned long ret = 0;
				ret = WriteWithCheck(va_el2, val, pa);
				cpu_reg(host_ctxt, 1) = ret;
			}
			else
			{
				cpu_reg(host_ctxt, 1) = (unsigned long)-2;
			}
		}
		else
		{
			cpu_reg(host_ctxt, 1) = (unsigned long)-1;
		}
		// confirm stage-2 is enabled
		// cpu_reg(host_ctxt, 1) = getHCR_EL2() ;
	}
	else
	{
		cpu_reg(host_ctxt, 1) = ((unsigned long)(val << 8) | size);
	}
}


static void handle___hvc_create_map_space(struct kvm_cpu_context *host_ctxt)
{
	memset ((void*)mPageArray, 0, sizeof(mPageArray)) ;

	cpu_reg(host_ctxt, 1) = 0;
}

extern int host_stage2_idmap_locked(phys_addr_t addr, u64 size,
									enum kvm_pgtable_prot prot, bool update_iommu);
extern unsigned long getVTTBR_EL2(void);									

phys_addr_t AddtoProtectList(phys_addr_t pa)
{
	int i = 0;

	if (protectPaList[0] == (phys_addr_t)0)
		memset((void *)protectPaList, 0, sizeof(protectPaList));

	while (i < MAX_PROTECTED_REG_PA)
	{
		if (protectPaList[i] == pa)
			return 0;
		if (protectPaList[i] == (phys_addr_t)0)
		{
			protectPaList[i] = pa;
			return protectPaList[i];
		}
		i++;
	}
	return (phys_addr_t)-1;
}

phys_addr_t findPaInprotectList(phys_addr_t va_el1)
{
	int i = 0;
	phys_addr_t pa;

	pa = VAToPA_EL1_S2(va_el1);
	pa = pa & (~0xff00000000000fffUL);

	while (i < MAX_PROTECTED_REG_PA)
	{
		if (protectPaList[i] == pa)
			return pa;
		if (protectPaList[i] == (phys_addr_t)0)
		{
			break;
		}
		i++;
	}
	return (phys_addr_t)0;
}

int DataAbortWriteWithCheck(struct kvm_cpu_context *host_ctxt, unsigned long esr, unsigned long far_el2)
{

	u32 reg_num = (esr >> 16) & 0x1F; 

	phys_addr_t pa;
	unsigned int *va_el2 = NULL;
	unsigned int val = (reg_num == 0x1f) ? 0 : cpu_reg(host_ctxt, reg_num);

	pa = VAToPA_EL1_S2(far_el2);
	pa = pa & (~0xff00000000000fffUL);
	pa = pa | (far_el2 & 0xfff);

	if ((pa < 0x1C000000UL) || pa >= 0x1D000000UL)
	{
		// should be our frame buffer.
		testvalu = pa;
		return -1;
	}

	va_el2 = (unsigned int *)find_in_map_by_pa(pa);

	if (va_el2)
	{
		WriteWithCheck(va_el2, val, pa);
	}

	return 0;
}
static int protectFBCount = 0 ;

phys_addr_t is_FB(phys_addr_t va_el1)
{
	int i = 0;
	phys_addr_t pa;

	pa = VAToPA_EL1_S2(va_el1);
	pa = pa & (~0xff00000000000fffUL);

	while (i < protectFBCount)
	{
		if (protectfbPaList[i] == pa)
			return pa;
		if (protectfbPaList[i] == (phys_addr_t)0)
		{
			break;
		}
		i++;
	}
	return (phys_addr_t)0;
}
unsigned long fb_isolation;

extern unsigned long get_cntfrq(void);
extern unsigned long get_cntpct(void);
unsigned long protect_memory_va(unsigned long va_el1, unsigned long size)
{
	phys_addr_t pa;
	int i = 0 ;
	unsigned long start , end;
	start = get_cntpct();

	while (size > 0)
	{
		pa = VAToPA_EL1_S2(va_el1) ;
		if ((pa&0x1) == 0)
		{
			if (protectFBCount<MAX_PROTECTED_FB_PA) 
			{
				protectfbPaList[protectFBCount] = pa & (~0xffffff0000000fff);
				protectFBCount ++ ;
			}
		}
		va_el1 += 0x1000 ;
		size -= 0x1000 ;
	}
	
	for (i=0; i<protectFBCount; i++)
	{
		host_stage2_idmap_locked(protectfbPaList[i], 0x1000, PAGE_HYP_RO, 0);
	}
	
	fb_va_el2 = map_fb_to_el2 (protectfbPaList, protectFBCount) ;

	end = get_cntpct();
	fb_isolation = end - start;
	
	return 0 ;
}

static void handle___hvc_protect_memory(struct kvm_cpu_context *host_ctxt)
{
	phys_addr_t ret = 0;
	DECLARE_REG(unsigned long, va_el1, host_ctxt, 1);
	DECLARE_REG(unsigned long, size, host_ctxt, 2);
	DECLARE_REG(unsigned long, is_pa, host_ctxt, 3);
	phys_addr_t pa;

	// va
	if (!is_pa) 
	{
		cpu_reg(host_ctxt, 1) = protect_memory_va (va_el1, size) ;
		return ;
	}

	// pa
	while (size > 0)
	{
		pa = va_el1;

		pa = pa & (~0xff00000000000fffUL);
		// protect
		ret = host_stage2_idmap_locked(pa, 0x1000, PAGE_HYP_RO, 0);

		start_protect = 1;

		if (ret == 0)
		{
			ret = AddtoProtectList(pa);
		}
		else
		{
			ret = -1;
		}

		va_el1 += 0x1000;
		size -= 0x1000;
	}

	cpu_reg(host_ctxt, 1) = (unsigned long)ret;
}

void handle___hvc_read_memory(struct kvm_cpu_context *host_ctxt)
{
	DECLARE_REG(unsigned long, va_el1, host_ctxt, 1);
	DECLARE_REG(unsigned long, size, host_ctxt, 2);
	DECLARE_REG(unsigned long, is_pa, host_ctxt, 3);
	phys_addr_t pa;

	if (va_el1 == 0)
	{
		cpu_reg(host_ctxt, 1) = -1;
		return;
	}

	if (is_pa)
	{
		pa = va_el1;
	}
	else
	{
		pa = VAToPA_EL1_S2(va_el1);
		if ((pa & 0x1) == 0x1)
		{
			cpu_reg(host_ctxt, 1) = -2;
			return;
		}
		pa &= (~0xff00000000000fffUL);
		pa |= (va_el1 & 0xfffUL);
	}

	size = size & 0xFF;
	if (size == sizeof(unsigned int))
	{
		unsigned int *va_el2 = NULL;
		va_el2 = (unsigned int *)find_in_map_by_pa(pa);

		if (va_el2)
		{
			cpu_reg(host_ctxt, 1) = (unsigned long)*va_el2;
		}
		else
		{
			cpu_reg(host_ctxt, 1) = (unsigned long)-3;
		}
	}
	else
	{
		cpu_reg(host_ctxt, 1) = -4;
	}
}

static void handle___hvc_set_drm_config(struct kvm_cpu_context *host_ctxt)
{
	DECLARE_REG(unsigned int, dma_iova, host_ctxt, 1);
	drm_config_hyp.dma_iova = dma_iova;
}

static unsigned long isFastboot = 1;
static void handle___hvc_isfastboot(struct kvm_cpu_context *host_ctxt)
{
	DECLARE_REG(unsigned long, isF, host_ctxt, 1);
	DECLARE_REG(unsigned long, setorget, host_ctxt, 2); // set :1, get: 0
	if (setorget == 0)
	{
		cpu_reg(host_ctxt, 1) = isFastboot;
	}
	else
	{
		isFastboot = isF;
	}
}
#define l1_index_mask (~0xffffff803ffffffful)
#define l1_index_shift 30
#define l2_base_addr_mask (~0xffffff0000000ffful)
#define l2_index_mask (~0xffffffffc01ffffful)
#define l2_index_shift 21
#define l3_base_addr_mask (~0xffffff0000000ffful)
#define l3_index_mask (~0xffffffffffe00ffful)
#define l3_index_shift 12
#define mem_pa_mask (~0xffffff0000000ffful)


extern unsigned long fb_map_to_pvm;
extern unsigned long isolation_cost;
extern unsigned long win5_cost;
unsigned long overhead_alias;

static void handle___hvc_temp_test(struct kvm_cpu_context *host_ctxt)
{
	DECLARE_REG(unsigned long, nfunc, host_ctxt, 1);
	unsigned long val = -1;
	unsigned long start, end;

	switch (nfunc)
	{
	case 0:
		val = read_sysreg_s(SYS_ID_AA64DFR0_EL1);
		break;

	case 1:
		val = read_sysreg_s(SYS_PMEVCNTRn_EL0(0x3));
		break;

	case 2:
		val = read_sysreg(pmcr_el0);
		break;

	case 3:
		val = read_sysreg(mdcr_el2);
		break;

	case 5:
	{
		DECLARE_REG(unsigned long, va_el1, host_ctxt, 2);
		phys_addr_t ipa = VAToPA_EL1(va_el1);
		if ((ipa & 0x1) == 0x0)
		{
			ipa = ipa & (~0xff00000000000fffUL);
			ipa = ipa | (va_el1 & 0xfff);
			val = (unsigned long)ipa;
		}
		break;
	}
	case 9:
	{
		DECLARE_REG(unsigned long, index, host_ctxt, 2);
		if (index < MAX_PROTECTED_REG_PA)
		{
			val = (unsigned long)protectPaList[index];
		}
		else
		{
			val = ~index;
		}

		break;
	}
	case 7:
	{
		DECLARE_REG(unsigned long, index, host_ctxt, 2);
		val = protectfbPaList[index] ;
		break;
	}
	case 8:
	{
		unsigned long pa;
		int off;
		pa = 0x1C275000UL;
		start = get_cntpct();
		host_stage2_idmap_locked(pa, 0x1000, PAGE_HYP_RO, 0);
		AddtoProtectList(pa);

		pa = 0x1C255000UL;
		host_stage2_idmap_locked(pa, 0x1000, PAGE_HYP_RO, 0);
		AddtoProtectList(pa);

		pa = 0x1C0D0000UL;
		for (off = 0; off < 0x6000; off += 0x1000)
		{
			host_stage2_idmap_locked(pa + off, 0x1000, PAGE_HYP_RO, 0);
			AddtoProtectList(pa);
		}

		pa = 0x1C0B0000UL;
		for (off = 0; off < 0x6000; off += 0x1000)
		{
			host_stage2_idmap_locked(pa + off, 0x1000, PAGE_HYP_RO, 0);
			AddtoProtectList(pa);
		}
		end = get_cntfrq();
		isolation_cost = end - start;
		break;
	}
	case 6:
	{
		extern u64 fb_pa;
		DECLARE_REG(unsigned long, va_el1, host_ctxt, 2);
		phys_addr_t pa;
		pa = VAToPA_EL1_S2(va_el1);
		if ((pa & 0x1) == 0x0)
		{
			pa = pa & (~0xffff000000000fffUL);
			val = (unsigned long)pa;
		}
		fb_pa = val;
		break;
	}
	case 10:
	{
		
		DECLARE_REG(unsigned long, cnt, host_ctxt, 2);	
		void DrawLock(unsigned char* vaddr, int x, int y, int locked) ;
		DrawLock((unsigned char*)fb_va_el2, 0, 0, cnt) ;
		break ;
	}
	case 11:
	{
		//unsigned long overhead_va;
		unsigned long *performance_array;
		unsigned long performance;
		DECLARE_REG(unsigned long, overhead_va, host_ctxt, 2);
		performance = VAToPA_EL1_S2(overhead_va);
		performance = performance & (~0xffff000000000ffful) ;
		performance = performance| (overhead_va & 0xfffUL);
		performance_array =(unsigned long*) __pkvm_create_private_mapping(performance, 1024, PAGE_HYP_DEVICE);
		performance_array[1] = get_cntfrq();
		performance_array[2] = fb_map_to_pvm;
		performance_array[3] = isolation_cost;
		performance_array[4] = fb_isolation;
		performance_array[5] = overhead_alias;
	}



	default:
		break;
	}
	cpu_reg(host_ctxt, 1) = val;
}

void set_win5_size (unsigned long w, unsigned long h)
{
	drm_config_hyp.src.w = w ;
	drm_config_hyp.src.f_h = drm_config_hyp.src.h = h ;
	drm_config_hyp.dst.w = w ;
	drm_config_hyp.dst.f_h = drm_config_hyp.dst.h = h ;

	DrawLock((unsigned char*)fb_va_el2, 0, 0, h==64?0:1) ; //0 unlock 1 locked
}


unsigned long* map_l1_va (void)
{
	static unsigned long *l1_va = 0 ;
	unsigned long ttbr_el2 ;

	if (l1_va)
		return l1_va ;

	ttbr_el2 = getTTBR0_EL2() ;
	ttbr_el2 = ttbr_el2 & (~0xFFFF000000000FFFUL) ;
	l1_va = (unsigned long *)__pkvm_create_private_mapping(ttbr_el2,
		0x1000, KVM_PGTABLE_PROT_DEVICE | KVM_PGTABLE_PROT_R | KVM_PGTABLE_PROT_W);
	
	return l1_va ;
}
unsigned long* map_fb_to_el2 (phys_addr_t *fb_list, int cnt)
{
	unsigned long *fb_va_el2 = NULL;
	unsigned long l2_pa ;
	unsigned long *l2_oa, *l1_oa, *l3_start;
	
	// 512 for level 2 page table, 1 page, we use 8 entry, 
	// 512*8 for level 3 page table, 8 page, 512*8 entry, all 16 M
	// +1 in case it is not page aligned.
	static unsigned long my_pt_mem[512 + 512*8 + 512] ;
	unsigned long *my_pt = my_pt_mem ;
	unsigned long tmp ;

	memset (my_pt, 0, sizeof(my_pt_mem)) ;
	
	tmp = (unsigned long) my_pt ;
	tmp = (tmp + 0xfffUL) & (~0xFFFUL) ;
	my_pt = (unsigned long *) tmp ;
	
	l1_oa = map_l1_va() ;
	if (l1_oa)
	{
		int i = 0 ;
		// looking for an empty entry from level1 page table.
		for (i=100; i <512; i++) 
		{
			if (((unsigned long)l1_oa[i] & 0x1 ) == 0)
			{
				// found an empty entry (1GB)
				l2_pa = VAToPA_EL2((unsigned long)my_pt) ;
				if ((l2_pa & 0x1) == 0x0)
				{
					tmp = l2_pa & (~0xffffff0000000fffUL) ;
					l1_oa[i] = tmp | 0x3 ;
					fb_va_el2 = (unsigned long*)(((unsigned long)i) << 30) ;
				}

				break ;
			}
		}
		l2_oa = my_pt ;
		for(i=0; i<8; i++)
		{
			tmp = (unsigned long) my_pt ;
			tmp = tmp + ((i+1) * 0x1000UL) ;
			tmp = VAToPA_EL2 (tmp) ;
			tmp = tmp & (~0xffffff0000000fffUL) ;
			l2_oa[i] = tmp | 0x3 ;
		}

		l3_start = my_pt + (0x1000 / sizeof(unsigned long)) ;
		for (i=0; i<cnt; i++)
		{
			tmp  = fb_list[i];
			if (tmp!=0)
			{
				tmp = tmp & (~0xffffff0000000fffUL) ;
				l3_start[i] = tmp | 0x40000000000743UL ;
			}
			else 
			{
				break ;
			}
		}
	}
	return fb_va_el2 ;
}



typedef struct __block_16X16 {
	unsigned int head[4] ;
	unsigned char data[1024] ;
} block_16X16 ;

static block_16X16 lock[] = {
	#include "lock.h"
} ;
static block_16X16 unlock[] = {
	#include "unlock.h"
} ;

static block_16X16 font[] = {
	#include "font.h"
} ;


#define BLK_PER_LINE (1088/16)
#define PIXEL_PER_BLK (16)
#define FONTSIZE  (64)
#define FONTSIZE_H  FONTSIZE
#define FONTSIZE_W (FONTSIZE/2)
#define BLK_PER_CHAR (8)

void WriteBlocks(unsigned char* vaddr, int pos_x, int pos_y, int w, int h, block_16X16* fb_block_buf) {

	unsigned int offset ;

	int x = 0, y = 0 ;
	int blk_no = 0 ;

	int start_block = pos_y / PIXEL_PER_BLK * BLK_PER_LINE + pos_x / PIXEL_PER_BLK ;

	for(y=0; y<h/PIXEL_PER_BLK; y++) {
		for(x=0; x<w/PIXEL_PER_BLK; x++) {
			// copy header ;
		
			memcpy (vaddr+start_block*sizeof(fb_block_buf[blk_no].head)+sizeof(fb_block_buf[blk_no].head[0]) ,
					&(fb_block_buf[blk_no].head[1]),
					sizeof(fb_block_buf[blk_no].head)-sizeof(fb_block_buf[blk_no].head[0])) ;
			
			// get offset for the 1K data ;
			offset = *(unsigned int*)(vaddr+start_block*sizeof(fb_block_buf[0].head)) ;
		
			// copy the 1K data;

			memcpy (vaddr+offset, fb_block_buf[blk_no].data, sizeof(fb_block_buf[blk_no].data)) ;

			start_block ++ ;
			blk_no ++ ;
		}
		start_block += (BLK_PER_LINE - w/PIXEL_PER_BLK) ;
	}
}

void DrawLock(unsigned char* vaddr, int x, int y, int locked) {
	block_16X16* char_data ;
	
	if (locked)
		char_data = lock ;
	else
		char_data = unlock ;

	WriteBlocks(vaddr, x, y, 64, 64, char_data) ;

	return ;
}

void DrawString(unsigned char* vaddr, int x, int y, char* draw_str) {
	int i = 0 ;
	block_16X16* char_data ;

	while ((draw_str[i] >= 'A' && draw_str[i] <= 'Z')||(draw_str[i] == ' ')) {
	        
	        if(draw_str[i] == ' '){
	          char_data = &((block_16X16*)font)[(26 * BLK_PER_CHAR)] ; ;
		  
	        }else{
	        
		  char_data = &((block_16X16*)font)[((draw_str[i] - 'A') * BLK_PER_CHAR)] ;
		  
		}
		WriteBlocks(vaddr, x, y, FONTSIZE_W, FONTSIZE_H, char_data) ;
		i++ ;
		x = x+FONTSIZE_W ;
	}
}


extern unsigned long alias;

void draw_alias(void)
{
	DrawString((unsigned char*)fb_va_el2, 64, 0, "                                ");
	DrawString((unsigned char*)fb_va_el2, 128, 0, (char*)(&alias));
}

void draw_blank(void)
{
	DrawString((unsigned char*)fb_va_el2, 64, 0, "                                ");
}

