#ifndef PROVISION_ENTRY_H_
#define PROVISION_ENTRY_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

// generic provision callback
struct generic_provision_callback{
    const char *name;
    int (*callback_ptr)(void);
};

#define GENERIC_PROVISION_CALLBACK_REGISTER(_name, _callback_ptr)               \
	static const STRUCT_SECTION_ITERABLE(generic_provision_callback, _name) = { \
		.name           = STRINGIFY(_name),                                     \
		.callback_ptr   = (_callback_ptr),                                      \
	}

#endif /* PROVISION_ENTRY_H_ */
