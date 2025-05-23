// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015 - ARM Ltd
 * Author: Marc Zyngier <marc.zyngier@arm.com>
 */

#include <hyp/switch.h>
#include <hyp/sysreg-sr.h>

#include <linux/arm-smccc.h>
#include <linux/kvm_host.h>
#include <linux/types.h>
#include <linux/jump_label.h>
#include <uapi/linux/psci.h>

#include <kvm/arm_psci.h>

#include <asm/barrier.h>
#include <asm/cpufeature.h>
#include <asm/kprobes.h>
#include <asm/kvm_asm.h>
#include <asm/kvm_emulate.h>
#include <asm/kvm_hyp.h>
#include <asm/kvm_mmu.h>
#include <asm/fpsimd.h>
#include <asm/debug-monitors.h>
#include <asm/processor.h>
#include <asm/memory.h>

#include <nvhe/mem_protect.h>
#include <nvhe/pkvm.h>

/* Non-VHE specific context */
DEFINE_PER_CPU(struct kvm_host_data, kvm_host_data);
DEFINE_PER_CPU(struct kvm_cpu_context, kvm_hyp_ctxt);
DEFINE_PER_CPU(unsigned long, kvm_hyp_vector);

static void __activate_traps(struct kvm_vcpu *vcpu)
{
	u64 val;

	___activate_traps(vcpu);
	__activate_traps_common(vcpu);

	val = vcpu->arch.cptr_el2;
	val |= CPTR_EL2_TTA | CPTR_EL2_TAM;
	if (!update_fp_enabled(vcpu)) {
		val |= CPTR_EL2_TFP | CPTR_EL2_TZ;
		__activate_traps_fpsimd32(vcpu);
	}

	write_sysreg(val, cptr_el2);
	write_sysreg(__this_cpu_read(kvm_hyp_vector), vbar_el2);

	if (cpus_have_final_cap(ARM64_WORKAROUND_SPECULATIVE_AT)) {
		struct kvm_cpu_context *ctxt = &vcpu->arch.ctxt;

		isb();
		/*
		 * At this stage, and thanks to the above isb(), S2 is
		 * configured and enabled. We can now restore the guest's S1
		 * configuration: SCTLR, and only then TCR.
		 */
		write_sysreg_el1(ctxt_sys_reg(ctxt, SCTLR_EL1),	SYS_SCTLR);
		isb();
		write_sysreg_el1(ctxt_sys_reg(ctxt, TCR_EL1),	SYS_TCR);
	}
}

static void __deactivate_traps(struct kvm_vcpu *vcpu)
{
	extern char __kvm_hyp_host_vector[];
	u64 cptr;

	___deactivate_traps(vcpu);

	if (cpus_have_final_cap(ARM64_WORKAROUND_SPECULATIVE_AT)) {
		u64 val;

		/*
		 * Set the TCR and SCTLR registers in the exact opposite
		 * sequence as __activate_traps (first prevent walks,
		 * then force the MMU on). A generous sprinkling of isb()
		 * ensure that things happen in this exact order.
		 */
		val = read_sysreg_el1(SYS_TCR);
		write_sysreg_el1(val | TCR_EPD1_MASK | TCR_EPD0_MASK, SYS_TCR);
		isb();
		val = read_sysreg_el1(SYS_SCTLR);
		write_sysreg_el1(val | SCTLR_ELx_M, SYS_SCTLR);
		isb();
	}

	__deactivate_traps_common(vcpu);

	write_sysreg(this_cpu_ptr(&kvm_init_params)->hcr_el2, hcr_el2);

	cptr = CPTR_EL2_DEFAULT;
	if (vcpu_has_sve(vcpu) && (vcpu->arch.flags & KVM_ARM64_FP_ENABLED))
		cptr |= CPTR_EL2_TZ;

	write_sysreg(cptr, cptr_el2);
	write_sysreg(__kvm_hyp_host_vector, vbar_el2);
}

