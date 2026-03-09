#include <unity.h>

#include "hal/common/StorageMediaAdapter.h"
#if !defined(ESP_PLATFORM) && !defined(ESP32) && !defined(ARDUINO)
#include "hal/native/NativeSuggestedStorageMedia.h"
#endif
#include "StorageSystem.h"
#include "../Mocks/native/MockFileSystem.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

void test_storage_media_adapter_properties(void)
{
    MockFileSystem fs;
    StorageMediaAdapter media("mock", &fs, true, 1024, 128, 1024, 896);

    TEST_ASSERT_EQUAL_STRING("mock", media.name());
    TEST_ASSERT_EQUAL_PTR(&fs, media.fileSystem());
    TEST_ASSERT_TRUE(media.isAvailable());
    TEST_ASSERT_EQUAL(1024, media.totalBytes());
    TEST_ASSERT_EQUAL(128, media.usedBytes());
    TEST_ASSERT_EQUAL(1024, media.capacity());
    TEST_ASSERT_EQUAL(896, media.freeBytes());
}

void test_storage_media_adapter_setters(void)
{
    MockFileSystem fs;
    StorageMediaAdapter media("mock", &fs);

    media.setAvailable(false);
    media.setTotalBytes(2048);
    media.setUsedBytes(256);
    media.setCapacity(4096);
    media.setFreeBytes(3840);

    TEST_ASSERT_FALSE(media.isAvailable());
    TEST_ASSERT_EQUAL(2048, media.totalBytes());
    TEST_ASSERT_EQUAL(256, media.usedBytes());
    TEST_ASSERT_EQUAL(4096, media.capacity());
    TEST_ASSERT_EQUAL(3840, media.freeBytes());
}

#if !defined(ESP_PLATFORM) && !defined(ESP32) && !defined(ARDUINO) && defined(__cplusplus) && __cplusplus >= 201703L
void test_native_suggested_storage_media_reports_space(void)
{
    NativeSuggestedStorageMedia media("native-test", ".");

    TEST_ASSERT_EQUAL_STRING("native-test", media.name());
    TEST_ASSERT_NOT_NULL(media.fileSystem());
    TEST_ASSERT_TRUE(media.isAvailable());

    auto total = media.totalBytes();
    auto used = media.usedBytes();
    auto free = media.freeBytes();

    TEST_ASSERT_TRUE(total >= used);
    TEST_ASSERT_TRUE(total >= free);
}

void test_native_suggested_storage_media_mounts_in_storage_system(void)
{
    StorageSystem storage;
    NativeSuggestedStorageMedia media("native", ".");

    TEST_ASSERT_TRUE(storage.mountMedia(&media, ""));
    TEST_ASSERT_NOT_NULL(storage.getMedia("native"));
    TEST_ASSERT_TRUE(storage.exists("."));
}
#endif

int process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_storage_media_adapter_properties);
    RUN_TEST(test_storage_media_adapter_setters);
#if !defined(ESP_PLATFORM) && !defined(ESP32) && !defined(ARDUINO) && defined(__cplusplus) && __cplusplus >= 201703L
    RUN_TEST(test_native_suggested_storage_media_reports_space);
    RUN_TEST(test_native_suggested_storage_media_mounts_in_storage_system);
#endif
    return UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    process_tests();
}
#elif defined(ARDUINO)
void setup()
{
    delay(1000);
    process_tests();
}
void loop() {}
#else
int main()
{
    return process_tests();
}
#endif
