#ifndef DEFAULT_AUTO_COMPLETERS_H
#define DEFAULT_AUTO_COMPLETERS_H

#include "interfaces/IAutoCompleter.h"
#include "interfaces/ICommand.h"
#include "interfaces/ITerminalStream.h"
#include "DirectoryNavigator.h"
#include "ETTypes.h"

namespace EmbeddedTerminal
{
    /// @brief Auto completer for command names
    /// Suggests available command keywords from the terminal
    class CommandCompleter : public IAutoCompleter
    {
    public:
        CommandCompleter(const ETMap<ETString, ICommand *> &commands)
            : commands_(commands) {}

        ETVector<ETString> getSuggestions(const ETString &partial) override
        {
            ETVector<ETString> suggestions;

            // Only match command names if we're at the start (before any space)
            // This is indicated by partial having no path separators
            if (partial.contains('/') || partial.contains('\\'))
            {
                return suggestions; // Don't suggest commands for paths
            }

            for (const auto &pair : commands_)
            {
                if (pair.first.startsWith(partial))
                {
                    suggestions.push_back(pair.first);
                }
            }

            return suggestions;
        }

    private:
        const ETMap<ETString, ICommand *> &commands_;
    };

    /// @brief Auto completer for file paths
    /// Suggests files and directories that match the partial path
    class FilePathCompleter : public IAutoCompleter
    {
    public:
        FilePathCompleter(DirectoryNavigator &navigator)
            : navigator_(navigator) {}

        ETVector<ETString> getSuggestions(const ETString &partial) override
        {
            ETVector<ETString> suggestions;

            // Determine the directory and prefix to search for
            ETString searchDir = "/";
            ETString searchPrefix = partial;

            // Check if partial contains a path separator
            size_t lastSlash = partial.find_last_of('/');
            if (lastSlash != ETString::npos)
            {
                // Path contains directory component
                searchDir = partial.substr(0, lastSlash);
                if (searchDir.empty())
                {
                    searchDir = "/";
                }
                searchPrefix = partial.substr(lastSlash + 1);
            }

            // List contents of the directory
            ETVector<ETString> entries = navigator_.ls(searchDir);

            for (const auto &entry : entries)
            {
                if (entry.startsWith(searchPrefix))
                {
                    // Construct full path for suggestion
                    ETString fullEntry = searchDir;
                    if (!searchDir.endsWith('/'))
                    {
                        fullEntry += "/";
                    }
                    fullEntry += entry;

                    // Add directory indicator if it's a directory
                    if (navigator_.isDirectory(fullEntry))
                    {
                        fullEntry += "/";
                    }

                    suggestions.push_back(fullEntry);
                }
            }

            return suggestions;
        }

    private:
        DirectoryNavigator &navigator_;
    };

    /// @brief Auto completer for directory paths only
    /// Suggests only directories that match the partial path
    class DirectoryCompleter : public IAutoCompleter
    {
    public:
        DirectoryCompleter(DirectoryNavigator &navigator)
            : navigator_(navigator) {}

        ETVector<ETString> getSuggestions(const ETString &partial) override
        {
            ETVector<ETString> suggestions;

            // Determine the directory and prefix to search for
            ETString searchDir = "/";
            ETString searchPrefix = partial;

            // Check if partial contains a path separator
            size_t lastSlash = partial.find_last_of('/');
            if (lastSlash != ETString::npos)
            {
                // Path contains directory component
                searchDir = partial.substr(0, lastSlash);
                if (searchDir.empty())
                {
                    searchDir = "/";
                }
                searchPrefix = partial.substr(lastSlash + 1);
            }

            // List contents of the directory
            ETVector<ETString> entries = navigator_.ls(searchDir);

            for (const auto &entry : entries)
            {
                if (entry.startsWith(searchPrefix))
                {
                    // Construct full path
                    ETString fullPath = searchDir;
                    if (!searchDir.endsWith('/'))
                    {
                        fullPath += "/";
                    }
                    fullPath += entry;

                    // Only include if it's a directory
                    if (navigator_.isDirectory(fullPath))
                    {
                        fullPath += "/";
                        suggestions.push_back(fullPath);
                    }
                }
            }

            return suggestions;
        }

    private:
        DirectoryNavigator &navigator_;
    };

} // namespace EmbeddedTerminal
#endif // DEFAULT_AUTO_COMPLETERS_H
