#ifndef RV8_H
#define RV8_H

int emulation_setup(int argc, const char* argv[], const char* envp[]);
int emulation_run(size_t count);
char *emulation_debug(char *cmd);
void emulation_fini();

#endif
