#ifndef _RV8_H_
#define _RV8_H_

#if RECOGNI

int emulation_setup(int argc, const char* argv[], const char* envp[]);
int emulation_run(size_t count);
void emulation_fini();

#endif  // RECOGNI

#endif  // _RV8_H_
