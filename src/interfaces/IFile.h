#ifndef I_FILE_H
#define I_FILE_H

#include "Path.h"
#include "ETTypes.h"

namespace EmbeddedTerminal
{

    const char *const FILE_MODE_READ = "r";
    const char *const FILE_MODE_WRITE = "w";
    const char *const FILE_MODE_APPEND = "a";

    /**
     * @brief Interface for file abstraction.
     */
    class IFile
    {
    public:
        virtual ~IFile() {}

        /**
         * @brief Reads a single byte from the file.
         * @return The byte read as an integer, or -1 if end of file is reached.
         */
        virtual int read() = 0; // Read a single byte, return byte or -1 on EOF

        /**
         * @brief Reads up to size bytes from the file into the provided buffer.
         * @param buffer Pointer to the buffer where the read bytes will be stored.
         * @param size Number of bytes to read.
         * @return The number of bytes actually read.
         */
        virtual size_t read(void *buffer, size_t size) = 0;

        /**
         * @brief Writes a single byte to the file.
         * @param b The byte to write.
         * @return The number of bytes written (1 if successful, 0 otherwise).
         */
        virtual size_t write(unsigned char b) = 0;

        /**
         * @brief Writes a buffer of data to the file.
         * @param buffer Pointer to the buffer containing the data to write.
         * @param size Number of bytes to write.
         * @return The number of bytes actually written.
         */
        virtual size_t write(const void *buffer, size_t size) = 0;

        /**
         * @brief Reads the entire content of the file as a string.
         * @return The content of the file as an ETString.
         */
        virtual ETString readString() = 0;

        /**
         * @brief Reads the entire content of the file as a string.
         * @return The content of the file as an ETString.
         */
        virtual ETString readAll() = 0;

        /**
         * @brief Writes the entire content of a string to the file.
         * @param content The string content to write to the file.
         * @return True if the write operation was successful, false otherwise.
         */
        virtual bool writeAll(const ETString &content) = 0;

        /**
         * @brief Seeks to a specific position in the file.
         * @param position The position to seek to, in bytes from the beginning of the file
         * @return True if the seek operation was successful, false otherwise.
         */
        virtual bool seek(size_t position) = 0;

        /**
         * @brief Gets the current position in the file.
         * @return The current position in bytes from the beginning of the file.
         */
        virtual size_t position() const = 0;

        /**
         * @brief Gets the total size of the file.
         * @return The size of the file in bytes.
         */
        virtual size_t size() const = 0;

        /**
         * @brief Closes the file.
         */
        virtual void close() = 0;

        /**
         * @brief Gets the name of the file (without path).
         * @return The name of the file as an ETString.
         * @todo COnsider if this is neccessary, why does the file need to know its name? Maybe this should be a method of the storage media instead, or maybe it can be removed entirely and the caller can just use the path to get the name if needed.
         */
        // virtual ETString name() const = 0;

        /**
         * @brief Gets the full path of the file.
         * @return The full path of the file as a Path object.
         * @todo Consider if this is neccessary, why does the file need to know its path? Maybe this should be a method of the storage media instead, or maybe it can be removed entirely and the caller can just keep track of the path when opening the file.
         */
        // virtual Path path() const = 0;

        /**
         * @brief Checks if the file is currently open.
         * @return True if the file is open, false otherwise.
         */
        virtual bool isOpen() const = 0;

        /**
         * @brief Checks if the file is a directory.
         * @return True if the file is a directory, false otherwise.
         */
        virtual bool isDirectory() const = 0;
    };
}
#endif // I_FILE_H