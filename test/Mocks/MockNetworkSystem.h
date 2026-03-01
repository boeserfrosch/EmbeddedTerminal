#pragma once
#include "interfaces/INetworkInterface.h"
#include "MockNetworkInterface.h"
#include "ETTypes.h"
#include <vector>
#include <map>

using namespace EmbeddedTerminal;

class MockNetworkSystem : public INetworkSystem
{
public:
    // ETVector<MockNetworkInterface *> interfacesList;
    ETMap<ETString, INetworkInterface *> nameMap;

    ~MockNetworkSystem()
    {
        // for (auto iface : interfacesList)
        //     delete iface;
    }

    void addInterface(const ETString &name, const ETString &ip, const ETString &mac, const ETString &netmask, const ETString &gateway, bool isUp)
    {
        NetworkInfo info;
        info.name = name;
        info.ip = ip;
        info.mac = mac;
        info.netmask = netmask;
        info.gateway = gateway;
        info.isUp = isUp;
        auto iface = new MockNetworkInterface(info);
        // interfacesList.push_back(iface);
        nameMap[name] = iface;
    }

    ETVector<INetworkInterface *> interfaces() const override
    {
        ETVector<INetworkInterface *> result;
        for (auto pair : nameMap)
            result.push_back(pair.second);
        return result;
    }

    INetworkInterface *getInterface(const ETString &name) const override
    {
        auto it = nameMap.find(name);
        if (it != nameMap.end())
            return it->second;
        return nullptr;
    }

    bool addInterface(const ETString &name, INetworkInterface *interface) override
    {
        if (nameMap.find(name) != nameMap.end())
            return false; // Interface with this name already exists

        auto info = interface->info();
        if (nameMap.find(info.name) != nameMap.end())
            return false; // Interface with this name already exists
        nameMap[info.name] = interface;
        return true;
    }

    bool removeInterface(const ETString &name) override
    {
        auto it = nameMap.find(name);
        if (it == nameMap.end())
            return false; // No interface with this name exists
        nameMap.erase(it);
        return true;
    }

    ETString ping(const ETString &interfaceName, const ETString &target) override
    {
        auto iface = getInterface(interfaceName);
        if (!iface)
            return "Interface " + interfaceName + " not found\n";
        return iface->ping(target);
    }

    ETString ping(const ETString &target) override
    {
        for (auto pair : nameMap)
        {
            auto iface = pair.second;
            if (iface->info().isUp)
                return iface->ping(target);
        }
        return "No default interface available to ping " + target + "\n";
    }
};
