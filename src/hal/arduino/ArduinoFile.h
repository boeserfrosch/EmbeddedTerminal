#pragma once
#ifdef ARDUINO
#include "interfaces/IFile.h"
#include <memory>
#include <FS.h>

namespace EmbeddedTerminal
{

    class ArduinoFile : public IFile
    {
    public:
        ArduinoFile() = delete;
        ArduinoFile(File f) : file_(f) {}

        ~ArduinoFile() override
        {
            file_.close();
        }

        int read() override
        {
            if (!file_.available())
                return -1;
            return file_.read();
        }
        size_t read(void *buffer, size_t size) override
        {
            return file_.read((uint8_t *)buffer, size);
        }

        size_t write(unsigned char b) override
        {
            return file_.write(b);
        }
        size_t write(const void *buffer, size_t size) override
        {
            return file_.write((const uint8_t *)buffer, size);
        }

        ETString readString() override { return readAll(); }
        ETString readAll() override
        {
            ETString out;
            file_.seek(0);
            while (file_.available())
            {
                char c = (char)file_.read();
                out += c;
            }
            return out;
        }

        bool writeAll(const ETString &content) override
        {
            file_.seek(0);
            auto written = file_.print(content.c_str());
            return written > 0;
        }

        bool seek(size_t position) override
        {
            return file_.seek(position);
        }

        size_t position() const override
        {
            return file_.position();
        }

        size_t size() const override
        {
            return file_.size();
        }

        void close() override
        {
            file_.close();
        }

        bool isDirectory() const override
        {
            auto f = file_;
            if (f.isDirectory())
                return true;
            return false;
        }

        bool isOpen() const override
        {
            auto f = file_;
            return f.seek(f.position());
        }

    private:
        File file_;
    };

} // namespace EmbeddedTerminal
#endif // ARDUINO
