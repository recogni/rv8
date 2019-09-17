#ifndef RV8_H
#define RV8_H

#include <string>

int emulation_setup(int argc, const char* argv[], const char* envp[]);
int emulation_run(size_t count);
char *emulation_debug(char *cmd);
void emulation_fini();

/*
 * pin_num - mapped pin number
 * val is the pin value (-1 - off, 0 - Z, 1 - on)
 * pullup is the external resistor (<0 pulled down, 0 - none, >0 pulled up)
 *      The value of the pullup/down specifies the strength
 */
int emulation_pin_get(std::string pin_type, unsigned pin_instance);
void emulation_pin_set(std::string pin_type, unsigned pin_instance,
		       int pullup, int val);



#endif
