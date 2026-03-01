#ifndef IDIRECTORYNAVIGATOR_H
#define IDIRECTORYNAVIGATOR_H

#include "IStorage.h"
#include "../ETTypes.h"
#include "Path.h"

namespace EmbeddedTerminal
{
    class IDirectoryNavigator
    {
    public:
        virtual ~IDirectoryNavigator() = default;

        /**
         * @brief Opens a file at the specified path with the given mode. The path can be absolute or relative to the current directory. If create is true, it will create the file if it does not exist (only for write modes).
         * @param path The path to the file to open.
         * @param mode The mode to open the file in (e.g., "r" for read, "w" for write).
         * @param create Whether to create the file if it does not exist (only applicable for write modes).
         * @return An ETFile object representing the opened file, or an invalid ETFile if the file could not be opened.
         * The method will return an invalid ETFile if the path does not exist (for read modes), if the path is invalid, or if the file cannot be opened for any reason (e.g., insufficient permissions, media not available).
         */
        virtual ETFile open(const Path &path, const char *mode = FILE_MODE_READ, const bool create = false) const = 0;

        /**
         * @brief Checks if a file or directory exists at the specified path. The path can be absolute or relative to the current directory.
         * @param path The path to check for existence.
         * @return True if a file or directory exists at the specified path, false otherwise. The method will return false if the path is invalid or if the storage media is not available.
         */
        virtual bool exists(const Path &path) const = 0;

        /**
         * @brief Changes the current directory to the specified path. The path can be absolute or relative to the current directory.
         * @param path The path to change the current directory to.
         * @return True if the current directory was changed successfully, false otherwise. The method will return false if the path is invalid or if the storage media is not available.
         */
        virtual bool cd(const Path &path) = 0;

        /**
         * @brief Gets the current working directory path.
         * @return The current working directory path as a Path object. The method will return an empty Path if the storage media is not available.
         */
        virtual Path pwd() const = 0;
        /**
         * @brief Gets the absolute path for a given relative or absolute path. If the input path is relative, it will be resolved against the current working directory.
         * @param path The path to resolve, which can be absolute or relative.
         * @return The resolved absolute path as a Path object. The method will return an empty Path if the storage media is not available or if the path is invalid.
         */
        virtual Path pwd(const Path &path) const = 0;

        /**
         * @brief Lists the contents of the current directory or a specified directory. The path can be absolute or relative to the current directory. If a prefix is provided, only entries that start with the prefix will be included in the results.
         * @return A vector of Path objects representing the paths of the files and directories in the current or specified directory. The method will return an empty vector if the path is invalid, if the storage media is not available, or if the directory is empty.
         */
        virtual ETVector<Path> ls() const = 0;

        /**
         * @brief Lists the contents of a specified directory that start with a given prefix. The path can be absolute or relative to the current directory.
         * @param path The path of the directory to list, which can be absolute or relative to the current directory.
         * @param prefix The prefix to filter the directory entries. Only entries that start with this prefix will be included in the results.
         * @return A vector of Path objects representing the paths of the files and directories in the specified directory that start with the given prefix. The method will return an empty vector if the path is invalid, if the storage media is not available, if the directory is empty, or if no entries match the prefix.
         */
        virtual ETVector<Path> ls(const Path &path, const ETString &prefix = "") const = 0;

        /**
         * @brief Creates a new directory at the specified path. The path can be absolute or relative to the current directory.
         * @param path The path of the directory to create, which can be absolute or relative to the current directory.
         * @return True if the directory was created successfully, false otherwise. The method will return false if the path is invalid, if the storage media is not available, or if a file or directory already exists at the specified path.
         */
        virtual bool mkdir(const Path &path) = 0;

        /**
         * @brief Removes an existing directory at the specified path. The path can be absolute or relative to the current directory. The directory must be empty to be removed.
         * @param path The path of the directory to remove, which can be absolute or relative to the current directory.
         * @return True if the directory was removed successfully, false otherwise. The method will return false if the path is invalid, if the storage media is not available, if no directory exists at the specified path, if the path points to a file instead of a directory, or if the directory is not empty.
         */
        virtual bool rmdir(const Path &path) = 0;

        /**
         * @brief Checks if a directory at the specified path is empty. The path can be absolute or relative to the current directory.
         * @param path The path of the directory to check, which can be absolute or relative to the current directory.
         * @return True if the directory exists and is empty, false if the directory exists and is not empty. The method will return false if the path is invalid, if the storage media is not available, if no directory exists at the specified path, or if the path points to a file instead of a directory.
         */
        virtual bool isEmpty(const Path &path) = 0;

        /**
         * @brief Removes a file at the specified path. The path can be absolute or relative to the current directory.
         * @param path The path of the file to remove, which can be absolute or relative to the current directory.
         * @return True if the file was removed successfully, false otherwise. The method will return false if the path is invalid, if the storage media is not available, if no file exists at the specified path or if the path points to a directory instead of a file.
         */
        virtual bool remove(const Path &path) = 0;

        /**
         * @brief Checks if the specified path points to a directory. The path can be absolute or relative to the current directory.
         * @param path The path to check, which can be absolute or relative to the current directory.
         * @return True if the path exists and points to a directory, false if the path exists and points to a file. The method will return false if the path is invalid, if the storage media is not available, or if no file or directory exists at the specified path.
         */
        virtual bool isDirectory(const Path &path) const = 0;

        /**
         * @brief Gets the underlying storage system used by this directory navigator.
         * @return A pointer to the IStorageSystem used by this directory navigator, or nullptr if the storage system is not available.
         */
        virtual IStorageSystem *getStorageSystem() const = 0;
    };
}

#endif // IDIRECTORYNAVIGATOR_H