//
//  device-gpio.h
//

#ifndef rv_device_gpio_h
#define rv_device_gpio_h

#include "pin.h"

namespace riscv {

	/* GPIO MMIO device */

	template <typename P>
	struct gpio_mmio_device : memory_segment<typename P::ux>
	{
		typedef typename P::ux UX;
		typedef std::shared_ptr<plic_mmio_device<P>> plic_mmio_device_ptr;

		P &proc;
		plic_mmio_device_ptr plic;
		UX irq;

		unsigned pin_instance_base;

		/* GPIO registers */

		struct {
			u32 ie;  /* interrupt enable */
			u32 ip;  /* interrupt pending */
			u32 in;  /* intput buffer */
			u32 out; /* output buffer */
		        u32 dir; /* Direction (1=out) */ 
		        u32 pull[32]; /* Internal pullup/down */ 
		} gpio;

		enum {
			OUT_POWER_OFF = 1,
			OUT_RESET = 2,
		};

		enum {
			total_size = sizeof(u32) * 37
		};

		constexpr u8* as_u8() { return (u8*)&gpio.ie; }
		constexpr u16* as_u16() { return (u16*)&gpio.ie; }
		constexpr u32* as_u32() { return (u32*)&gpio.ie; }
		constexpr u64* as_u64() { return (u64*)&gpio.ie; }

		/* MIPI constructor */

	gpio_mmio_device(P &proc, UX mpa, plic_mmio_device_ptr plic, UX irq,
			 unsigned pin_instance_base) :
			memory_segment<UX>("GPIO", mpa, /*uva*/0, /*size*/total_size,
				pma_type_io | pma_prot_read | pma_prot_write),
			proc(proc),
			plic(plic),
			irq(irq),
			pin_instance_base(pin_instance_base),
			gpio{}
			{
			    for (int i=0; i<32; i++) {
				proc.pins.add_pin("GPIO", i + pin_instance_base);
			    } 
			}

		/* GPIO interface */

		void print_registers()
		{
			debug("gpio_mmio:ie               0x%08x", gpio.ie);
			debug("gpio_mmio:ip               0x%08x", gpio.ip);
			debug("gpio_mmio:in               0x%08x", gpio.in);
			debug("gpio_mmio:out              0x%08x", gpio.out);
			debug("gpio_mmio:dir              0x%08x", gpio.dir);
			debug("gpio_mmio:pull             0x%08x", gpio.pull);
		}

		void service()
		{
			plic->set_irq(irq, (gpio.ie & gpio.ip) ? 1 : 0);
		}

		void pin_set()
		{
		    for (int i=0; i<32; i++) {
			u32 mask = 1 << i;
			/* Set the pullups in case they changed */
			proc.pins.int_pin_pullup("GPIO", i, gpio.pull[i]); 
			if ((gpio.dir & mask) == 0) {
			    /* Input pin - drive it Z */
			    proc.pins.int_pin_set("GPIO", i, 'Z'); 
			} else {
			    /* Output drive according to out reg */
			    proc.pins.int_pin_set("GPIO", i,
				   (gpio.out == '0') ? 0 : 1); 
			} 
			gpio.in &= ~mask;
			if (proc.pins.pin_get("GPIO", i) == '0' ) {
			    gpio.in |= mask;
			} 
		    }
		}

		void trigger()
		{
			if (gpio.out & OUT_POWER_OFF) {
				proc.raise(P::internal_cause_poweroff, proc.pc);
			}
			if (gpio.out & OUT_RESET) {
				proc.reset();
			}
		}

		/* GPIO MMIO */

		buserror_t load_8 (addr_t va, u8  &val)
		{
			val = (va < total_size) ? *(as_u8() + va) : 0;
			if (proc.log & proc_log_mmio) {
				printf("gpio_mmio:0x%04llx -> 0x%02hhx\n", addr_t(va), val);
			}
			return 0;
		}

		buserror_t load_16(addr_t va, u16 &val)
		{
			val = (va < total_size - 1) ? *(as_u16() + (va>>1)) : 0;
			if (proc.log & proc_log_mmio) {
				printf("gpio_mmio:0x%04llx -> 0x%04hx\n", addr_t(va), val);
			}
			return 0;
		}

		buserror_t load_32(addr_t va, u32 &val)
		{
			val = (va < total_size - 3) ? *(as_u32() + (va>>2)) : 0;
			if (proc.log & proc_log_mmio) {
				printf("gpio_mmio:0x%04llx -> 0x%08x\n", addr_t(va), val);
			}
			return 0;
		}

		buserror_t load_64(addr_t va, u64 &val)
		{
			val = (va < total_size - 7) ? *(as_u64() + (va>>3)) : 0;
			if (proc.log & proc_log_mmio) {
				printf("gpio_mmio:0x%04llx -> 0x%016llx\n", addr_t(va), val);
			}
			return 0;
		}

		buserror_t store_8 (addr_t va, u8  val)
		{
			if (proc.log & proc_log_mmio) {
				printf("gpio_mmio:0x%04llx <- 0x%02hhx\n", addr_t(va), val);
			}
			if (va < total_size) *(as_u8() + va) = val;
			//trigger();
			pin_set();
			return 0;
		}

		buserror_t store_16(addr_t va, u16 val)
		{
			if (proc.log & proc_log_mmio) {
				printf("gpio_mmio:0x%04llx <- 0x%04hx\n", addr_t(va), val);
			}
			if (va < total_size - 1) *(as_u16() + (va>>1)) = val;
			//trigger();
			pin_set();
			return 0;
		}

		buserror_t store_32(addr_t va, u32 val)
		{
			if (proc.log & proc_log_mmio) {
				printf("gpio_mmio:0x%04llx <- 0x%08x\n", addr_t(va), val);
			}
			if (va < total_size - 3) *(as_u32() + (va>>2)) = val;
			//trigger();
			pin_set();
			return 0;
		}

		buserror_t store_64(addr_t va, u64 val)
		{
			if (proc.log & proc_log_mmio) {
				printf("gpio_mmio:0x%04llx <- 0x%016llx\n", addr_t(va), val);
			}
			if (va < total_size - 7) *(as_u64() + (va>>3)) = val;
			//trigger();
			pin_set();
			return 0;
		}

	};

}

#endif
