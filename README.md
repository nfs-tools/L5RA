# STREAML5RA Thing
~~A somewhat poorly-coded tool that's capable of extracting a few types of data from certain files~~

### Note: This has not been tested on every file. This currently does not run on Windows.
**This tool currently only supports STREAML5RA split files with corresponding `.moo` files.**
### Another note: This is more of a public proof-of-concept of how to read some types of data from NFSW, rather than a tool for users. 

---

Reads STREAML5RA_* files (**specifically, their chunks**) from **Need for Speed World**'s Asian Beta (**AT1**). I will probably update this to support the final version, and maybe even other games?

## How do I use it?

2 options:
- Don't.
- Follow the steps below.

---
**Usage Steps:**
1. Somehow manage to compile this thing; I use CLion as my IDE. If you can't compile this, well, you'll have to wait.
2. Get the NFSW Asian Beta (AT1) from 2009; you can find it if you browse around a bit.
3. Get the files from the installer by using 7zip
4. Go to the TRACKS folder in AT1's data directory
5. Make sure you see a bunch of .BUN files and a bunch of .moo files
    
    5a. If you don't, abort mission and keep Googling.
    
    5b. If you do, great. Continue to step 6.
6. Are you sure you compiled the program?
7. Find the program executable.
8. Run `/path/to/the/program/executable /path/to/a/STREAML5RA-without-the-extension`
    
      **Example: `./cmake-build-debug/L5RA /Users/heyitsleo/Desktop/AT1/TRACKS/STREAML5RA_18`**
9. Cross your fingers and hold your breath with anticipation
10. Look at your fresh list of... well... well... text. That's assuming you happened to select a L5RA with any text-containing chunks.
## How does it work?
In AT1, every STREAML5RA_\*.BUN file has a corresponding STREAML5RA_\*.moo file.

For example: `STREAML5RA_24000.BUN` -\> `STREAML5RA_24000.moo`

Each `.moo` file is in a relatively simple text-based format. Here's an excerpt of one.
```
//
// TrackPrePacker.exe generated incremental info file.
//
// Info file - ..\PC\CDShift\TRACKS\STREAML5RA_23310.moo
// ChunkFile - ..\PC\CDShift\TRACKS\STREAML5RA_23310.BUN

MAGIC: 1234124  // Change this # in TrackStreamPacker.cpp to invalidate these scripts.
OUTPUTFILE: ..\PC\CDShift\TRACKS\STREAML5RA_23310.BUN
NUMCHUNKS: 13

//      FilePos     Size        Checksum    ChunkID     TempSize    PrePad      PostPad
// =================================================================================================
CHUNK:  0x00000000  0x000001f8  0xb069da25  0x80134000  0x00000000  0x00000000  0x00000000 // X310 [UPDATED] BCHUNK_SPEED_ESOLID_LIST_CHUNKS
CHUNK:  0x00000224  0x000671f8  0x7a64adb0  0x80134000  0x00000000  0x00000024  0x00000000 // X310 [UPDATED] BCHUNK_SPEED_ESOLID_LIST_CHUNKS
CHUNK:  0x00067478  0x0002e2f0  0x8c32c7b4  0x80134000  0x00000000  0x00000054  0x00000000 // X310 [UPDATED] BCHUNK_SPEED_ESOLID_LIST_CHUNKS
CHUNK:  0x000957ec  0x0000dccc  0x0d0db6fd  0x80134000  0x00000000  0x0000007c  0x00000000 // X310 [UPDATED] BCHUNK_SPEED_ESOLID_LIST_CHUNKS
CHUNK:  0x000a34c0  0x0003adf4  0xd0a76751  0x80134000  0x00000000  0x00000000  0x00000000 // X310 [UPDATED] BCHUNK_SPEED_ESOLID_LIST_CHUNKS
...
```

Seems a bit cryptic, right? Not at all; this format gives quite a bit of information. (Simply a nice-to-have. Parsing can be done without one of these.)

The tool parses this format to build chunk information structures, which are then used to, well, read the chunks and extract information from them.
Each chunk type has a different format and layout, which makes the process of extracting information a bit tedious at times.
That's why this tool currently only extracts the names of things.

## Current Features

- Extracts texture names from StreamingTexturePack chunks.
Example:
```
Texture Names for [StreamingTexturePackI119.mpk] (14)
#1: ARC_FRATFENCE2
#2: ARC_HOSPITALOLD_WALL_02_GH
#3: ARC_HOSPITALOLD_WALL_02_GH_NORMAL
#4: ARC_HOSPITALOLD_WALL_02_GH_SPEC
#5: ARC_HOSPITALOLD_WINDOW
#6: OBJ_BOARDEMUP
#7: OBJ_GASFILLCOVER
#8: OBJ_GASFILLCOVER_NORMAL
#9: OBJ_GASFILLCOVER_SPEC
#10: OBJ_GRAPHITI_01
#11: OBJ_TUNNELFAN
#12: SGN_SAFEHOUSEA
#13: SGN_SAFEHOUSEG
#14: TRN_OILSTAIN_A_BP
```

- Extracts names of scenery objects from scenery chunks.
Example:
```
Scenery Names for [Chunk13__Scenery] (77)
#1: GasS_GasFillCover01
#2: OBJ_OilStain01_DEINST1_
#3: SafeHouse_BoardedWindow
#4: SafeHouse_BrokenWindow0
#5: SafeHouse_Graffiti05_DE
#6: TRN_CT_Road_Colonial_01
#7: TRN_CT_Road_Colonial_01
#8: TRN_CT_Road_Colonial_01
#9: TRN_CT_Road_Colonial_01
#10: XB_CT_BBD01_2b_00
#11: XB_CT_BBD02_1b_00
#12: XB_CT_BBD03_1b_00
#13: XB_CT_MDBD_1b_00
#14: XB_CT_RadioStation_1b_0
#15: XB_CT_SafehouseA_1b_00
...
```

- Extracts the name and world-section that a solid is located in. Example:
```
eLabScenery_Chop_L2RA_CT_Terrain.bin in I119
eLabScenery_L2RA_Library_Objects.bin in I119
eLabScenery_L2RA_Library_Building_Commercial.bin in I119
```

## Chunk Types
~~_Oh dear, where do I start..._~~

Here's all the known types!
```cpp
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
```

You're probably feeling one of 3 ways right now:

- Wait, what the heck is that?
- Uhhhhhhhhhh
- Okay that's cool

Allow me to aid you in deciphering this.
Take this for example: 
```
{0xB3300000, {"Texture Pack", "BCHUNK_SPEED_TEXTURE_PACK_LIST_CHUNKS"}}
```

`0xB3300000` is the **chunk type**. 
In a chunk, it would be the bytes `00 00 30 B3`, not `B3 30 00 00` as you might expect. 
Be careful!

`"Texture Pack"` is just a human-readable identifier I put in.

`"BCHUNK_SPEED_TEXTURE_PACK_LIST_CHUNKS"` is the "BChunk ID". They're basically somewhat-human-readable strings that describe a chunk's type.
These are only present in the .moo files.

**One last note: This is a very early version.** I will be improving this code and extending it in the future.
If you do end up running this, please let me know if you find any "UNKNOWN" chunks.