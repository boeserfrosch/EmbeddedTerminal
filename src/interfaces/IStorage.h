#pragma once

#include "IFileSystem.h"
#include "IFile.h"
#include "ETTypes.h"

namespace EmbeddedTerminal
{

    /**
     * @brief Interface for storage media (e.g., SD card, internal flash)
     */
    class IStorageMedia
    {
    public:
        virtual ~IStorageMedia() = default;

        /**
         * @brief Gets the name of the storage media (e.g., "SD Card", "Internal Flash").
         * @return The name of the storage media as a C-string.
         */
        virtual const char *name() const = 0;

        /**
         * @brief Gets the file system associated with this storage media.
         * @return A pointer to an IFileSystem object representing the file system of this storage media.
         */
        virtual IFileSystem *fileSystem() = 0;

        /**
         * @brief Checks if the storage media is currently available (e.g., SD card is inserted).
         * @return True if the storage media is available, false otherwise.
         */
        virtual bool isAvailable() const = 0;

        /**
         * @brief Gets the total capacity of the storage media in bytes.
         * @return The total capacity of the storage media in bytes.
         */
        virtual unsigned long long totalBytes() const = 0;

        /**
         * @brief Gets the number of bytes currently used on the storage media.
         * @return The number of bytes currently used on the storage media.
         */
        virtual unsigned long long usedBytes() const = 0;

        /**
         * @brief Gets the total capacity of the storage media in bytes.
         * @return The total capacity of the storage media in bytes.
         */
        virtual unsigned long long capacity() const = 0;

        /**
         * @brief Gets the amount of free space available on the storage media in bytes.
         * @return The amount of free space available on the storage media in bytes.
         */
        virtual unsigned long long freeBytes() const = 0;
    };

    /**
     * @brief Interface for a unified storage system that can manage multiple storage media.
     */
    class IStorageSystem : public IFileSystem
    {
    public:
        virtual ~IStorageSystem() = default;

        /**
         * @brief Gets a list of all available storage media.
         * @return A vector of pointers to IStorageMedia objects representing the available storage media.
         */
        virtual ETVector<std::shared_ptr<IStorageMedia>> media() const = 0;

        /**
         * @brief Gets a specific storage media by name.
         * @param name The name of the storage media to retrieve.
         */
        virtual std::shared_ptr<IStorageMedia> getMedia(const ETString &name) const = 0;

        /**
         * @brief Gets the storage media associated with a specific path in the file system.
         * @param path The path to look up.
         * @return A pointer to the IStorageMedia associated with the given path, or nullptr if no media is associated with that path.
         */
        virtual std::shared_ptr<IStorageMedia> getMediaFromPath(const Path &path) const = 0;

        /**
         * @brief Mounts a storage media to a specified mount point in the file system.
         * @param media The storage media to mount.
         * @param mountPoint The mount point in the file system where the media should be mounted.
         * @return True if the media was successfully mounted, false otherwise.
         */
        virtual bool mountMedia(std::shared_ptr<IStorageMedia> media, const Path &mountPoint) = 0;

        /**
         * @brief Unmounts a storage media from the file system.
         * @param name The name of the storage media to unmount.
         * @return True if the media was successfully unmounted, false otherwise.
         */
        virtual bool unmountMedia(const ETString &name) = 0;

        /**
         * @brief Unmounts a storage media from the file system using its mount point.
         * @param mountPoint The mount point of the storage media to unmount.
         * @return True if the media was successfully unmounted, false otherwise.
         */
        virtual bool unmountMediaFromMountpoint(const Path &mountPoint) = 0;

        /**
         * @brief Copies a file from one path to another within the storage system. There is no check if dst already exists, so it may overwrite existing files.
         * @param srcPath The source path of the file to copy.
         * @param dstPath The destination path where the file should be copied.
         * @return True if the file was successfully copied, false otherwise. It will return false if the source file does not exist, the destination path is invalid, or if the copy operation fails for any reason (e.g., insufficient space, media not available).
         */
        virtual bool copyFile(const Path &srcPath, const Path &dstPath) = 0;

        /**
         * @brief Moves a file from one path to another within the storage system.
         * @param srcPath The source path of the file to move.
         * @param dstPath The destination path where the file should be moved.
         * @return True if the file was successfully moved, false otherwise.
         */
        virtual bool moveFile(const Path &srcPath, const Path &dstPath)
        {
            if (!copyFile(srcPath, dstPath))
                return false;
            return removeFile(srcPath);
        };

        /**
         * @brief Removes a file at the specified path within the storage system.
         * @param path The path of the file to remove.
         * @return True if the file was successfully removed, false otherwise.
         */
        virtual bool removeFile(const Path &path) = 0;
    };

} // namespace EmbeddedTerminal
