//
//  rv-main.cc
//


#include "rv-sim.h"

/* Parameterized ABI proxy processor models */

using proxy_emulator_rv32imafdc = processor_runloop<
	processor_proxy<processor_rv32imafdc_model<
	decode, processor_rv32imafd, mmu_proxy_rv32>>>;
using proxy_emulator_rv64imafdc = processor_runloop<
	processor_proxy<processor_rv64imafdc_model<
	decode, processor_rv64imafd, mmu_proxy_rv64>>>;


/* program main */
struct rv_emulator;

int main(int argc, const char* argv[], const char* envp[])
{
	rv_emulator emulator;
	emulator.parse_commandline(argc, argv, envp);
	emulator.exec();
	return 0;
}

