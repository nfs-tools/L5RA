#ifndef ASIANTOOLS_FILESTREAM_H
#define ASIANTOOLS_FILESTREAM_H

#include <fstream>

typedef unsigned char BYTE;

class FileStream
{
public:
    explicit FileStream(const std::string &filename);

    FileStream(const FileStream &) = delete;
    FileStream &operator=(const FileStream &)= delete;

    FileStream(const FileStream&&) = delete;
    FileStream &operator=(const FileStream &&)= delete;

    ~FileStream();

    char readChar();

    BYTE readSingle();

    std::string readString(std::string::size_type length);

    const uint8_t readUint8();

    const uint16_t readUint16();

    const uint32_t readUint32();

    const uint64_t readUint64();

    template<typename T>
    void read(const T &data);

    std::fpos<mbstate_t> getPosition();

    const int getBytesRead();

    FileStream() = delete;

private:
    std::ifstream m_stream;
    std::string m_filename;

    int m_bytesRead;
};


#endif //ASIANTOOLS_FILESTREAM_H