/* Save VGICv3 state on non-VHE systems */
static void __hyp_vgic_save_state(struct kvm_vcpu *vcpu)
{
	if (static_branch_unlikely(&kvm_vgic_global_state.gicv3_cpuif)) {
		__vgic_v3_save_state(&vcpu->arch.vgic_cpu.vgic_v3);
		__vgic_v3_deactivate_traps(&vcpu->arch.vgic_cpu.vgic_v3);
	}
}

/* Restore VGICv3 state on non_VEH systems */
static void __hyp_vgic_restore_state(struct kvm_vcpu *vcpu)
{
	if (static_branch_unlikely(&kvm_vgic_global_state.gicv3_cpuif)) {
		__vgic_v3_activate_traps(&vcpu->arch.vgic_cpu.vgic_v3);
		__vgic_v3_restore_state(&vcpu->arch.vgic_cpu.vgic_v3);
	}
}

/**
 * Disable host events, enable guest events
 */
static bool __pmu_switch_to_guest(struct kvm_cpu_context *host_ctxt)
{
	struct kvm_host_data *host;
	struct kvm_pmu_events *pmu;

	host = container_of(host_ctxt, struct kvm_host_data, host_ctxt);
	pmu = &host->pmu_events;

	if (pmu->events_host)
		write_sysreg(pmu->events_host, pmcntenclr_el0);

	if (pmu->events_guest)
		write_sysreg(pmu->events_guest, pmcntenset_el0);

	return (pmu->events_host || pmu->events_guest);
}

/**
 * Disable guest events, enable host events
 */
static void __pmu_switch_to_host(struct kvm_cpu_context *host_ctxt)
{
	struct kvm_host_data *host;
	struct kvm_pmu_events *pmu;

	host = container_of(host_ctxt, struct kvm_host_data, host_ctxt);
	pmu = &host->pmu_events;

	if (pmu->events_guest)
		write_sysreg(pmu->events_guest, pmcntenclr_el0);

	if (pmu->events_host)
		write_sysreg(pmu->events_host, pmcntenset_el0);
}

/**
 * Handler for protected VM MSR, MRS or System instruction execution in AArch64.
 *
 * Returns true if the hypervisor has handled the exit, and control should go
 * back to the guest, or false if it hasn't.
 */
static bool kvm_handle_pvm_sys64(struct kvm_vcpu *vcpu, u64 *exit_code)
{
	/*
	 * Make sure we handle the exit for workarounds and ptrauth
	 * before the pKVM handling, as the latter could decide to
	 * UNDEF.
	 */
	return (kvm_hyp_handle_sysreg(vcpu, exit_code) ||
		kvm_handle_pvm_sysreg(vcpu, exit_code));
}

/**
 * Handler for protected floating-point and Advanced SIMD accesses.
 *
 * Returns true if the hypervisor has handled the exit, and control should go
 * back to the guest, or false if it hasn't.
 */
static bool kvm_handle_pvm_fpsimd(struct kvm_vcpu *vcpu, u64 *exit_code)
{
	/* Linux guests assume support for floating-point and Advanced SIMD. */
	BUILD_BUG_ON(!FIELD_GET(ARM64_FEATURE_MASK(ID_AA64PFR0_FP),
				PVM_ID_AA64PFR0_ALLOW));
	BUILD_BUG_ON(!FIELD_GET(ARM64_FEATURE_MASK(ID_AA64PFR0_ASIMD),
				PVM_ID_AA64PFR0_ALLOW));

	return kvm_hyp_handle_fpsimd(vcpu, exit_code);
}

static const exit_handler_fn hyp_exit_handlers[] = {
	[0 ... ESR_ELx_EC_MAX]		= NULL,
	[ESR_ELx_EC_CP15_32]		= kvm_hyp_handle_cp15_32,
	[ESR_ELx_EC_SYS64]		= kvm_hyp_handle_sysreg,
	[ESR_ELx_EC_SVE]		= kvm_hyp_handle_fpsimd,
	[ESR_ELx_EC_FP_ASIMD]		= kvm_hyp_handle_fpsimd,
	[ESR_ELx_EC_IABT_LOW]		= kvm_hyp_handle_iabt_low,
	[ESR_ELx_EC_DABT_LOW]		= kvm_hyp_handle_dabt_low,
	[ESR_ELx_EC_PAC]		= kvm_hyp_handle_ptrauth,
};

