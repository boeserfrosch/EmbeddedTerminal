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
        ESPNetworkInterface() {}
        ~ESPNetworkInterface() override {}

        ETMap<ETString, NetworkInfo> getInterfaces() const override
        {
            ETMap<ETString, NetworkInfo> interfaces;

#if defined(ARDUINO)
            // Check WiFi interface (Arduino framework)
            if (WiFi.status() == WL_CONNECTED)
            {
                NetworkInfo wifiInfo;
                wifiInfo.name = "wlan0";
                wifiInfo.ip = WiFi.localIP().toString().c_str();
                wifiInfo.mac = WiFi.macAddress().c_str();
                wifiInfo.netmask = WiFi.subnetMask().toString().c_str();
                wifiInfo.gateway = WiFi.gatewayIP().toString().c_str();
                wifiInfo.isUp = true;
                interfaces["wlan0"] = wifiInfo;
            }

#ifdef ETH_PHY_TYPE
            // Check Ethernet interface (if available)
            if (ETH.linkUp())
            {
                NetworkInfo ethInfo;
                ethInfo.name = "eth0";
                ethInfo.ip = ETH.localIP().toString().c_str();
                ethInfo.mac = ETH.macAddress().c_str();
                ethInfo.netmask = ETH.subnetMask().toString().c_str();
                ethInfo.gateway = ETH.gatewayIP().toString().c_str();
                ethInfo.isUp = true;
                interfaces["eth0"] = ethInfo;
            }
#endif
#endif // ARDUINO

            return interfaces;
        }

        NetworkInfo getInterface(const ETString &name) const override
        {
            NetworkInfo info;

#if defined(ARDUINO)
            if (name == "wlan0" && WiFi.status() == WL_CONNECTED)
            {
                info.name = "wlan0";
                info.ip = WiFi.localIP().toString().c_str();
                info.mac = WiFi.macAddress().c_str();
                info.netmask = WiFi.subnetMask().toString().c_str();
                info.gateway = WiFi.gatewayIP().toString().c_str();
                info.isUp = true;
            }
#ifdef ETH_PHY_TYPE
            else if (name == "eth0" && ETH.linkUp())
            {
                info.name = "eth0";
                info.ip = ETH.localIP().toString().c_str();
                info.mac = ETH.macAddress().c_str();
                info.netmask = ETH.subnetMask().toString().c_str();
                info.gateway = ETH.gatewayIP().toString().c_str();
                info.isUp = true;
            }
#endif
#endif // ARDUINO

            return info;
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

            // Ping the target (returns average time in ms, 0 if failed)
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
    };

} // namespace EmbeddedTerminal

#endif // ESP32 || ESP_PLATFORM

#endif // ESP_NETWORK_INTERFACE_H

