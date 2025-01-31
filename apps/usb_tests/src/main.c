/*
 * Class A LoRaWAN sample application
 *
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include <sample_usbd.h>

#define DELAY K_MSEC(1000)

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

const struct device *const uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

static struct usbd_context *sample_usbd;

static int enable_usb_device_next(void)
{
	int err;

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL)
	{
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(sample_usbd))
	{
		err = usbd_enable(sample_usbd);
		if (err)
		{
			LOG_ERR("Failed to enable device support");
			return err;
		}
	}

	LOG_INF("USB device support enabled");

	return 0;
}

int main(void)
{
	int ret;

	if (!device_is_ready(uart_dev))
	{
		LOG_ERR("CDC ACM device not ready");
		return 0;
	}

	ret = enable_usb_device_next();

	if (ret != 0)
	{
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	while (1)
	{
		//	LOG_INF("I'm alive");
		k_sleep(DELAY);
	}
}
