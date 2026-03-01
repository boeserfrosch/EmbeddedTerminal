#ifndef ESP_NETWORK_INTERFACE_H
#define ESP_NETWORK_INTERFACE_H

#if defined(ESP32) || defined(ESP_PLATFORM)

#include "interfaces/INetworkInterface.h"
#include "ETTypes.h"

#if defined(ARDUINO)
#include <WiFi.h>
#include <ETH.h>
#else
#include "esp_system.h"
#include "ping/ping_sock.h"
#endif

namespace EmbeddedTerminal
{
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
#endif

            return result;
        }

        ETString ping(const ETString &target) override
        {
#if defined(ARDUINO)
            IPAddress ip;

            if (!ip.fromString(target.c_str()))
            {
                if (!WiFi.hostByName(target.c_str(), ip))
                {
                    return "Host " + target + " could not be resolved\n";
                }
            }

            int avgTime = WiFi.ping(ip);

            if (avgTime <= 0)
            {
                return "Host " + target + " is not reachable\n";
            }

            ETString result = target + " is reachable:\n";
            result += "  Average time: " + ETString(avgTime) + " ms\n";

            return result;
#else
            return "Ping not yet implemented for pure ESP-IDF framework\n";
#endif
        }

    private:
        ETString interfaceName_;
    };

} // namespace EmbeddedTerminal

#endif // ESP32 || ESP_PLATFORM

#endif // ESP_NETWORK_INTERFACE_H
