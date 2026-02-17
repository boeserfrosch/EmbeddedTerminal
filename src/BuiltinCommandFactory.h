#ifndef BUILTIN_COMMAND_FACTORY_H
#define BUILTIN_COMMAND_FACTORY_H

#include "Terminal.h"
#include "BuiltinCommandFlags.h"
#include "DirectoryNavigator.h"
#include "interfaces/IFileSystem.h"
#include "interfaces/INetworkInterface.h"
#include "commands/BuiltinCommands.h"

namespace EmbeddedTerminal
{
    /**
     * Factory for creating and registering built-in commands.
     * Owns the lifetime of all built-in commands it creates.
     *
     * Usage:
     *   Terminal terminal(stream);
     *   BuiltinCommandFactory factory;
     *   factory.registerFilesystemCommands(terminal, nav, CMD_LS | CMD_CD);
     *   factory.registerNetworkCommands(terminal, net, CMD_IP);
     *   // factory destructor will clean up all created commands
     */
    class BuiltinCommandFactory
    {
    public:
        BuiltinCommandFactory();
        ~BuiltinCommandFactory();

        // Delete copy operations (class manages dynamic memory)
        BuiltinCommandFactory(const BuiltinCommandFactory &) = delete;
        BuiltinCommandFactory &operator=(const BuiltinCommandFactory &) = delete;

        /**
         * Register filesystem commands (ls, cd, cat, mkdir, rm, rmdir, tail)
         *
         * @param terminal Terminal instance to register commands with
         * @param nav DirectoryNavigator dependency for filesystem operations
         * @param flags Bitflags selecting which commands to register (default: all)
         */
        void registerFilesystemCommands(Terminal &terminal, DirectoryNavigator &nav,
                                        uint32_t flags = CMD_FILESYSTEM_ALL);

        /**
         * Register disk commands (df)
         *
         * @param terminal Terminal instance to register commands with
         * @param nav DirectoryNavigator dependency (provides IFileSystem access)
         * @param flags Bitflags selecting which commands to register (default: all)
         */
        void registerDiskCommands(Terminal &terminal, DirectoryNavigator &nav,
                                  uint32_t flags = CMD_DISK_ALL);

        /**
         * Register network commands (ip, download)
         *
         * @param terminal Terminal instance to register commands with
         * @param net INetworkInterface dependency for network operations
         * @param flags Bitflags selecting which commands to register (default: all)
         */
        void registerNetworkCommands(Terminal &terminal, INetworkInterface &net,
                                     uint32_t flags = CMD_NETWORK_ALL);

        /**
         * Register help command
         *
         * @param terminal Terminal instance to register command with
         */
        void registerHelpCommand(Terminal &terminal);

        /**
         * Convenience method to register all built-in commands
         *
         * @param terminal Terminal instance to register commands with
         * @param nav DirectoryNavigator dependency (provides IFileSystem access)
         * @param net INetworkInterface dependency
         */
        void registerAllCommands(Terminal &terminal, DirectoryNavigator &nav,
                                 INetworkInterface &net);

        /**
         * Deregister filesystem commands from terminal
         * (Commands remain owned by factory until destruction)
         *
         * @param terminal Terminal instance to deregister commands from
         * @param flags Bitflags selecting which commands to deregister (default: all)
         */
        void deregisterFilesystemCommands(Terminal &terminal,
                                          uint32_t flags = CMD_FILESYSTEM_ALL);

        /**
         * Deregister disk commands from terminal
         *
         * @param terminal Terminal instance to deregister commands from
         * @param flags Bitflags selecting which commands to deregister (default: all)
         */
        void deregisterDiskCommands(Terminal &terminal,
                                    uint32_t flags = CMD_DISK_ALL);

        /**
         * Deregister network commands from terminal
         *
         * @param terminal Terminal instance to deregister commands from
         * @param flags Bitflags selecting which commands to deregister (default: all)
         */
        void deregisterNetworkCommands(Terminal &terminal,
                                       uint32_t flags = CMD_NETWORK_ALL);

        /**
         * Deregister help command from terminal
         *
         * @param terminal Terminal instance to deregister command from
         */
        void deregisterHelpCommand(Terminal &terminal);

        /**
         * Deregister all built-in commands from terminal
         *
         * @param terminal Terminal instance to deregister commands from
         */
        void deregisterAllCommands(Terminal &terminal);

    private:
        // Built-in command objects owned by this factory
        ETMap<ETString, ICommand *> _builtinCommands;
    };
}
#endif // BUILTIN_COMMAND_FACTORY_H
