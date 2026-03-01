#include "BuiltinCommandFactory.h"

namespace EmbeddedTerminal
{
    BuiltinCommandFactory::BuiltinCommandFactory()
    {
    }

    BuiltinCommandFactory::~BuiltinCommandFactory()
    {
        // Clean up all owned command objects
        for (auto &pair : builtinCommands_)
        {
            delete pair.second;
        }
        builtinCommands_.clear();
    }

    void BuiltinCommandFactory::registerFilesystemCommands(Terminal &terminal, DirectoryNavigator &nav, uint32_t flags)
    {
        if (flags & CMD_CAT)
        {
            auto it = builtinCommands_.find(CMD_NAME_CAT);
            if (it == builtinCommands_.end())
            {
                cmd::cat *catCmd = new cmd::cat(nav);
                if (catCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_CAT] = catCmd;
                }
            }

            if (builtinCommands_.find(CMD_NAME_CAT) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_CAT, builtinCommands_[CMD_NAME_CAT]);
            }
        }

        if (flags & CMD_CD)
        {
            auto it = builtinCommands_.find(CMD_NAME_CD);
            if (it == builtinCommands_.end())
            {
                cmd::cd *cdCmd = new cmd::cd(nav);
                if (cdCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_CD] = cdCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_CD) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_CD, builtinCommands_[CMD_NAME_CD]);
            }
        }

        if (flags & CMD_DOWNLOAD)
        {
            auto it = builtinCommands_.find(CMD_NAME_DOWNLOAD);
            if (it == builtinCommands_.end())
            {
                cmd::download *downloadCmd = new cmd::download(nav);
                if (downloadCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_DOWNLOAD] = downloadCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_DOWNLOAD) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_DOWNLOAD, builtinCommands_[CMD_NAME_DOWNLOAD]);
            }
        }

        if (flags & CMD_LS)
        {
            auto it = builtinCommands_.find(CMD_NAME_LS);
            if (it == builtinCommands_.end())
            {
                cmd::ls *lsCmd = new cmd::ls(nav);
                if (lsCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_LS] = lsCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_LS) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_LS, builtinCommands_[CMD_NAME_LS]);
            }
        }

        if (flags & CMD_MKDIR)
        {
            auto it = builtinCommands_.find(CMD_NAME_MKDIR);
            if (it == builtinCommands_.end())
            {
                cmd::mkdir *mkdirCmd = new cmd::mkdir(nav);
                if (mkdirCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_MKDIR] = mkdirCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_MKDIR) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_MKDIR, builtinCommands_[CMD_NAME_MKDIR]);
            }
        }

        if (flags & CMD_RM)
        {
            auto it = builtinCommands_.find(CMD_NAME_RM);
            if (it == builtinCommands_.end())
            {
                cmd::rm *rmCmd = new cmd::rm(nav);
                if (rmCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_RM] = rmCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_RM) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_RM, builtinCommands_[CMD_NAME_RM]);
            }
        }

        if (flags & CMD_RMDIR)
        {
            auto it = builtinCommands_.find(CMD_NAME_RMDIR);
            if (it == builtinCommands_.end())
            {
                cmd::rmdir *rmdirCmd = new cmd::rmdir(nav);
                if (rmdirCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_RMDIR] = rmdirCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_RMDIR) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_RMDIR, builtinCommands_[CMD_NAME_RMDIR]);
            }
        }

        if (flags & CMD_PWD)
        {
            auto it = builtinCommands_.find(CMD_NAME_PWD);
            if (it == builtinCommands_.end())
            {
                cmd::pwd *pwdCmd = new cmd::pwd(nav);
                if (pwdCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_PWD] = pwdCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_PWD) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_PWD, builtinCommands_[CMD_NAME_PWD]);
            }
        }

        if (flags & CMD_TAIL)
        {
            auto it = builtinCommands_.find(CMD_NAME_TAIL);
            if (it == builtinCommands_.end())
            {
                cmd::tail *tailCmd = new cmd::tail(nav);
                if (tailCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_TAIL] = tailCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_TAIL) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_TAIL, builtinCommands_[CMD_NAME_TAIL]);
            }
        }
        if (flags & CMD_XXD)
        {
            auto it = builtinCommands_.find(CMD_NAME_XXD);
            if (it == builtinCommands_.end())
            {
                cmd::xxd *xxdCmd = new cmd::xxd(nav);
                if (xxdCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_XXD] = xxdCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_XXD) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_XXD, builtinCommands_[CMD_NAME_XXD]);
            }
        }
    }

    void BuiltinCommandFactory::registerDiskCommands(Terminal &terminal, DirectoryNavigator &nav, uint32_t flags)
    {
        if (flags & CMD_DF)
        {
            auto it = builtinCommands_.find(CMD_NAME_DF);
            if (it == builtinCommands_.end())
            {
                cmd::df *dfCmd = new cmd::df(*nav.getStorageSystem());
                if (dfCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_DF] = dfCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_DF) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_DF, builtinCommands_[CMD_NAME_DF]);
            }
        }
    }

    void BuiltinCommandFactory::registerNetworkCommands(Terminal &terminal, INetworkSystem &net, uint32_t flags)
    {
        if (flags & CMD_IP)
        {
            auto it = builtinCommands_.find(CMD_NAME_IP);
            if (it == builtinCommands_.end())
            {
                cmd::ip *ipCmd = new cmd::ip(net);
                if (ipCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_IP] = ipCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_IP) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_IP, builtinCommands_[CMD_NAME_IP]);
            }
        }

        if (flags & CMD_PING)
        {
            auto it = builtinCommands_.find(CMD_NAME_PING);
            if (it == builtinCommands_.end())
            {
                cmd::ping *pingCmd = new cmd::ping(net);
                if (pingCmd != nullptr)
                {
                    builtinCommands_[CMD_NAME_PING] = pingCmd;
                }
            }
            if (builtinCommands_.find(CMD_NAME_PING) != builtinCommands_.end())
            {
                terminal.registerCommand(CMD_NAME_PING, builtinCommands_[CMD_NAME_PING]);
            }
        }
    }

    void BuiltinCommandFactory::registerHelpCommand(Terminal &terminal)
    {
        auto it = builtinCommands_.find(CMD_NAME_HELP);
        if (it == builtinCommands_.end())
        {
            cmd::help *helpCmd = new cmd::help(terminal);
            if (helpCmd != nullptr)
            {
                builtinCommands_[CMD_NAME_HELP] = helpCmd;
            }
        }

        if (builtinCommands_.find(CMD_NAME_HELP) != builtinCommands_.end())
        {
            terminal.registerCommand(CMD_NAME_HELP, builtinCommands_[CMD_NAME_HELP]);
        }
    }

    void BuiltinCommandFactory::registerAllCommands(Terminal &terminal, DirectoryNavigator &nav,
                                                    INetworkSystem &net)
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
