#ifndef ESP_NETWORK_INTERFACE_H
#define ESP_NETWORK_INTERFACE_H

/**
 * ESP32/ESP8266 Network Interface Implementation
 *
 * This file is only compiled for ESP32/ESP8266 platforms with Arduino framework.
 *
 * Related configurations:
 * - platformio.ini: esp32s3_arduino environment includes ESP32 Ping library
 * - .vscode/c_cpp_properties.json: Configure IntelliSense for Arduino includes
 *
 * IntelliSense Note: If you see "WiFi.h not found" squiggles, this is expected
 * when analyzing with native environment. The code will compile correctly for
 * ESP32/ESP8266 targets. To fix squiggles, configure your VS Code intelliSense
 * to use the Arduino include paths.
 */

#if defined(ESP32) || defined(ESP_PLATFORM)

#include "interfaces/INetworkInterface.h"
#include "ETTypes.h"
#include <WiFi.h>
#include <ETH.h>

// ESP32Ping is only available when building with Arduino framework
// (esp32s3_arduino environment - see platformio.ini)
#if defined(ESP32)
#include <ESP32Ping.h>
#endif

namespace EmbeddedTerminal
{
    /**
     * ESP32 Network Interface implementation
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

            // Check WiFi interface
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

// Check Ethernet interface (if available)
#ifdef ETH_PHY_TYPE
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

            return interfaces;
        }

        NetworkInfo getInterface(const ETString &name) const override
        {
            NetworkInfo info;

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

            return info;
        }

        ETString ping(const ETString &target) override
        {
#if defined(ESP32)
            // ESP32-specific implementation using ESP32Ping library
            const int pingCount = 3;
            bool pingable = Ping.ping(target.c_str(), pingCount);

            if (!pingable)
            {
                return "Host " + target + " is not reachable\n";
            }

            ETString result = target + " pinged " + ETString(pingCount) + " times:\n";
            result += "  Average time: " + ETString((int)Ping.averageTime()) + " ms\n";
            result += "  Min time: " + ETString((int)Ping.minTime()) + " ms\n";
            result += "  Max time: " + ETString((int)Ping.maxTime()) + " ms\n";

            return result;
#else
            // ESP8266 or other ESP platform with WiFi.ping()
            const int pingCount = 3;
            int avgTime = WiFi.ping(target.c_str());

            if (avgTime == 0)
            {
                return "Host " + target + " is not reachable\n";
            }

            ETString result = target + " pinged " + ETString(pingCount) + " times:\n";
            result += "  Average time: " + ETString(avgTime) + " ms\n";

            return result;
#endif
        }
    };

} // namespace EmbeddedTerminal

#endif // ESP32 || ESP_PLATFORM

#endif // ESP_NETWORK_INTERFACE_H
