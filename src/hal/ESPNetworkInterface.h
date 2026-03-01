#ifndef ESP_NETWORK_INTERFACE_H
#define ESP_NETWORK_INTERFACE_H

/**
 * ESP32/ESP8266 Network Interface Implementation
 *
 * This file is only compiled for ESP32/ESP8266 platforms.
 *
 * Uses native platform ping functionality:
 * - Arduino framework: WiFi.ping() (built-in to ESP32/ESP8266 WiFi library)
 * - ESP-IDF framework: esp_ping API (part of ESP-IDF)
 *
 * Related configurations:
 * - platformio.ini: esp32s3_arduino and esp32s3_espressif environments
 * - .vscode/c_cpp_properties.json: Configure IntelliSense for includes
 *
 * IntelliSense Note: If you see "WiFi.h not found" squiggles, this is expected
 * when analyzing with native environment. The code will compile correctly for
 * ESP32/ESP8266 targets.
 */

#if defined(ESP32) || defined(ESP_PLATFORM)

#include "interfaces/INetworkInterface.h"
#include "ETTypes.h"

#if defined(ARDUINO)
// Arduino framework - WiFi.ping() is available
#include <WiFi.h>
#include <ETH.h>
#else
// Pure ESP-IDF framework - use esp_ping API
#include "esp_system.h"
#include "ping/ping_sock.h"
#endif

namespace EmbeddedTerminal
{
    /**
     * ESP32/ESP8266 Network Interface implementation
     * Supports both WiFi and Ethernet interfaces
     */
    class ESPNetworkInterface : public INetworkInterface
    {
    public:
        explicit ESPNetworkInterface(const ETString &name = "wlan0") : interfaceName_(name) {}
        ~ESPNetworkInterface() override {}

        NetworkInfo info() const override
        {
            NetworkInfo result;

#if defined(ARDUINO)
            if (WiFi.status() == WL_CONNECTED)
            {
                result.name = interfaceName_;
                result.ip = WiFi.localIP().toString().c_str();
                result.mac = WiFi.macAddress().c_str();
                result.netmask = WiFi.subnetMask().toString().c_str();
                result.gateway = WiFi.gatewayIP().toString().c_str();
                result.isUp = true;
            }
#endif // ARDUINO

            return result;
        }

        ETString ping(const ETString &target) override
        {
#if defined(ARDUINO)
            // Arduino framework - use built-in WiFi.ping()
            // Works on both ESP32 and ESP8266
            IPAddress ip;

            // Try to parse as IP address first
            if (!ip.fromString(target.c_str()))
            {
                // If not an IP, try to resolve hostname
                if (!WiFi.hostByName(target.c_str(), ip))
                {
                    return "Host " + target + " could not be resolved\n";
                }
            }

            // Ping the target (return s average time in ms, 0 if failed)
            int avgTime = WiFi.ping(ip);

            if (avgTime <= 0)
            {
                return "Host " + target + " is not reachable\n";
            }

            ETString result = target + " is reachable:\n";
            result += "  Average time: " + ETString(avgTime) + " ms\n";

            return result;
#else
            // Pure ESP-IDF framework - ping not yet implemented
            // TODO: Implement using esp_ping_new_session() and esp_ping_start()
            return "Ping not yet implemented for pure ESP-IDF framework\n";
#endif
        }

    private:
        ETString interfaceName_;
    };

} // namespace EmbeddedTerminal

#endif // ESP32 || ESP_PLATFORM

#endif // ESP_NETWORK_INTERFACE_H
