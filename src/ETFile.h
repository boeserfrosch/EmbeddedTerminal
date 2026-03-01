#ifndef ET_FILE_H
#define ET_FILE_H

#include "ETTypes.h"
#include "interfaces/IFile.h"
#include <memory>

namespace EmbeddedTerminal
{

    class ETFile
    {
    public:
        ETFile(std::shared_ptr<IFile> filePtr) : file_(filePtr) {}
        ETFile() : file_(nullptr) {}

        // Forward all IFile methods
        size_t read(void *buffer, size_t size)
        {
            return file_ ? file_->read(buffer, size) : 0;
        }
        int read()
        {
            return file_ ? file_->read() : -1;
        }

        // Write a single byte, return number of bytes written (1 or 0)
        size_t write(unsigned char c)
        {
            return file_ ? file_->write(c) : 0;
        }

        size_t write(const void *buffer, size_t size)
        {
            return file_ ? file_->write(buffer, size) : 0;
        }
        ETString readAll()
        {
            return file_ ? file_->readAll() : ETString();
        }
        ETString readString()
        {
            return file_ ? file_->readString() : ETString();
        }
        bool writeAll(const ETString &content)
        {
            return file_ ? file_->writeAll(content) : false;
        }
        bool seek(size_t position)
        {
            return file_ ? file_->seek(position) : false;
        }
        size_t position() const
        {
            return file_ ? file_->position() : 0;
        }
        size_t size() const
        {
            return file_ ? file_->size() : 0;
        }
        void close()
        {
            if (file_)
                file_->close();
        }
        bool isOpen() const
        {
            return file_ ? file_->isOpen() : false;
        }
        std::shared_ptr<IFile> getRaw() { return file_; }

        bool operator!()
        {
            return !file_ || !file_->isOpen();
        }

        bool isDirectory() const
        {
            return file_ ? file_->isDirectory() : false;
        }

    private:
        std::shared_ptr<IFile> file_;
    };

} // namespace EmbeddedTerminal

#endif // ET_FILE_H
