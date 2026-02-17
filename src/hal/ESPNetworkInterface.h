#ifndef ESP_NETWORK_INTERFACE_H
#define ESP_NETWORK_INTERFACE_H

#if defined(ESP32) || defined(ESP_PLATFORM)

#include "interfaces/INetworkInterface.h"
#include "ETTypes.h"
#include <WiFi.h>
#include <ETH.h>

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
    };

} // namespace EmbeddedTerminal

#endif // ESP32 || ESP_PLATFORM

#endif // ESP_NETWORK_INTERFACE_H
