#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <map>

// trim from start (in place)
static inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
    {
        return std::isspace(ch) == 0;
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
    {
        return std::isspace(ch) == 0;
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s)
{
    trim(s);
    return s;
}

extern void hexdump(FILE *stream, void const *data, size_t len)
{
    unsigned int i;
    unsigned int r, c;

    if (stream == nullptr)
        return;
    if (data == nullptr)
        return;

    for (r = 0, i = 0; r < (len / 16 + static_cast<unsigned int>(len % 16 != 0)); r++, i += 16)
    {
        fprintf(stream, "%04X:   ", i); /* location of first byte in line */

        for (c = i; c < i + 8; c++) /* left half of hex dump */
            if (c < len)
                fprintf(stream, "%02X ", ((unsigned char const *) data)[c]);
            else
                fprintf(stream, "   "); /* pad if short line */

        fprintf(stream, "  ");

        for (c = i + 8; c < i + 16; c++) /* right half of hex dump */
            if (c < len)
                fprintf(stream, "%02X ", ((unsigned char const *) data)[c]);
            else
                fprintf(stream, "   "); /* pad if short line */

        fprintf(stream, "   ");

        for (c = i; c < i + 16; c++) /* ASCII dump */
            if (c < len)
                if (((unsigned char const *) data)[c] >= 32 &&
                    ((unsigned char const *) data)[c] < 127)
                    fprintf(stream, "%c", ((char const *) data)[c]);
                else
                    fprintf(stream, "."); /* put this for non-printables */
            else
                fprintf(stream, " "); /* pad if short line */

        fprintf(stream, "\n");
    }

    fflush(stream);
}

/**
 * Info about a chunk in a STREAM-L5RA file
 */
struct L5RAChunkInfo
{
    unsigned long FilePos; // file offset, starting from 0
    unsigned long Size; // chunk size
    __unused unsigned long Checksum; // checksum of chunk content
    unsigned long ChunkID; // chunk ID; can be the same for multiple chunks??
    __unused unsigned long TempSize; // unknown
    __unused unsigned long PrePad; // unknown
    __unused unsigned long PostPad; // unknown
    std::string bChunkId;
};

void split(const std::string &s, const char *delim, std::vector<std::string> &v)
{
    // to avoid modifying original string
    // first duplicate the original string and return a char pointer then free the memory
    char *dup = strdup(s.c_str());
    char *token = strtok(dup, delim);
    while (token != nullptr)
    {
        v.emplace_back(token);
        // the call is treated as a subsequent calls to strtok:
        // the function continues from where it left in previous invocation
        token = strtok(nullptr, delim);
    }
    free(dup);
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        std::cout << "Usage: " << argv[0] << " <BUN path without extension>" << std::endl;
        return 1;
    }

    const std::string &bunArg = argv[1];

    const std::vector<unsigned char> magicTpkBytes = {
            0x00,
            0x01,
            0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    const std::vector<unsigned char> sceneryEntryBytes = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F
    };

    const std::map<uint32_t, std::pair<const std::string, const std::string>> typeMap = {
            {0xB3300000, {"Texture Pack",              "BCHUNK_SPEED_TEXTURE_PACK_LIST_CHUNKS"}},

            {0x80134000, {"Solids",                    "BCHUNK_SPEED_ESOLID_LIST_CHUNKS"}},
            {0x80034100, {"Scenery",                   "BCHUNK_SPEED_SCENERY_SECTION"}},

            {0x00037220, {"Animation Start",           "BCHUNK_SPEED_BBGANIM_BLOCKHEADER"}},
            {0x00037240, {"Animation Keyframes",       "BCHUNK_SPEED_BBGANIM_KEYFRAMES"}},
            {0x00037250, {"Animation Instance Node",   "BCHUNK_SPEED_BBGANIM_INSTANCE_NODE"}},
            {0x00037260, {"Animation Instance Tree",   "BCHUNK_SPEED_BBGANIM_INSTANCE_TREE"}},
            {0x00037270, {"Animation End",             "BCHUNK_SPEED_BBGANIM_ENDPACKHEADER"}},

            {0x80135000, {"Lighting",                  "BCHUNK_SPEED_ELIGHT_CHUNKS"}},
            {0x80036000, {"[Emitter?] Trigger Pack",   "BCHUNK_SPEED_EMTRIGGER_PACK"}},
            {0x00034027, {"Smokeable (smoke-emitter)", "BCHUNK_SPEED_SMOKEABLE_SPAWNER"}},
            {0x0003bc00, {"Emitter Library",           "BCHUNK_SPEED_EMITTER_LIBRARY"}},
            {0x0003b801, {"Collisions",                "BCHUNK_CARP_WCOLLISIONPACK"}},
            {0x0003b802, {"Island Data (?)",           "BCHUNK_CARP_WGRID_ISLAND_DATA"}}
    };

    std::ifstream mooFile(bunArg + ".moo");
    std::ifstream bunFile(bunArg + ".BUN", std::ios::binary);

    if (mooFile.fail())
    {
        std::cout << "Couldn't open .moo file: " << bunArg << ".moo" << std::endl;
        return 1;
    }

    if (bunFile.fail())
    {
        std::cout << "Couldn't open .BUN file: " << bunArg << ".BUN" << std::endl;
        return 1;
    }

    std::ios::fmtflags f(std::cout.flags());

    int lineIndex = 0;
    int chunkIndex = 0;
    std::vector<L5RAChunkInfo> chunks;

    for (std::string line; std::getline(mooFile, line); lineIndex++)
    {
        if (lineIndex >= 12)
        {
            std::cout << "Chunk #" << std::dec << (lineIndex - 12) + 1 << std::endl;

            std::vector<std::string> parts;
            split(line, " ", parts);

            L5RAChunkInfo chunk{};

            for (int i = 1; i <= 11; i++)
            {
                if (i == 8 || i == 10) continue;

                std::cout << "    ";
                switch (i)
                {
                    case 1:
                        std::cout << "FilePos: ";
                        chunk.FilePos = std::stoul(parts[i], nullptr, 16);
                        break;
                    case 2:
                        std::cout << "Size: ";
                        chunk.Size = std::stoul(parts[i], nullptr, 16);
                        break;
                    case 3:
                        std::cout << "Checksum: ";
                        chunk.Checksum = std::stoul(parts[i], nullptr, 16);
                        break;
                    case 4:
                        std::cout << "ChunkID: ";
                        chunk.ChunkID = std::stoul(parts[i], nullptr, 16);
                        break;
                    case 5:
                        std::cout << "TempSize: ";
                        chunk.TempSize = std::stoul(parts[i], nullptr, 16);
                        break;
                    case 6:
                        std::cout << "PrePad: ";
                        chunk.PrePad = std::stoul(parts[i], nullptr, 16);
                        break;
                    case 7:
                        std::cout << "PostPad: ";
                        chunk.PostPad = std::stoul(parts[i], nullptr, 16);
                        break;
                    case 9:
                        std::cout << "Section: ";
                        break;
                    case 11:
                        std::cout << "BChunk ID: ";
                        chunk.bChunkId = parts[i];
                        break;
                    default:
                        break;
                }

                std::cout << parts[i] << std::endl;
            }

            chunkIndex++;

            chunks.emplace_back(chunk);
        }
    }

    std::cout << "" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "" << std::endl;

    std::map<std::string, std::vector<std::string>> textureNameMap;
    std::map<std::string, std::vector<std::string>> sceneryNameMap;

    for (int i = 0; i < chunks.size(); i++)
    {
        const L5RAChunkInfo &chunk = chunks[i];
        std::cout << "Chunk #" << i + 1 << std::endl;
        std::cout << "  ID = " << std::dec << chunk.ChunkID << std::endl;
        std::cout << "  File Offset = " << std::dec << chunk.FilePos << std::endl;

        auto fpos = static_cast<std::fpos<mbstate_t>>(chunk.FilePos);

        std::cout << "[!] Seeking bundleFile to offset: " << fpos << std::endl;
        bunFile.seekg(fpos);
        std::cout << "[!] Seek complete" << std::endl;

        std::vector<unsigned char> vec;
        vec.resize(chunk.Size);

        bunFile.read((char *) &vec[0], chunk.Size);

        std::cout << "[I] Vector size: " << vec.size() << std::endl;

        std::cout << "[!] Resetting stream" << std::endl;
        bunFile.clear();
        bunFile.seekg(fpos);

        int32_t chunkType;
        int32_t chunkSize;

        bunFile.read((char *) &chunkType, sizeof(chunkType));
        bunFile.read((char *) &chunkSize, sizeof(chunkSize));

        std::cout << "[I] Type: " << std::setw(8) << std::setfill('0') << std::hex << chunkType << std::dec
                  << std::endl;
        std::cout << "[I] Size: " << std::setw(8) << std::setfill('0') << std::hex << chunkSize << std::dec
                  << std::endl;
        std::cout << "[I] Resolved type: ";

        std::map<uint32_t, std::pair<const std::string, const std::string>>::const_iterator foundType;

        if ((foundType = typeMap.find(chunkType)) != typeMap.end())
        {
            std::cout << foundType->second.first;
        } else
        {
            std::cout << "UNKNOWN";
        }

        std::cout << std::endl;

        if (chunkType == 0xB3300000) // BCHUNK_SPEED_TEXTURE_PACK_LIST_CHUNKS
        {
            unsigned char header[0x4c]; // 76 bytes
            unsigned char name[0x5c]; // 92 bytes; end-of-string padded by NULs

            bunFile.read((char *) &header[0], sizeof(header));
            bunFile.read((char *) &name[0], sizeof(name));

            std::string nameStr(name, name + sizeof(name));
            std::vector<std::string> textureNames{};

            {
                auto firstNull = nameStr.find_first_of((char) '\0');
                auto lastAscii = nameStr.find_last_of(
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.");
                nameStr = nameStr.substr(0, lastAscii + 1);
                nameStr = nameStr.substr(firstNull + 1);
            }

            std::cout << "Name: " << nameStr << std::endl;

            for (;;)
            {
                if (bunFile.get() != '\0')
                {
                    bunFile.seekg(bunFile.tellg() - (std::streamoff) 1);
                    break;
                }
            }

            uint32_t unknownValue;
            bunFile.read((char *) &unknownValue, sizeof(unknownValue));

            unsigned char zeroes[24];
            bunFile.read((char *) &zeroes[0], sizeof(zeroes));

            int32_t hash = 0;
            int hashIndex = 0;

            while (true)
            {
                bunFile.read((char *) &hash, sizeof(hash));

                hashIndex++;

                if (hash == 0xFFFFFFFF)
                {
                    break;
                }
            }

            {
                auto it = std::search(vec.begin(), vec.end(), magicTpkBytes.begin(), magicTpkBytes.end());

                if (it != vec.end())
                {
                    auto offset = it - vec.begin();

                    std::cout << "Found magic bytes at offset " << offset << std::endl;

                    auto newPos = chunk.FilePos + offset;

                    bunFile.seekg(newPos);

                    for (int k = 0; k < 4; k++)
                    {
                        uint32_t placeholder;
                        bunFile.read((char *) &placeholder, sizeof(placeholder));
                    }
                }
            }

            std::string vecStr(vec.begin(),
                               vec.end()), sub = "DXT"; // str is string to search, sub is the substring to search for

            std::vector<size_t> positions; // holds all the positions that sub occurs within str

            size_t pos = vecStr.find(sub, 0);
            while (pos != std::string::npos)
            {
                positions.push_back(pos);
                pos = vecStr.find(sub, pos + 1);
            }

            for (int t = 0; t < positions.size(); t++)
            {
                char ukc;
                bunFile.get(ukc);

                std::stringstream textureNameStream;

                while (true)
                {
                    char nc;
                    bunFile.get(nc);

                    if (nc < 32 || nc > 127)
                    {
                        break;
                    }

                    textureNameStream << nc;
                }

                std::string textureName = textureNameStream.str();

                textureNames.push_back(textureName);

                unsigned char pad[88];
                bunFile.read((char *) &pad[0], sizeof(pad));

                for (char c; bunFile.get(c);)
                {
                    if (c != 0)
                    {
                        bunFile.seekg(bunFile.tellg() - (std::streamoff) 1);
                        break;
                    }
                }
            }

            std::sort(textureNames.begin(), textureNames.end());
            textureNameMap.insert({nameStr, textureNames});
        } else if (chunkType == 0x80134000) // BCHUNK_SPEED_ESOLID_LIST_CHUNKS
        {
            unsigned char header[40];
            bunFile.read((char *) &header[0], sizeof(header));

            while (true)
            {
                char pc;
                bunFile.get(pc);

                if (pc == 'e')
                {
                    bunFile.get(pc);

                    if (pc == 'L')
                    {
                        bunFile.get(pc);

                        if (pc == 'a')
                        {
                            bunFile.get(pc);

                            if (pc == 'b')
                            {
                                // eLab is the prefix
                                bunFile.seekg(bunFile.tellg() - (std::streamoff) 4);
                                break;
                            }
                        }
                    }
                }
            }

            char nameBuf[56]; // max 56 characters, padded by NUL
            char sectionBuf[8]; // 8 chars just to be safe; NULs will balance

            bunFile.read(nameBuf, sizeof(nameBuf));
            bunFile.read(sectionBuf, sizeof(sectionBuf));

            std::string nameStr(nameBuf), sectionStr(sectionBuf);

            std::cout << nameStr << " in " << sectionStr << std::endl;
        } else if (chunkType == 0x80034100) // BCHUNK_SPEED_SCENERY_SECTION
        {
            unsigned char header[0x35 - 8];

            bunFile.read((char *) &header[0], sizeof(header));

            std::vector<std::string> sceneryNames{};
            std::string vecStr(vec.begin(), vec.end());
            std::string sub(sceneryEntryBytes.begin(), sceneryEntryBytes.end());

            std::vector<size_t> positions;
            size_t pos = vecStr.find(sub, 0);
            while (pos != std::string::npos)
            {
                positions.push_back(pos);
                pos = vecStr.find(sub, pos + 1);
            }

            std::map<int/*posIndex*/, size_t/*pos*/> posMap;

            for (int j = 0; j < positions.size(); j++)
            {
                auto it = std::find_if(posMap.begin(), posMap.end(), [&](std::pair<int, size_t> k)
                {
                    return k.second == positions[j];
                });

                if (it != posMap.end())
                {
                    continue;
                }

                posMap.insert(std::make_pair(j, positions[j]));
            }

            int count = 0;
            const std::vector<unsigned char> endingBytes = {
                    0x02, 0x41, 0x03, 0x00
            };

            size_t totalSize = 0;

            for (auto &pair : posMap)
            {
                int index = pair.first;
                size_t position = pair.second;

                size_t newPos = chunk.FilePos + position;

                bunFile.seekg(newPos);

                if (index != posMap.size() - 1)
                {
                    auto nextPair = posMap.find(index + 1);

                    size_t size = (chunk.FilePos + nextPair->second) - newPos;
                    totalSize += size;

                    if (size == 16)
                    {
                        continue;
                    }

                    unsigned char thunk[size];
                    bunFile.read((char *) &thunk[0], sizeof(thunk));
                } else
                {
                    std::vector<unsigned char> thunk;
                    thunk.resize(chunk.Size - totalSize);
                    bunFile.read((char *) &thunk[0], thunk.size());

                    auto it = std::search(thunk.begin(), thunk.end(), endingBytes.begin(), endingBytes.end());

                    if (it != thunk.end())
                    {
                        bunFile.seekg(newPos);

                        long offset = it - thunk.begin();

                        for (long j = 0; j < offset + 8; j++)
                        {
                            bunFile.get();
                        }

                        for (;;)
                        {
                            char nb[24]; // null-padded
                            unsigned char uk[48]; // other data

                            bunFile.read(nb, sizeof(nb));
                            bunFile.read((char *) &uk[0], sizeof(uk));

                            if (nb[0] < 32 || nb[0] > 127)
                                break;

                            sceneryNames.emplace_back(std::string(nb));
                            count++;
                        }
                    }
                }
            }

            std::sort(sceneryNames.begin(), sceneryNames.end());
            std::string sceneryStr = "Chunk" + std::to_string(i + 1) + "__Scenery";
            sceneryNameMap.insert(std::make_pair(sceneryStr, sceneryNames));
        } else if (chunkType == 0x00034027) // BCHUNK_SPEED_SMOKEABLE_SPAWNER
        {
            //
        } else if (chunkType == 0x00037220) // BCHUNK_SPEED_BBGANIM_BLOCKHEADER
        {
            //
        } else if (chunkType == 0x00037260) // BCHUNK_SPEED_BBGANIM_INSTANCE_TREE
        {
            //
        } else if (chunkType == 0x00037250) // BCHUNK_SPEED_BBGANIM_INSTANCE_NODE
        {
            //
        } else if (chunkType == 0x00037270) // BCHUNK_SPEED_BBGANIM_ENDPACKHEADER
        {
            //
        } else if (chunkType == 0x80135000) // BCHUNK_SPEED_ELIGHT_CHUNKS
        {
            //
        } else if (chunkType == 0x80036000) // BCHUNK_SPEED_EMTRIGGER_PACK
        {
            //
        } else if (chunkType == 0x0003b802) // BCHUNK_CARP_WGRID_ISLAND_DATA
        {

        }

        std::string hexdumpFileName =
                bunArg + "-Chunk" + std::to_string(i + 1) + "-" + trim_copy(chunk.bChunkId) + ".chunk";
        std::string rawFileName =
                bunArg + "-Chunk" + std::to_string(i + 1) + "-" + trim_copy(chunk.bChunkId) + ".rawchunk";
        FILE *hexDump = fopen(hexdumpFileName.c_str(), "wb");
        FILE *rawDump = fopen(rawFileName.c_str(), "wb");

        hexdump(hexDump, vec.data(), vec.size());
        fwrite(vec.data(), vec.size(), 1, rawDump);
    }

    std::cout.flags(f);

    for (auto &pair : textureNameMap)
    {
        std::cout << "Texture Names for [" << pair.first << "] (" << pair.second.size() << ")" << std::endl;

        for (int i = 0; i < pair.second.size(); i++)
        {
            std::cout << "#" << i + 1 << ": " << pair.second[i] << std::endl;
        }

        std::cout << std::endl;
    }

    for (auto &pair : sceneryNameMap)
    {
        std::cout << "Scenery Names for [" << pair.first << "] (" << pair.second.size() << ")" << std::endl;

        for (int i = 0; i < pair.second.size(); i++)
        {
            std::cout << "#" << i + 1 << ": " << pair.second[i] << std::endl;
        }

        std::cout << std::endl;
    }

    return 0;
}