static const exit_handler_fn pvm_exit_handlers[] = {
	[0 ... ESR_ELx_EC_MAX]		= NULL,
	[ESR_ELx_EC_HVC64]		= kvm_handle_pvm_hvc64,
	[ESR_ELx_EC_SYS64]		= kvm_handle_pvm_sys64,
	[ESR_ELx_EC_SVE]		= kvm_handle_pvm_restricted,
	[ESR_ELx_EC_FP_ASIMD]		= kvm_handle_pvm_fpsimd,
	[ESR_ELx_EC_IABT_LOW]		= kvm_hyp_handle_iabt_low,
	[ESR_ELx_EC_DABT_LOW]		= kvm_hyp_handle_dabt_low,
	[ESR_ELx_EC_PAC]		= kvm_hyp_handle_ptrauth,
};

static const exit_handler_fn *kvm_get_exit_handler_array(struct kvm_vcpu *vcpu)
{
	if (unlikely(vcpu_is_protected(vcpu)))
		return pvm_exit_handlers;

	return hyp_exit_handlers;
}

/*
 * Some guests (e.g., protected VMs) are not be allowed to run in AArch32.
 * The ARMv8 architecture does not give the hypervisor a mechanism to prevent a
 * guest from dropping to AArch32 EL0 if implemented by the CPU. If the
 * hypervisor spots a guest in such a state ensure it is handled, and don't
 * trust the host to spot or fix it.  The check below is based on the one in
 * kvm_arch_vcpu_ioctl_run().
 *
 * Returns false if the guest ran in AArch32 when it shouldn't have, and
 * thus should exit to the host, or true if a the guest run loop can continue.
 */
static void early_exit_filter(struct kvm_vcpu *vcpu, u64 *exit_code)
{
	if (unlikely(vcpu_is_protected(vcpu) && vcpu_mode_is_32bit(vcpu))) {
		/*
		 * As we have caught the guest red-handed, decide that it isn't
		 * fit for purpose anymore by making the vcpu invalid. The VMM
		 * can try and fix it by re-initializing the vcpu with
		 * KVM_ARM_VCPU_INIT, however, this is likely not possible for
		 * protected VMs.
		 */
		vcpu->arch.target = -1;
		*exit_code &= BIT(ARM_EXIT_WITH_SERROR_BIT);
		*exit_code |= ARM_EXCEPTION_IL;
	}
}

