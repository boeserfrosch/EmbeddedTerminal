#ifndef BUILTIN_COMMAND_FLAGS_H
#define BUILTIN_COMMAND_FLAGS_H

#include <cstdint>

namespace EmbeddedTerminal
{
    // Built-in command name constants
    constexpr const char *CMD_NAME_CAT = "cat";
    constexpr const char *CMD_NAME_CD = "cd";
    constexpr const char *CMD_NAME_DOWNLOAD = "download";
    constexpr const char *CMD_NAME_LS = "ls";
    constexpr const char *CMD_NAME_MKDIR = "mkdir";
    constexpr const char *CMD_NAME_RM = "rm";
    constexpr const char *CMD_NAME_RMDIR = "rmdir";
    constexpr const char *CMD_NAME_TAIL = "tail";
    constexpr const char *CMD_NAME_DF = "df";
    constexpr const char *CMD_NAME_IP = "ip";
    constexpr const char *CMD_NAME_PING = "ping";
    constexpr const char *CMD_NAME_PWD = "pwd";
    constexpr const char *CMD_NAME_HELP = "help";

    // Built-in command flags for selective registration
    typedef uint32_t BuiltinCommand;

    const BuiltinCommand CMD_NONE = 0;
    const BuiltinCommand CMD_CAT = 1 << 0;
    const BuiltinCommand CMD_CD = 1 << 1;
    const BuiltinCommand CMD_DOWNLOAD = 1 << 2;
    const BuiltinCommand CMD_LS = 1 << 3;
    const BuiltinCommand CMD_MKDIR = 1 << 4;
    const BuiltinCommand CMD_RM = 1 << 5;
    const BuiltinCommand CMD_RMDIR = 1 << 6;
    const BuiltinCommand CMD_TAIL = 1 << 7;
    const BuiltinCommand CMD_DF = 1 << 8;
    const BuiltinCommand CMD_IP = 1 << 9;
    const BuiltinCommand CMD_PING = 1 << 10;
    const BuiltinCommand CMD_PWD = 1 << 11;
    const BuiltinCommand CMD_HELP = 1 << 12;

    // Convenience flags
    const BuiltinCommand CMD_FILESYSTEM_ALL = CMD_CAT | CMD_CD | CMD_DOWNLOAD | CMD_LS | CMD_MKDIR | CMD_RM | CMD_RMDIR | CMD_TAIL | CMD_PWD;
    const BuiltinCommand CMD_DISK_ALL = CMD_DF;
    const BuiltinCommand CMD_NETWORK_ALL = CMD_IP | CMD_PING;
    const BuiltinCommand CMD_ALL = CMD_FILESYSTEM_ALL | CMD_DISK_ALL | CMD_NETWORK_ALL | CMD_HELP;
}

#endif // BUILTIN_COMMAND_FLAGS_H
