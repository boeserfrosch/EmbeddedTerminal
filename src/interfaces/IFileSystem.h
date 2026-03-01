// Platform-independent file system interface
#ifndef IFILESYSTEM_H
#define IFILESYSTEM_H

#include "ETTypes.h"
#include "ETFile.h"
#include "Path.h"
namespace EmbeddedTerminal
{

    /**
     * @brief Interface for file system abstraction.
     */
    class IFileSystem
    {
    public:
        virtual ~IFileSystem() {}

        /**
         * @brief Opens a file with the given path and mode.
         * @param path The path to the file to open.
         * @param mode The mode to open the file in (e.g., "r" for read, "w" for write).
         * @param create Whether to create the file if it does not exist (default is false).
         * @return An ETFile object representing the opened file, or an empty ETFile if the file could not be opened.
         */
        virtual ETFile open(const Path &path, const char *mode = FILE_MODE_READ, const bool create = false) = 0;

        /**
         * @brief Checks if a file or directory exists at the given path.
         * @param path The path to check for existence.
         * @return True if the file or directory exists, false otherwise.
         */
        virtual bool exists(const Path &path) = 0;

        /**
         * @brief Checks if the path is a directory.
         * @param path The path to check.
         * @return True if the path is a directory, false otherwise.
         */
        virtual bool isDirectory(const Path &path) = 0;

        /**
         * @brief Checks if the file or directory at the given path is empty.
         * @param path The path to check.
         * @return True if the file or directory is empty, false otherwise.
         */
        virtual bool isEmpty(const Path &path) = 0;

        /**
         * @brief Removes the file at the given path.
         * @param path The path to the file to remove.
         * @return True if the file was successfully removed, false otherwise.
         */
        virtual bool remove(const Path &path) = 0;

        /**
         * @brief Creates a directory at the given path.
         * @param path The path to the directory to create.
         * @return True if the directory was successfully created or already exists, false otherwise.
         */
        virtual bool mkdir(const Path &path) = 0;

        /**
         * @brief Removes the directory at the given path.
         * @param path The path to the directory to remove.
         * @return True if the directory was successfully removed, false otherwise.
         */
        virtual bool rmdir(const Path &path) = 0;

        /**
         * @brief Lists the contents of the directory at the given path.
         * @param path The path to the directory to list. If the path is a file, it should return a vector containing just that file name.
         * @param prefix The prefix to apply to the list of files and directories. If empty, no filtering is applied. The prefix just checks the prefix of the file/directory name, so it will return all entries that start with the prefix string.
         * @return A vector of Path objects representing the paths of the files and directories contained in the specified directory, or a vector containing just the file path if the path is a file. Returns an empty vector if the path does not exist or is not a directory.
         */
        virtual ETVector<Path> list(const Path &path, const ETString &prefix = "") const = 0;
    };
}
#endif // IFILESYSTEM_H
