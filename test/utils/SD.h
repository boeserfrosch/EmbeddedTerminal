#if defined(ARDUINO)

#include <SD_MMC.h>

#elif defined(ESP_PLATFORM) || defined(ESP_32)

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#endif

// Shared SDMMC setup helper

#ifndef SDMMC_PIN_CLK
#define SDMMC_PIN_CLK 14
#endif
#ifndef SDMMC_PIN_CMD
#define SDMMC_PIN_CMD 15
#endif
#ifndef SDMMC_PIN_D0
#define SDMMC_PIN_D0 2
#endif
#ifndef SDMMC_PIN_D1
#define SDMMC_PIN_D1 4
#endif
#ifndef SDMMC_PIN_D2
#define SDMMC_PIN_D2 12
#endif
#ifndef SDMMC_PIN_D3
#define SDMMC_PIN_D3 13
#endif
#ifndef SDMMC_BUS_WIDTH
#define SDMMC_BUS_WIDTH 4
#endif
#ifndef SDMMC_MOUNT_POINT
#define SDMMC_MOUNT_POINT "/sdcard"
#endif
#ifndef SDMMC_MAX_FILES
#define SDMMC_MAX_FILES 5
#endif
#ifndef SDMMC_ALLOCATION_UNIT_SIZE
#define SDMMC_ALLOCATION_UNIT_SIZE (16 * 1024)
#endif
#ifndef SDMMC_FORMAT_IF_MOUNT_FAILED
#define SDMMC_FORMAT_IF_MOUNT_FAILED false
#endif
#ifndef SDMMC_FREQUENCY_KHZ
#define SDMMC_FREQUENCY_KHZ SDMMC_FREQ_HIGHSPEED
#endif
#ifndef SDMMC_HOST_FLAGS
#define SDMMC_HOST_FLAGS (SDMMC_HOST_FLAG_4BIT | SDMMC_HOST_FLAG_DDR)
#endif
#ifndef SDMMC_HOST_IO_VOLTAGE
#define SDMMC_HOST_IO_VOLTAGE 3.3
#endif
#ifndef SDMMC_HOST_COMMAND_TIMEOUT_MS
#define SDMMC_HOST_COMMAND_TIMEOUT_MS 1000
#endif
#ifndef SDMMC_HOST_GET_REAL_FREQ
#define SDMMC_HOST_GET_REAL_FREQ true
#endif
#ifndef SDMMC_HOST_DEINIT_WITH_SLOT
#define SDMMC_HOST_DEINIT_WITH_SLOT false
#endif
#ifndef SDMMC_HOST_DEINIT_ARG
#define SDMMC_HOST_DEINIT_ARG SDMMC_HOST_DEINIT_WITH_SLOT
#endif
#ifndef SDMMC_HOST_DEINIT_FUNC
#define SDMMC_HOST_DEINIT_FUNC sdmmc_host_deinit
#endif
#ifndef SDMMC_HOST_SLOT
#define SDMMC_HOST_SLOT SDMMC_HOST_SLOT_1
#endif

#if defined(ARDUINO)

bool setup_sdmmc()
{
    SD_MMC.setPins(SDMMC_PIN_CLK, SDMMC_PIN_CMD, SDMMC_PIN_D0, SDMMC_PIN_D1, SDMMC_PIN_D2, SDMMC_PIN_D3);
    return SD_MMC.begin("/sdcard", SDMMC_BUS_WIDTH == 1);
}

void teardown_sdmmc()
{
    SD_MMC.end();
}

#elif defined(ESP_PLATFORM) || defined(ESP_32)
sdmmc_card_t *card;
bool setup_sdmmc()
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = SDMMC_FORMAT_IF_MOUNT_FAILED,
        .max_files = SDMMC_MAX_FILES,
        .allocation_unit_size = SDMMC_ALLOCATION_UNIT_SIZE,
        .disk_status_check_enable = true};

    const char mount_point[] = SDMMC_MOUNT_POINT;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT;
    host.max_freq_khz = SDMMC_FREQUENCY_KHZ;
    host.flags = SDMMC_HOST_FLAGS;
    host.io_voltage = SDMMC_HOST_IO_VOLTAGE;
    host.command_timeout_ms = SDMMC_HOST_COMMAND_TIMEOUT_MS;
#if SDMMC_HOST_GET_REAL_FREQ
    host.get_real_freq = [](int slot, int *real_freq) -> esp_err_t
    {
        // For testing purposes, we can just return the requested frequency as the real frequency
        *real_freq = SDMMC_FREQUENCY_KHZ;
        return ESP_OK;
    };
#else
    host.get_real_freq = nullptr;
#endif
#if SDMMC_HOST_DEINIT_ARG
    host.deinit_p = [](int slot) -> esp_err_t
    {
        return SDMMC_HOST_DEINIT_FUNC();
    };
#else
    host.deinit = SDMMC_HOST_DEINIT_FUNC;
#endif

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = (gpio_num_t)SDMMC_BUS_WIDTH;

    slot_config.clk = (gpio_num_t)SDMMC_PIN_CLK;
    slot_config.cmd = (gpio_num_t)SDMMC_PIN_CMD;
    slot_config.d0 = (gpio_num_t)SDMMC_PIN_D0;
#if SDMMC_BUS_WIDTH > 1
    slot_config.d1 = (gpio_num_t)SDMMC_PIN_D1;
    slot_config.d2 = (gpio_num_t)SDMMC_PIN_D2;
    slot_config.d3 = (gpio_num_t)SDMMC_PIN_D3;
#endif
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    auto ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
        }
        else
        {
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
            check_sd_card_pins(&config, pin_count);
#endif
        }
        return false;
    }
    return true;
}

void teardown_sdmmc()
{
    esp_vfs_fat_sdcard_unmount(SDMMC_MOUNT_POINT, card);
}
#endif