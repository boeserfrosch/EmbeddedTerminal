#pragma once
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include "interfaces/IFile.h"
#include <memory>
#include <cstdio>
#include <sys/stat.h>
#include <cstring>

namespace EmbeddedTerminal
{
    class ESPIDFFile : public IFile
    {
    public:
        ESPIDFFile() = delete;
        ESPIDFFile(FILE *f) : file_(f)
        {
        }

        ~ESPIDFFile() override
        {
            if (file_)
                fclose(file_);
        }

        int read() override
        {
            if (!file_)
                return -1;
            int c = fgetc(file_);
            return c;
        }
        size_t read(void *buffer, size_t size) override
        {
            if (!file_)
                return 0;
            return fread(buffer, 1, size, file_);
        }

        size_t write(unsigned char b) override
        {
            if (!file_)
                return 0;
            return fwrite(&b, 1, 1, file_);
        }
        size_t write(const void *buffer, size_t size) override
        {
            if (!file_)
                return 0;
            return fwrite(buffer, 1, size, file_);
        }

        ETString readString() override { return readAll(); }
        ETString readAll() override
        {
            if (!file_)
                return "";
            fseek(file_, 0, SEEK_SET);
            ETString result;
            char buf[128];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf) - 1, file_)) > 0)
            {
                buf[n] = '\0';
                result += buf;
            }
            return result;

            // if (!file_)
            //     return ETString();
            // fseek(file_, 0, SEEK_SET);
            // ETString out;
            // size_t bufferSize = 128;
            // char buf[bufferSize];
            // size_t n;
            // while ((n = fread(buf, 1, bufferSize - 1, file_)) > 0)
            // {
            //     if (n < bufferSize - 1)
            //     {
            //         buf[n] = '\0';
            //     }
            //     out += ETString(buf);
            // }
            // return out;
        }

        bool writeAll(const ETString &content) override
        {
            if (!file_)
                return false;
            fseek(file_, 0, SEEK_SET);
            size_t written = fwrite(content.c_str(), 1, content.length(), file_);
            fflush(file_);
            return written == content.length();
        }

        bool seek(size_t position) override
        {
            if (!file_)
                return false;
            return fseek(file_, position, SEEK_SET) == 0;
        }

        size_t position() const override
        {
            if (!file_)
                return 0;
            return ftell(file_);
        }

        size_t size() const override
        {
            if (!file_)
                return 0;
            long cur = ftell(file_);
            fseek(file_, 0, SEEK_END);
            long sz = ftell(file_);
            fseek(file_, cur, SEEK_SET);
            return sz;
        }

        void close() override
        {
            if (file_)
            {
                fclose(file_);
                file_ = nullptr;
            }
        }

        ETString name() const override
        {
            // Not portable: just return empty string
            return ETString();
        }

        ETString path() const override { return ETString(); }

        bool isDirectory() const override
        {
            // Not supported for FILE*
            return false;
        }

        bool isOpen() const override
        {
            return file_ != nullptr;
        }

    private:
        FILE *file_;
    };
}
#endif // ESP_PLATFORM || ESP_32