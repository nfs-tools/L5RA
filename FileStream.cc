#include "FileStream.h"

FileStream::FileStream(const std::string &filename)
{

}

FileStream::~FileStream() = default;

BYTE FileStream::readSingle()
{
    BYTE c;
    m_stream.read((char *) &c, 1);
    m_bytesRead++;
    return c;
}

char FileStream::readChar()
{
    char c;
    m_stream.get(c);
    m_bytesRead++;
    return c;
}

std::string FileStream::readString(const std::string::size_type length)
{
    char buf[length];

    m_stream.read(buf, sizeof(buf));
    m_bytesRead += length;

    std::string str(buf, sizeof(buf));

    return str;
}

const uint8_t FileStream::readUint8()
{
    uint8_t uint8;
    m_stream.read((char *) &uint8, sizeof(uint8));
    m_bytesRead++;

    return uint8;
}

const uint16_t FileStream::readUint16()
{
    uint16_t uint16;
    m_stream.read((char *) &uint16, sizeof(uint16));
    m_bytesRead += 2;

    return uint16;
}

const uint32_t FileStream::readUint32()
{
    uint32_t uint32;
    m_stream.read((char *) &uint32, sizeof(uint32));
    m_bytesRead += 4;

    return uint32;
}

const uint64_t FileStream::readUint64()
{
    uint64_t uint64;
    m_stream.read((char *) &uint64, sizeof(uint64));
    m_bytesRead += 8;

    return uint64;
}

template<typename T>
void FileStream::read(const T &data)
{
    m_stream.read((char *) &data[0], sizeof(data));
    m_bytesRead += sizeof(data);
}

std::fpos<mbstate_t> FileStream::getPosition()
{
    return m_stream.tellg();
}

const int FileStream::getBytesRead()
{
    return m_bytesRead;
}

