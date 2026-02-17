#ifndef I_FILE_H
#define I_FILE_H

#include "ETTypes.h"

namespace EmbeddedTerminal
{

    const char *const FILE_MODE_READ = "r";
    const char *const FILE_MODE_WRITE = "w";
    const char *const FILE_MODE_APPEND = "a";
    class IFile
    {
    public:
        virtual ~IFile() {}

        virtual int read() = 0; // Read a single byte, return byte or -1 on EOF
        // Read up to size bytes into buffer, return number of bytes read
        virtual size_t read(void *buffer, size_t size) = 0;

        // Write a single byte, return number of bytes written (1 or 0)
        virtual size_t write(unsigned char) = 0;
        // Write up to size bytes from buffer, return number of bytes written
        virtual size_t write(const void *buffer, size_t size) = 0;

        // Same as readAll()
        virtual ETString readString() = 0;
        // Read all content as string
        virtual ETString readAll() = 0;

        // Write string content
        virtual bool writeAll(const ETString &content) = 0;

        // Seek to position
        virtual bool seek(size_t position) = 0;

        // Get current position
        virtual size_t position() const = 0;

        // Get file size
        virtual size_t size() const = 0;

        // Close file
        virtual void close() = 0;

        virtual ETString name() const = 0;
        virtual ETString path() const = 0;

        virtual bool isOpen() const = 0;
        virtual bool isDirectory() const = 0;
    };

}
#endif // I_FILE_H