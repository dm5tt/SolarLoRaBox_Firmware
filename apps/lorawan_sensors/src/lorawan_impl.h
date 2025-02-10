#ifndef LORAWAN_IMPL_H
#define LORAWAN_IMPL_H


#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>

#define LORAWAN_IMPL_DEV_EUI			{ 0x00 }  /* MSB! Dummy Key - doesn't work! */
#define LORAWAN_IMPL_JOIN_EUI	  	{ 0x00 }  /* MSB!  Dummy Key - doesn't work! */
#define LORAWAN_IMPL_APP_KEY			{ 0x00 } /* MSB!  Dummy Key - doesn't work! */

int lorawan_impl_init(const struct device *lora_dev);

#endif
