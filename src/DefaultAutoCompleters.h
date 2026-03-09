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
            // Cleanup partial path, change windows backslashes to slashes
            Path cleanedPartial = Path(partial);

            Path basePath = cleanedPartial.getBasePath();
            ETString name = cleanedPartial.getName();

            ETVector<EmbeddedTerminal::Path> suggestions = navigator_.ls(basePath, name);
            Path absoluteBasePath = navigator_.pwd(basePath);

            // Append base path to suggestions
            for (auto &s : suggestions)
            {
                s = absoluteBasePath + s; // Keep absolute base path
            }

            ETVector<ETString> stringSuggestions;
            bool isPartialAbsolute = !partial.empty() && (partial[0] == '/');
            for (const auto &s : suggestions)
            {
                ETString suggestionStr = ETString(s.c_str()) + (navigator_.isDirectory(s) ? "/" : "");
                if (!isPartialAbsolute && suggestionStr.length() > 0 && suggestionStr[0] == '/')
                {
                    suggestionStr = suggestionStr.substr(1);
                }
                stringSuggestions.push_back(suggestionStr);
            }
            return sort(stringSuggestions);
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
            ETVector<ETString> stringSuggestions;

            Path cleanedPartial = Path(partial);
            // Determine the directory and prefix to search for
            Path searchDir = cleanedPartial.getBasePath();
            ETString searchPrefix = cleanedPartial.getName();

            // List contents of the directory
            ETVector<Path> entries = navigator_.ls(searchDir, searchPrefix);
            const Path absoluteSearchDir = navigator_.pwd(searchDir);

            for (const auto &entry : entries)
            {
                if (navigator_.isDirectory(entry))
                {
                    Path suggestionPath = absoluteSearchDir + entry;
                    stringSuggestions.push_back(ETString(suggestionPath.c_str()) + "/");
                }
            }

            return sort(stringSuggestions);
        }

    private:
        DirectoryNavigator &navigator_;
    };

} // namespace EmbeddedTerminal
#endif // DEFAULT_AUTO_COMPLETERS_H
