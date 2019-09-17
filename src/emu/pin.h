//
//  pin.h
//

#ifndef rv_pin_h
#define rv_pin_h

#include <string>

namespace riscv {

    struct ext_pin
    {
	std::string pin_type;
	unsigned pin_instance;
	
	/* Pins have value "0", "1" or "Z" */
	char pin_internal;
	char pin_external;

	/* Pullups have values <0 (down), 0 (not pulled), and >0 (up)
	   Pullup values indicate strength */
	int pullup_internal;
	int pullup_external;

    ext_pin(std::string pin_type, unsigned pin_instance) :
	    pin_type(pin_type),
	    pin_instance(pin_instance)
	{
	    /* Set everything to Z */
	    pin_internal = 0;
	    pin_external = 0;
	    pullup_internal = 0;
	    pullup_external = 0;
	}
	~ext_pin() {}

	void print_pin(void)
	{
	    std::string reason = "";
	    int val = 0;
	    if ((pin_internal != 'Z') && (pin_external != 'Z')) {
		val = 'X';
		reason = "On fire";
	    } else if (pin_internal != 'Z') {
		val = pin_internal;
		reason = "Internal";
	    } else if (pin_external != 'Z') {
		val = pin_external;
		reason = "External";
	    } else {
		if ((pullup_internal + pullup_external) == 0) {
		    val = 'X';
		    reason = "No drive";
		} else if ((pullup_internal + pullup_external) < 0) {
		    val = '0';
		    reason = "Pulled down";		    
		} else {
		    val = '1';
		    reason = "Pulled up";
		} 
	    } 

	    debug( "Pin %s/%i: %c (%s)", pin_type.c_str(), pin_instance, val,
		   reason.c_str());
	}

	int get_value(void)
	{
	    if ((pin_internal != 'Z') && (pin_external != 'Z')) {
		debug("Pin %s/%i set both internally and externally",
		      pin_type.c_str(), pin_instance);
	    }

	    if (pin_internal != 'Z') {
		return pin_internal;
	    } 

	    if (pin_external != 'Z') {
		return pin_external;
	    }

	    /* Both internal and external are "Z".  Return pullup value. */
	    if ((pullup_internal + pullup_external) == 0) {
		return 'X';
	    }
	    if ((pullup_internal + pullup_external) < 0) {
		return '0';
	    } 

	    return '1';
	}

	void set_internal(char val)
	{
	    pin_internal = val;
	    /* Dummy call to check for illegal values */
	    (void) get_value();
	}
	
	void set_external(int val)
	{
	    pin_external = val;
	    /* Dummy call to check for illegal values */
	    (void) get_value();
	}
	
	void set_external_pullup(char val)
	{	
	    pullup_external = val;
	    /* Dummy call to check for illegal values */
	    (void) get_value();
	}
	
	void set_internal_pullup(char val)
	{	
	    pullup_internal = val;
	    /* Dummy call to check for illegal values */
	    (void) get_value();
	}
	
    };
	
    struct ext_pins
    {
	typedef std::map<const std::tuple<std::string, unsigned>, ext_pin*> pin_map_t;
	
	pin_map_t pins;

        ext_pins() {}
	~ext_pins() { pins.clear(); }

	/* print pins */
	void print_pins()
	{
	    for (auto pin : pins) {
		pin.second->print_pin();
	    }
	}

	void ext_pin_set(std::string pin_type, unsigned pin_instance, char val)
	{
	    pins[std::make_tuple(pin_type, pin_instance)]->set_external(val);
	}

	void ext_pin_pullup(std::string pin_type, unsigned pin_instance, int pullup)
	{
	    pins[std::make_tuple(pin_type, pin_instance)]->set_external_pullup(pullup);
	}

	void int_pin_set(std::string pin_type, unsigned pin_instance, char val)
	{
	    pins[std::make_tuple(pin_type, pin_instance)]->set_internal(val);
	}

	void int_pin_pullup(std::string pin_type, unsigned pin_instance, int pullup)
	{
	    pins[std::make_tuple(pin_type, pin_instance)]->set_internal_pullup(pullup);
	}

	int pin_get(std::string pin_type, unsigned pin_instance)
	{
	    return pins[std::make_tuple(pin_type, pin_instance)]->get_value();
	}

	void add_pin(std::string pin_type, unsigned pin_instance)
	{
	    pins[std::make_tuple(pin_type, pin_instance)] =
		new ext_pin(pin_type, pin_instance);
	}
	    
	void del_pin(std::string pin_type, unsigned pin_instance)
	{
	    pins.erase(std::make_tuple(pin_type, pin_instance));
	}
	    
    };

    
}

#endif