/* Switch to the guest for legacy non-VHE systems */
int __kvm_vcpu_run(struct kvm_vcpu *vcpu)
{
	struct kvm_cpu_context *host_ctxt;
	struct kvm_cpu_context *guest_ctxt;
	struct kvm_s2_mmu *mmu;
	bool pmu_switch_needed;
	u64 exit_code;

	/*
	 * Having IRQs masked via PMR when entering the guest means the GIC
	 * will not signal the CPU of interrupts of lower priority, and the
	 * only way to get out will be via guest exceptions.
	 * Naturally, we want to avoid this.
	 */
	if (system_uses_irq_prio_masking()) {
		gic_write_pmr(GIC_PRIO_IRQON | GIC_PRIO_PSR_I_SET);
		pmr_sync();
	}

	host_ctxt = &this_cpu_ptr(&kvm_host_data)->host_ctxt;
	host_ctxt->__hyp_running_vcpu = vcpu;
	guest_ctxt = &vcpu->arch.ctxt;

	pmu_switch_needed = __pmu_switch_to_guest(host_ctxt);

	__sysreg_save_state_nvhe(host_ctxt);
	/*
	 * We must flush and disable the SPE buffer for nVHE, as
	 * the translation regime(EL1&0) is going to be loaded with
	 * that of the guest. And we must do this before we change the
	 * translation regime to EL2 (via MDCR_EL2_E2PB == 0) and
	 * before we load guest Stage1.
	 */
	__debug_save_host_buffers_nvhe(vcpu);

	__kvm_adjust_pc(vcpu);

	/*
	 * We must restore the 32-bit state before the sysregs, thanks
	 * to erratum #852523 (Cortex-A57) or #853709 (Cortex-A72).
	 *
	 * Also, and in order to be able to deal with erratum #1319537 (A57)
	 * and #1319367 (A72), we must ensure that all VM-related sysreg are
	 * restored before we enable S2 translation.
	 */
	__sysreg32_restore_state(vcpu);
	__sysreg_restore_state_nvhe(guest_ctxt);

	mmu = kern_hyp_va(vcpu->arch.hw_mmu);
	__load_stage2(mmu, kern_hyp_va(mmu->arch));
	__activate_traps(vcpu);

	__hyp_vgic_restore_state(vcpu);
	__timer_enable_traps(vcpu);

	__debug_switch_to_guest(vcpu);

	do {
		/* Jump in the fire! */
		exit_code = __guest_enter(vcpu);

		/* And we're baaack! */
	} while (fixup_guest_exit(vcpu, &exit_code));

	__sysreg_save_state_nvhe(guest_ctxt);
	__sysreg32_save_state(vcpu);
	__timer_disable_traps(vcpu);
	__hyp_vgic_save_state(vcpu);

	__deactivate_traps(vcpu);
	__load_host_stage2();

	__sysreg_restore_state_nvhe(host_ctxt);

	if (vcpu->arch.flags & KVM_ARM64_FP_ENABLED)
		__fpsimd_save_fpexc32(vcpu);

	__debug_switch_to_host(vcpu);
	/*
	 * This must come after restoring the host sysregs, since a non-VHE
	 * system may enable SPE here and make use of the TTBRs.
	 */
	__debug_restore_host_buffers_nvhe(vcpu);

	if (pmu_switch_needed)
		__pmu_switch_to_host(host_ctxt);

	/* Returning to host will clear PSR.I, remask PMR if needed */
	if (system_uses_irq_prio_masking())
		gic_write_pmr(GIC_PRIO_IRQOFF);

	host_ctxt->__hyp_running_vcpu = NULL;

	return exit_code;
}

void __noreturn hyp_panic(void)
{
	u64 spsr = read_sysreg_el2(SYS_SPSR);
	u64 elr = read_sysreg_el2(SYS_ELR);
	u64 par = read_sysreg_par();
	struct kvm_cpu_context *host_ctxt;
	struct kvm_vcpu *vcpu;

	host_ctxt = &this_cpu_ptr(&kvm_host_data)->host_ctxt;
	vcpu = host_ctxt->__hyp_running_vcpu;

	if (vcpu) {
		__timer_disable_traps(vcpu);
		__deactivate_traps(vcpu);
		__load_host_stage2();
		__sysreg_restore_state_nvhe(host_ctxt);
	}

	__hyp_do_panic(host_ctxt, spsr, elr, par);
	unreachable();
}

asmlinkage void kvm_unexpected_el2_exception(void)
{
	return __kvm_unexpected_el2_exception();
}

//prism
#define l1_index_mask (~0xffffff803ffffffful)
#define l1_index_shift 30
#define l2_base_addr_mask (~0xffffff0000000ffful)
#define l2_index_mask (~0xffffffffc01ffffful)
#define l2_index_shift 21
#define l3_base_addr_mask (~0xffffff0000000ffful)
#define l3_index_mask (~0xffffffffffe00ffful)
#define l3_index_shift 12
#define mem_pa_mask (~0xffffff0000000ffful)

unsigned long fb_pa;
unsigned long tmp_pa;
unsigned long guest_fb_pa;
unsigned long* host_fb_oa;
unsigned long* guest_fb_oa;
unsigned long host_fb_pa;

