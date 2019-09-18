
#include "rv-sys.h"

using namespace riscv;

/* Parameterized privileged soft-mmu processor models */

using priv_emulator_rv32imafdc = processor_runloop<processor_privileged<processor_rv32imafdc_model<decode,processor_priv_rv32imafd,mmu_soft_rv32>>>;
using priv_emulator_rv64imafdc = processor_runloop<processor_privileged<processor_rv64imafdc_model<decode,processor_priv_rv64imafd,mmu_soft_rv64>>>;



#ifdef RECOGNI

proxy_emulator_rv32imafdc proc;
rv_emulator emulator;

int emulation_setup(int argc, const char* argv[], const char* envp[])
{
	emulator.parse_commandline(argc, argv, envp);
	emulator.load();
	emulator.setup_proxy<proxy_emulator_rv32imafdc>(proc);
	return 0;
}

int emulation_run(size_t count)
{
    exit_cause ec;
    ec = proc.step(count);
    return (ec != exit_cause_cli);
}

char *emulation_debug(char *debug_cmd)
{
    std::shared_ptr<debug_cli<proxy_emulator_rv32imafdc>> cli;
    cli->one_shot(&proc, debug_cmd);
    return NULL;
}

void emulation_fini()
{
    emulator.fini_proxy(proc);
}

int emulation_pin_get(std::string pin_type, unsigned pin_instance) {
    return proc.pins.pin_get(pin_type, pin_instance);
}

void emulation_pin_set(std::string pin_type, unsigned pin_instance,
		       int pullup, int val) {
    proc.pins.ext_pin_set(pin_type, pin_instance, val);
    proc.pins.ext_pin_pullup(pin_type, pin_instance, pullup);
}

#endif  // RECOGNI
