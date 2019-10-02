//
//  device-external.h
//

#ifndef rv_device_external_h
#define rv_device_external_h

#include "pin.h"

namespace riscv {

	/* EXTERNAL MMIO device */

	template <typename P>
	struct external_mmio_device : memory_segment<typename P::ux>
	{
		typedef typename P::ux UX;

		P &proc;
		
		std::function<int (unsigned long long addr,
				   unsigned val)> reg_write_cb_fn;
		std::function<int (unsigned long long addr,
				   unsigned &val)> reg_read_cb_fn;
		
		enum {
			total_size = sizeof(u32) * 4096
		};

		/* MIPI constructor */

	external_mmio_device(P &proc, UX mpa):
			memory_segment<UX>("EXTERNAL", mpa, /*uva*/0, /*size*/total_size,
				pma_type_io | pma_prot_read | pma_prot_write),
			proc(proc)
			{
			    reg_write_cb_fn = NULL;
			}

#if 0			
		buserror_t load_8 (addr_t va, u8  &val)
		{
			val = (va < total_size) ? *(as_u8() + va) : 0;
			if (proc.log & proc_log_mmio) {
				printf("external_mmio:0x%04llx -> 0x%02hhx\n", addr_t(va), val);
			}
			return 0;
		}

		buserror_t load_16(addr_t va, u16 &val)
		{
			val = (va < total_size - 1) ? *(as_u16() + (va>>1)) : 0;
			if (proc.log & proc_log_mmio) {
				printf("external_mmio:0x%04llx -> 0x%04hx\n", addr_t(va), val);
			}
			return 0;
		}
#endif
		
		buserror_t load_32(addr_t va, u32 &val)
		{
		        buserror_t rv = 1;
			if (va < (total_size - 3)) {
			    rv = reg_read_cb_fn(va, val);
			}
			if (proc.log & proc_log_mmio) {
				printf("external_mmio:0x%04llx -> 0x%08x\n", addr_t(va), val);
			}
			return rv;
		}

#if 0		
		buserror_t load_64(addr_t va, u64 &val)
		{
			val = (va < total_size - 7) ? *(as_u64() + (va>>3)) : 0;
			if (proc.log & proc_log_mmio) {
				printf("external_mmio:0x%04llx -> 0x%016llx\n", addr_t(va), val);
			}
			return 0;
		}

		buserror_t store_8 (addr_t va, u8  val)
		{
			if (proc.log & proc_log_mmio) {
				printf("external_mmio:0x%04llx <- 0x%02hhx\n", addr_t(va), val);
			}
			if (va < total_size) *(as_u8() + va) = val;
			//trigger();
			pin_set();
			return 0;
		}

		buserror_t store_16(addr_t va, u16 val)
		{
			if (proc.log & proc_log_mmio) {
				printf("external_mmio:0x%04llx <- 0x%04hx\n", addr_t(va), val);
			}
			if (va < total_size - 1) *(as_u16() + (va>>1)) = val;
			//trigger();
			pin_set();
			return 0;
		}
#endif
		buserror_t store_32(addr_t va, u32 val)
		{
			if (proc.log & proc_log_mmio) {
				printf("external_mmio:0x%04llx <- 0x%08x\n", addr_t(va), val);
			}
			if (va < (total_size - 3)) {
			    return reg_write_cb_fn(va, val);
			}
			return 1;
		}
#if 0
		buserror_t store_64(addr_t va, u64 val)
		{
			if (proc.log & proc_log_mmio) {
				printf("external_mmio:0x%04llx <- 0x%016llx\n", addr_t(va), val);
			}
			if (va < total_size - 7) *(as_u64() + (va>>3)) = val;
			//trigger();
			pin_set();
			return 0;
		}
#endif

	};

}

#endif
