#ifndef INETWORKINTERFACE_H
#define INETWORKINTERFACE_H

#include "../ETTypes.h"

namespace EmbeddedTerminal
{

    /**
     * @brief Structure representing network interface information.
     */
    struct NetworkInfo
    {
        ETString name;
        ETString ip;
        ETString mac;
        ETString netmask;
        ETString gateway;
        bool isUp = false;

        NetworkInfo(ETString name = "", ETString ip = "", ETString mac = "", ETString netmask = "", ETString gateway = "", bool isUp = false)
            : name(name), ip(ip), mac(mac), netmask(netmask), gateway(gateway), isUp(isUp) {}
    };

    /**
     * @brief Interface for a single network interface (device/media/port).
     */
    class INetworkInterface
    {
    public:
        virtual ~INetworkInterface() {}

        /**
         * @brief Gets information about this network interface.
         * @return NetworkInfo struct containing details about this interface.
         */
        virtual NetworkInfo info() const = 0;

        /**
         * @brief Ping a host (IP address or hostname) from this interface.
         * @param target Host to ping.
         * @return Result string with status and optional statistics.
         */
        virtual ETString ping(const ETString &target) = 0;
    };

    /**
     * @brief Interface for managing multiple network interfaces.
     *
     * Similar to IStorageSystem for IStorageMedia.
     */
    class INetworkSystem
    {
    public:
        virtual ~INetworkSystem() {}

        /**
         * @brief Get all available network interfaces.
         * @return Vector of pointers to INetworkInterface objects.
         */
        virtual ETVector<INetworkInterface *> interfaces() const = 0;

        /**
         * @brief Get a specific network interface by name.
         * @param name Name of the interface.
         * @return Pointer to the INetworkInterface, or nullptr if not found.
         */
        virtual INetworkInterface *getInterface(const ETString &name) const = 0;

        /**
         * @brief Adds a network interface to the system.
         * @param name Name of the interface to add.
         * @param interface Pointer to the INetworkInterface to add.
         * @return True if the interface was added successfully, false if an interface with the same name already exists.
         */
        virtual bool addInterface(const ETString &name, INetworkInterface *interface) = 0;

        /**
         * @brief Removes a network interface from the system by name.
         * @param name Name of the interface to remove.
         * @return True if the interface was removed successfully, false if no interface with the given name exists.
         */
        virtual bool removeInterface(const ETString &name) = 0;

        /**
         * @brief Pings a target host from a specific network interface.
         * @param interfaceName Name of the interface to use for pinging.
         * @param target Host to ping (IP address or hostname).
         * @return Result string with status and optional statistics, or an error message if the interface is not found.
         */
        virtual ETString ping(const ETString &interfaceName, const ETString &target) = 0;

        /**
         * @brief Pings a target host, looking for a default interface to use.
         * @param target Host to ping (IP address or hostname).
         * @return Result string with status and optional statistics, or an error message if no default interface is available.
         */
        virtual ETString ping(const ETString &target) = 0;
    };

} // namespace EmbeddedTerminal

#endif // INETWORKINTERFACE_H
