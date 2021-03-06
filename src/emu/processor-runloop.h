//
//  processor-runloop.h
//

#ifndef rv_processor_runloop_h
#define rv_processor_runloop_h

// TODO(sabhiram) : HTTP server requires the below hack -- can we remove?
/*
 *  Because the smart people at RV8 have chosen to call one of their string 
 *  libraries `strings.h` and we require the actual linux/osx strings.h to be 
 *  included, we are forced to add a path explicitly forcing the variant of the
 *  file we want. Since `c++/../strings.h` which we used to rely on does not 
 *  *ALWAYS* show up on OSX (without xcode installed for example), we use the 
 *  ever-present `net` directory to force this file.
 */
#include <net/../strings.h>
#include "httplib.h"

namespace riscv {

	/* Simple processor stepper with instruction cache */

	struct processor_singleton
	{
		static processor_singleton *current;
	};

	processor_singleton* processor_singleton::current = nullptr;

	template <typename P>
	struct processor_runloop : processor_singleton, P
	{
		static const size_t inst_cache_size = 8191;
		static const int inst_step = 100000;

		std::shared_ptr<debug_cli<P>> cli;

		using Request = httplib::Request;
		using Response = httplib::Response;
		std::shared_ptr<httplib::Server> server;

		struct rv_inst_cache_ent
		{
			inst_t inst;
			typename P::decode_type dec;
		};

		rv_inst_cache_ent inst_cache[inst_cache_size];

		processor_runloop() : cli(std::make_shared<debug_cli<P>>()), inst_cache() {}
		processor_runloop(std::shared_ptr<debug_cli<P>> cli) : cli(cli), inst_cache() {}

		static void signal_handler(int signum, siginfo_t *info, void *)
		{
			static_cast<processor_runloop<P>*>
				(processor_singleton::current)->signal_dispatch(signum, info);
		}

		static void signal_handler_sev(int signum, siginfo_t *info, void *)
		{
			static_cast<processor_runloop<P>*>
				(processor_singleton::current)->signal_dispatch(signum, info);
		}

		void signal_dispatch(int signum, siginfo_t *info)
		{
			printf("SIGNAL   :%s pc:0x%0llx si_addr:0x%0llx\n",
				signal_name(signum), (addr_t)P::pc, (addr_t)info->si_addr);

			/* let the processor longjmp */
			P::signal(signum, info);
		}

		void init()
		{
			// block signals before so we don't deadlock in signal handlers
			sigset_t set;
			sigemptyset(&set);
			sigaddset(&set, SIGSEGV);
			sigaddset(&set, SIGTERM);
			sigaddset(&set, SIGQUIT);
			sigaddset(&set, SIGINT);
			sigaddset(&set, SIGHUP);
			sigaddset(&set, SIGUSR1);
			if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
				panic("can't set thread signal mask: %s", strerror(errno));
			}

			// disable unwanted signals
			sigset_t sigpipe_set;
			sigemptyset(&sigpipe_set);
			sigaddset(&sigpipe_set, SIGPIPE);
			sigprocmask(SIG_BLOCK, &sigpipe_set, nullptr);

			// install signal handler
			struct sigaction sigaction_handler_sev;
			memset(&sigaction_handler_sev, 0, sizeof(sigaction_handler_sev));
			sigaction_handler_sev.sa_sigaction = &processor_runloop<P>::signal_handler_sev;
			sigaction_handler_sev.sa_flags = SA_SIGINFO;
			sigaction(SIGSEGV, &sigaction_handler_sev, nullptr);

			struct sigaction sigaction_handler;
			memset(&sigaction_handler, 0, sizeof(sigaction_handler));
			sigaction_handler.sa_sigaction = &processor_runloop<P>::signal_handler;
			sigaction_handler.sa_flags = SA_SIGINFO;
			sigaction(SIGTERM, &sigaction_handler, nullptr);
			sigaction(SIGQUIT, &sigaction_handler, nullptr);
			sigaction(SIGINT, &sigaction_handler, nullptr);
			sigaction(SIGHUP, &sigaction_handler, nullptr);
			sigaction(SIGUSR1, &sigaction_handler, nullptr);
			processor_singleton::current = this;

