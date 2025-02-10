#include <zephyr/device.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h>

#include "lorawan_impl.h"

#define LOG_LEVEL CONFIG_LOG_DEBUG_LEVEL
LOG_MODULE_REGISTER(lorawan_sensor_device);

#define ADC_RESOLUTION 12
#define ADC_CHANNEL_COUNT 2
#define LORAWAN_JOIN_ATTEMPTS 10
#define RETRY_DELAY K_MINUTES(1)
#define SEND_INTERVAL K_MINUTES(25)

static uint16_t sample_buffer[ADC_CHANNEL_COUNT];

/* ADC Configuration */
static const struct adc_channel_cfg channel_cfg[] = {
    {
        .channel_id = 6, // ADC1_IN6 (PA2)
        .gain = ADC_GAIN_1,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 40),
    },
    {
        .channel_id = 7, // ADC1_IN7 (PA3)
        .gain = ADC_GAIN_1,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 40),
    }
};

static struct adc_sequence sequence = {
    .buffer = sample_buffer,
    .buffer_size = sizeof(sample_buffer),
    .resolution = ADC_RESOLUTION,
    .channels = BIT(6) | BIT(7)
};

static int configure_adc(const struct device *adc_dev)
{
    if (!device_is_ready(adc_dev)) {
        LOG_ERR("ADC device not ready");
        return -1;
    }

    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        if (adc_channel_setup(adc_dev, &channel_cfg[i]) < 0) {
            LOG_ERR("Failed to configure ADC channel %d", i);
            return -1;
        }
    }
    return 0;
}

static int join_lorawan(const struct device *lora_dev)
{
    struct lorawan_join_config join_cfg = {
        .mode = LORAWAN_ACT_OTAA,
        .dev_eui = (uint8_t *)LORAWAN_IMPL_DEV_EUI,
        .otaa = {
            .join_eui = (uint8_t *)LORAWAN_IMPL_JOIN_EUI,
            .app_key = (uint8_t *)LORAWAN_IMPL_APP_KEY,
            .nwk_key = (uint8_t *)LORAWAN_IMPL_APP_KEY,
            .dev_nonce = 12345
        }
    };

    LOG_INF("Joining network over OTAA");
    for (int attempts = 0; attempts < LORAWAN_JOIN_ATTEMPTS; attempts++) {
        int ret = lorawan_join(&join_cfg);
        if (ret == 0) {
            return 0;
        }
        LOG_ERR("LoRaWAN join failed: %d - Retrying", ret);
        k_sleep(RETRY_DELAY);
    }
    LOG_ERR("LoRaWAN join failed after multiple attempts.");
    return -1;
}

void main(void)
{
    const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
    lorawan_impl_init(lora_dev);

    if (join_lorawan(lora_dev) < 0) {
        return;
    }

    const struct device *adc_dev = DEVICE_DT_GET(DT_ALIAS(adc));
    if (configure_adc(adc_dev) < 0) {
        return;
    }

    while (1) {
        if (adc_read(adc_dev, &sequence) < 0) {
            LOG_ERR("ADC read failed");
            return;
        }

        for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
            LOG_INF("ADC[%d] = %d", i, sample_buffer[i]);
        }

        int ret = lorawan_send(2, (uint8_t *)sample_buffer, sizeof(sample_buffer), LORAWAN_MSG_UNCONFIRMED);
        if (ret < 0) {
            LOG_ERR("LoRa send failed: %d", ret);
        } else {
            LOG_INF("LoRa message sent");
        }
        
        k_sleep(SEND_INTERVAL);
    }
}