extern void invalid_stage2_tlb(unsigned long invalid_ipa_addr);
extern unsigned long VAToPA_EL2(unsigned long addr);
extern unsigned long VAToPA_EL1(unsigned long addr);
extern unsigned long VAToPA_EL1_S2(unsigned long addr);
extern unsigned long VAToPA_EL0_S2(unsigned long addr);
extern unsigned long getTTBR0_EL2(void);
extern unsigned long getVTCR_EL2(void);
extern unsigned long getVTTBR_EL2(void);
extern void setVTTBR_EL2(unsigned long addr);
extern int map_to_secure_endpoint(unsigned long host_addr, unsigned long guest_addr, struct kvm_vcpu *vcpu);
extern unsigned long __pkvm_create_private_mapping(phys_addr_t phys, size_t size,
					    enum kvm_pgtable_prot prot);

extern unsigned long VAToPA_EL0(unsigned long addr);
void *find_in_map_by_pa(phys_addr_t pa) ;

extern void set_win5_size (unsigned long w, unsigned long h) ;


//
//	modify the page table to map ipa to pa.
//
unsigned long digest_list[20];     //store four digest four alias
unsigned long *digest_oa;    //point to digest that is computed by Microdroid Manager
unsigned long alias;

static int entry_used = -1;
extern void DrawString(unsigned char* vaddr, int x, int y, char* draw_str);
extern void draw_alias(void);
extern void draw_blank(void);

//handle hvc from secure endpoint and microdroid manager
asmlinkage unsigned long secure_endpoint_func(unsigned long arg1, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5)  
{
	unsigned long ret = -1 ;
	switch(arg1){
	case 4:
	{
		// allocate IPA
		unsigned long l1_pa, l2_pa ;
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
		
		l1_pa = getVTTBR_EL2();
		l1_pa = l1_pa & (~0xffff000000000001UL);
		l1_oa = (unsigned long*) (__pkvm_create_private_mapping(l1_pa, 512*8, PAGE_HYP_DEVICE));
		if (l1_oa)
		{
#define MAX_PROTECTED_FB_PA (4096)
			extern phys_addr_t protectfbPaList[] ;
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
					}
					ret = (l1_oa[i] & (~0xfffUL)) | i ;
					entry_used = i ;
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
				l2_oa[i] = tmp | 0x7ff | 0x100000000000000 ;
			}

			l3_start = my_pt + (0x1000 / sizeof(unsigned long)) ;
			for (i=0; i<MAX_PROTECTED_FB_PA; i++)
			{
				tmp  = protectfbPaList[i];
				if (tmp!=0)
				{
					// don't map status bar's frame buffer. 
					if (i==0 || ((i>=0x28) && (i<0x28+0x40)) ) continue ;
					
					tmp = tmp & (~0xffffff0000000fffUL) ;
					l3_start[i] = tmp | 0x7ff | 0x100000000000000 ;
				}
				else 
				{
					ret = (((unsigned long) i) << 16) | (ret & 0xfffUL) ;
					break ;
				}
			}
		}
		break ;
	}
	case 6:
	{
		set_win5_size (1080, arg2+64) ;  //widget region is arg2
		break ;
	}
	case 5:
	{
		unsigned long l1_pa, *l1_oa ;
		l1_pa = getVTTBR_EL2();
		l1_pa = l1_pa & (~0xffff000000000001UL);
		l1_oa = (unsigned long*) (__pkvm_create_private_mapping(l1_pa, 512*8, PAGE_HYP_DEVICE));
		l1_oa[entry_used] = 0 ;

		set_win5_size (64, 64) ;
		break ;
	}
	case 7:      //pass digest and draw alias or blank
	{
		if(digest_list[0] == arg2)
			draw_alias();
		else
			draw_blank();
		break;


	}
	case 8 :   //reg digest
	{
		digest_list[0] = arg2;
		digest_list[1] = arg3;
		digest_list[2] = arg4;
		digest_list[3] = arg5;
		break;
	}
	case 9 :  // registratin alias
	{
		alias = arg2;
		draw_alias();
		break;		
	}

	default:
		ret = 0;
		break;
	}
	return ret;
}
