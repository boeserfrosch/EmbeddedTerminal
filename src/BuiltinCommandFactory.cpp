#include "BuiltinCommandFactory.h"

namespace EmbeddedTerminal
{
    BuiltinCommandFactory::BuiltinCommandFactory()
    {
    }

    BuiltinCommandFactory::~BuiltinCommandFactory()
    {
        // Clean up all owned command objects
        for (auto &pair : _builtinCommands)
        {
            delete pair.second;
        }
        _builtinCommands.clear();
    }

    void BuiltinCommandFactory::registerFilesystemCommands(Terminal &terminal, DirectoryNavigator &nav, uint32_t flags)
    {
        if (flags & CMD_CAT)
        {
            auto it = _builtinCommands.find(CMD_NAME_CAT);
            if (it == _builtinCommands.end())
            {
                cmd::cat *catCmd = new cmd::cat(nav);
                if (catCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_CAT] = catCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_CAT) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_CAT, _builtinCommands[CMD_NAME_CAT]);
            }
        }

        if (flags & CMD_CD)
        {
            auto it = _builtinCommands.find(CMD_NAME_CD);
            if (it == _builtinCommands.end())
            {
                cmd::cd *cdCmd = new cmd::cd(nav);
                if (cdCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_CD] = cdCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_CD) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_CD, _builtinCommands[CMD_NAME_CD]);
            }
        }

        if (flags & CMD_DOWNLOAD)
        {
            auto it = _builtinCommands.find(CMD_NAME_DOWNLOAD);
            if (it == _builtinCommands.end())
            {
                cmd::download *downloadCmd = new cmd::download(nav);
                if (downloadCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_DOWNLOAD] = downloadCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_DOWNLOAD) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_DOWNLOAD, _builtinCommands[CMD_NAME_DOWNLOAD]);
            }
        }

        if (flags & CMD_LS)
        {
            auto it = _builtinCommands.find(CMD_NAME_LS);
            if (it == _builtinCommands.end())
            {
                cmd::ls *lsCmd = new cmd::ls(nav);
                if (lsCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_LS] = lsCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_LS) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_LS, _builtinCommands[CMD_NAME_LS]);
            }
        }

        if (flags & CMD_MKDIR)
        {
            auto it = _builtinCommands.find(CMD_NAME_MKDIR);
            if (it == _builtinCommands.end())
            {
                cmd::mkdir *mkdirCmd = new cmd::mkdir(nav);
                if (mkdirCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_MKDIR] = mkdirCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_MKDIR) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_MKDIR, _builtinCommands[CMD_NAME_MKDIR]);
            }
        }

        if (flags & CMD_RM)
        {
            auto it = _builtinCommands.find(CMD_NAME_RM);
            if (it == _builtinCommands.end())
            {
                cmd::rm *rmCmd = new cmd::rm(nav);
                if (rmCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_RM] = rmCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_RM) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_RM, _builtinCommands[CMD_NAME_RM]);
            }
        }

        if (flags & CMD_RMDIR)
        {
            auto it = _builtinCommands.find(CMD_NAME_RMDIR);
            if (it == _builtinCommands.end())
            {
                cmd::rmdir *rmdirCmd = new cmd::rmdir(nav);
                if (rmdirCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_RMDIR] = rmdirCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_RMDIR) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_RMDIR, _builtinCommands[CMD_NAME_RMDIR]);
            }
        }

        if (flags & CMD_PWD)
        {
            auto it = _builtinCommands.find(CMD_NAME_PWD);
            if (it == _builtinCommands.end())
            {
                cmd::pwd *pwdCmd = new cmd::pwd(nav);
                if (pwdCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_PWD] = pwdCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_PWD) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_PWD, _builtinCommands[CMD_NAME_PWD]);
            }
        }

        if (flags & CMD_TAIL)
        {
            auto it = _builtinCommands.find(CMD_NAME_TAIL);
            if (it == _builtinCommands.end())
            {
                cmd::tail *tailCmd = new cmd::tail(nav);
                if (tailCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_TAIL] = tailCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_TAIL) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_TAIL, _builtinCommands[CMD_NAME_TAIL]);
            }
        }
        if (flags & CMD_XXD)
        {
            auto it = _builtinCommands.find(CMD_NAME_XXD);
            if (it == _builtinCommands.end())
            {
                cmd::xxd *xxdCmd = new cmd::xxd(nav);
                if (xxdCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_XXD] = xxdCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_XXD) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_XXD, _builtinCommands[CMD_NAME_XXD]);
            }
        }
    }

    void BuiltinCommandFactory::registerDiskCommands(Terminal &terminal, DirectoryNavigator &nav, uint32_t flags)
    {
        if (flags & CMD_DF)
        {
            auto it = _builtinCommands.find(CMD_NAME_DF);
            if (it == _builtinCommands.end())
            {
                cmd::df *dfCmd = new cmd::df(*nav.getFileSystem());
                if (dfCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_DF] = dfCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_DF) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_DF, _builtinCommands[CMD_NAME_DF]);
            }
        }
    }

    void BuiltinCommandFactory::registerNetworkCommands(Terminal &terminal, INetworkInterface &net, uint32_t flags)
    {
        if (flags & CMD_IP)
        {
            auto it = _builtinCommands.find(CMD_NAME_IP);
            if (it == _builtinCommands.end())
            {
                cmd::ip *ipCmd = new cmd::ip(net);
                if (ipCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_IP] = ipCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_IP) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_IP, _builtinCommands[CMD_NAME_IP]);
            }
        }

        if (flags & CMD_PING)
        {
            auto it = _builtinCommands.find(CMD_NAME_PING);
            if (it == _builtinCommands.end())
            {
                cmd::ping *pingCmd = new cmd::ping(net);
                if (pingCmd != nullptr)
                {
                    _builtinCommands[CMD_NAME_PING] = pingCmd;
                }
            }
            if (_builtinCommands.find(CMD_NAME_PING) != _builtinCommands.end())
            {
                terminal.registerCommand(CMD_NAME_PING, _builtinCommands[CMD_NAME_PING]);
            }
        }
    }

    void BuiltinCommandFactory::registerHelpCommand(Terminal &terminal)
    {
        auto it = _builtinCommands.find(CMD_NAME_HELP);
        if (it == _builtinCommands.end())
        {
            cmd::help *helpCmd = new cmd::help(terminal);
            if (helpCmd != nullptr)
            {
                _builtinCommands[CMD_NAME_HELP] = helpCmd;
            }
        }
        if (_builtinCommands.find(CMD_NAME_HELP) != _builtinCommands.end())
        {
            terminal.registerCommand(CMD_NAME_HELP, _builtinCommands[CMD_NAME_HELP]);
        }
    }

    void BuiltinCommandFactory::registerAllCommands(Terminal &terminal, DirectoryNavigator &nav,
                                                    INetworkInterface &net)
    {
        registerFilesystemCommands(terminal, nav, CMD_FILESYSTEM_ALL);
        registerDiskCommands(terminal, nav, CMD_DISK_ALL);
        registerNetworkCommands(terminal, net, CMD_NETWORK_ALL);
        registerHelpCommand(terminal);
    }

    void BuiltinCommandFactory::deregisterFilesystemCommands(Terminal &terminal, uint32_t flags)
    {
        if (flags & CMD_CAT)
            terminal.deregisterCommand(CMD_NAME_CAT);
        if (flags & CMD_CD)
            terminal.deregisterCommand(CMD_NAME_CD);
        if (flags & CMD_DOWNLOAD)
            terminal.deregisterCommand(CMD_NAME_DOWNLOAD);
        if (flags & CMD_LS)
            terminal.deregisterCommand(CMD_NAME_LS);
        if (flags & CMD_MKDIR)
            terminal.deregisterCommand(CMD_NAME_MKDIR);
        if (flags & CMD_RM)
            terminal.deregisterCommand(CMD_NAME_RM);
        if (flags & CMD_RMDIR)
            terminal.deregisterCommand(CMD_NAME_RMDIR);
        if (flags & CMD_TAIL)
            terminal.deregisterCommand(CMD_NAME_TAIL);
        if (flags & CMD_XXD)
            terminal.deregisterCommand(CMD_NAME_XXD);
    }

    void BuiltinCommandFactory::deregisterDiskCommands(Terminal &terminal, uint32_t flags)
    {
        if (flags & CMD_DF)
            terminal.deregisterCommand(CMD_NAME_DF);
    }

    void BuiltinCommandFactory::deregisterNetworkCommands(Terminal &terminal, uint32_t flags)
    {
        if (flags & CMD_IP)
            terminal.deregisterCommand(CMD_NAME_IP);
        if (flags & CMD_PING)
            terminal.deregisterCommand(CMD_NAME_PING);
    }

    void BuiltinCommandFactory::deregisterHelpCommand(Terminal &terminal)
    {
        terminal.deregisterCommand(CMD_NAME_HELP);
    }

    void BuiltinCommandFactory::deregisterAllCommands(Terminal &terminal)
    {
        deregisterFilesystemCommands(terminal, CMD_FILESYSTEM_ALL);
        deregisterDiskCommands(terminal, CMD_DISK_ALL);
        deregisterNetworkCommands(terminal, CMD_NETWORK_ALL);
        deregisterHelpCommand(terminal);
    }

} // namespace EmbeddedTerminal
