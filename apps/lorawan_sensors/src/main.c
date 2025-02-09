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
#include <zephyr/drivers/adc.h>
#include <zephyr/pm/pm.h>

#include "lorawan_impl.h"

#define DELAY K_MSEC(10000)

#define LOG_LEVEL CONFIG_LOG_DEBUG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_sensor_device);

static uint16_t sample_buffer[2]; // Buffer für zwei Kanäle

/* ADC */
static const struct adc_channel_cfg channel_cfg[] = {
	{
		.channel_id = 6, // ADC1_IN6 (PA2)
		.gain = ADC_GAIN_1,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	},
	{
		.channel_id = 7, // ADC1_IN7 (PA3)
		.gain = ADC_GAIN_1,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time =  ADC_ACQ_TIME_DEFAULT,
	}
};

struct adc_sequence sequence = {
	.buffer = sample_buffer,
	.buffer_size = sizeof(sample_buffer),
	.resolution = 12, // 12-bit ADC
	.channels = BIT(6) | BIT(7)
};

int main(void)
{
	int ret;

	/* LoRaWAN Configuration */
	const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	lorawan_impl_init(lora_dev);

	struct lorawan_join_config join_cfg;
	uint8_t dev_eui[] = LORAWAN_IMPL_DEV_EUI;
	uint8_t join_eui[] = LORAWAN_IMPL_JOIN_EUI;
	uint8_t app_key[] = LORAWAN_IMPL_APP_KEY;

	join_cfg.mode = LORAWAN_ACT_OTAA;
	join_cfg.dev_eui = dev_eui;
	join_cfg.otaa.join_eui = join_eui;
	join_cfg.otaa.app_key = app_key;
	join_cfg.otaa.nwk_key = app_key;
	join_cfg.otaa.dev_nonce = 12345;

	/* ADC Configuration */
	const struct device *adc_dev = DEVICE_DT_GET(DT_ALIAS(adc));
	if (!device_is_ready(adc_dev)) {
		LOG_ERR("ADC device not ready");
		return -1;
	}

	for (int i = 0; i < 2; i++) {
		if (adc_channel_setup(adc_dev, &channel_cfg[i]) < 0) {
			LOG_ERR("Failed to configure ADC channel %d", i);
			return -1;
		}
	}

	/* Join LoRaWAN Network */
	LOG_INF("Joining network over OTAA");
	unsigned int attempts = 0;
	do {
		if (attempts == 10) {
			LOG_ERR("lorawan_join_network failed after multiple attempts.");
			return -1;
		}
		ret = lorawan_join(&join_cfg);
		if (ret != 0) {
			LOG_ERR("lorawan_join_network failed: %d - Retrying", ret);
			k_sleep(K_MINUTES(1));
		}
		attempts++;
	} while (ret != 0);

	while (1) {
		/* Read ADC values */
		if (adc_read(adc_dev, &sequence) < 0) {
			LOG_ERR("ADC read failed");
			return -1;
		}

		/* Send ADC data over LoRa */
		ret = lorawan_send(1, (uint8_t *)sample_buffer, sizeof(sample_buffer), LORAWAN_MSG_UNCONFIRMED);
		if (ret < 0) {
			LOG_ERR("LoRa send failed: %d", ret);
		} else {
			LOG_INF("LoRa message sent");
		}
		k_sleep(K_MINUTES(25));
	}

	return 0;
}