			/* unblock signals */
			if (pthread_sigmask(SIG_UNBLOCK, &set, NULL) != 0) {
				panic("can't set thread signal mask: %s", strerror(errno));
			}

			/* processor initialization */
			P::init();
		}

		void run(exit_cause ex = exit_cause_continue)
		{
			u32 logsave = P::log;
			size_t count = inst_step;
			for (;;) {
				switch (ex) {
					case exit_cause_continue:
						break;
					case exit_cause_cli:
						P::debugging = true;
						count = cli->run(this);
						if (count == size_t(-1)) {
							P::debugging = false;
							P::log = logsave;
							count = inst_step;
						} else {
							P::log |= (proc_log_inst | proc_log_operands | proc_log_trap);
						}
						break;
					case exit_cause_poweroff:
						return;
				}
				ex = step(count);
				if (P::debugging && ex == exit_cause_continue) {
					ex = exit_cause_cli;
				}
			}
		}

		void run_server(const int server_port)
		{
			if (!server) {
				server = std::make_shared<httplib::Server>();
				if (!server) {
					panic("error: could not create httplib::Server instance\n");
				}

				server->set_keep_alive_max_count(0);

				server->Get("/ping", [&](const Request& req, Response& rsp) {
					rsp.set_content("PONG", "application/text");
				});

				server->Get("/step", [&](const Request& req, Response& rsp) {
					const int ex = step(1);
					if (ex != exit_cause_continue)
						rsp.set_content("FINISHED", "application/text");
					else
						rsp.set_content("CONTINUE", "application/text");
				});

				server->Get(R"(/step/(\d+))", [&](const Request& req, Response& rsp) {
					const uint n = std::stoi(req.matches[1]);
					const int ex = step(n);
					if (ex != exit_cause_continue)
						rsp.set_content("FINISHED", "application/text");
					else
						rsp.set_content("CONTINUE", "application/text");
				});

				server->Get("/finish", [&](const Request& req, Response& rsp) {
					run();
				});
			}

			server->listen("localhost", server_port);
		}

		exit_cause step(size_t count)
		{
			typename P::decode_type dec;
			typename P::ux inststop = P::instret + count;
			typename P::ux pc_offset, new_offset;
			inst_t inst = 0, inst_cache_key;

			/* interrupt service routine */
			P::time = cpu_cycle_clock();
			P::isr();

			/* trap return path */
			int cause;
			if (unlikely((cause = setjmp(P::env)) > 0)) {
				cause -= P::internal_cause_offset;
				switch(cause) {
					case P::internal_cause_cli:
						return exit_cause_cli;
					case P::internal_cause_fatal:
						P::print_csr_registers();
						P::print_int_registers();
						return exit_cause_poweroff;
					case P::internal_cause_poweroff:
						return exit_cause_poweroff;
				}

				P::trap(dec, cause);
				if (!P::running) {
					return exit_cause_poweroff;
				}
			}

			/* step the processor */
			while (P::instret != inststop) {
				inst = P::mmu.inst_fetch(*this, P::pc, pc_offset);
				inst_cache_key = inst % inst_cache_size;
				if (inst_cache[inst_cache_key].inst == inst) {
					dec = inst_cache[inst_cache_key].dec;
				} else {
					P::inst_decode(dec, inst);
					inst_cache[inst_cache_key].inst = inst;
					inst_cache[inst_cache_key].dec = dec;
				}

				if ((new_offset = P::inst_exec(dec, pc_offset)) != typename P::ux(-1)  ||
					(new_offset = P::inst_priv(dec, pc_offset)) != typename P::ux(-1))
				{
					if (P::log) P::print_log(dec, inst);
					P::pc += new_offset;
					P::instret++;
				} else {
					P::raise(rv_cause_illegal_instruction, P::pc);
				}
				if (P::pc == P::breakpoint && P::breakpoint != 0) {
					return exit_cause_cli;
				}
			}
			return exit_cause_continue;
		}

	};

}

#endif
