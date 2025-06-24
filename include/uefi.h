/*
// Copyright (c) 2021-2025 Hewlett Packard Enterprise Development, LP
// 
// Hewlett-Packard and the Hewlett-Packard logo are trademarks of
// Hewlett-Packard Development Company, L.P. in the U.S. and/or other countries.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

struct GUID
{
    uint32_t start;
    uint16_t mid;
    uint16_t mid2;
    char remaining[8];
};

namespace UEFI
{
const char fvSignature[4] = {'_', 'F', 'V', 'H'};

#define ROM_SIZE (0x4000000)    // 64MB

typedef struct
{
    uint32_t NumBlocks;
    uint32_t Length;
} EFI_FV_BLOCK_MAP_ENTRY;

struct firmwareVolumeHeader
{
    unsigned char dummy[16];
    struct GUID FileSystemGUID;
    uint64_t Length;
    uint32_t Signature;
    uint32_t Attributes;
    uint16_t HeaderLen;
    uint16_t Checksum;
    uint16_t ExtHeaderOffset;
    unsigned char Reserved;
    unsigned char Revision;
    EFI_FV_BLOCK_MAP_ENTRY    BlockMap[1];  // added in gen12
};

#define EFI_FV_FILETYPE_FREEFORM 2

struct uefiFileHeader
{
    struct GUID fileID;
    uint16_t integrity;
    char Type;
    char Attribute;
    unsigned char Size[3];
    char State;
};

extern const char* FFS2;

struct firmwareVolumeExtHeader
{
    struct GUID FVNameGUID;
    uint32_t ExtHeaderSize;
};
#define GUID_LEN 36

struct sectionHeader
{
    unsigned char Size[3];
    unsigned char Type;
};

#define ILO_FV "2957631F-48E5-76DF-32E7-E499A9DE452A"
#define HPE_UEFI_BUILD_MANIFEST_FV "7659EE7C-451A-31F8-36CE-4F883593F0B3"

class FVSection
{
  public:
  private:
};

// This class support non compressed Files, and do not take care at building up
// a hierarchy into the file system. LZMA algorithm needs to be supported to
// uncompress FFS and parse their content which shall be considered as a new
// UEFIFirmwareImage object

class FVFile
{
  public:
    FVFile(const char* bfr, long offset);
    long next();
    void* Content();
    uint32_t Size();
    std::string GUID();

  private:
    uint32_t _Size = 0;
    long _NextEntryOffset;
    std::string _GUID;
    std::vector<FVSection> _LocalSections;
    void* _Content = NULL;
};

class FileVolume
{
  public:
    FileVolume(long offset, const char* bfr);
    ~FileVolume();
    std::string GUID();
    uint64_t Length();
    uint64_t Offset();
    std::string getCurrentLocalFilesGUID();
    uint32_t Size();
    void* getContent(std::string GUID);
    uint32_t getSize(std::string GUID);

  private:
    std::string _GUID;
    uint64_t _Length;
    uint64_t _Offset;
    uint64_t _CurrentFile = 0;
    std::vector<FVFile> _LocalFiles;
};

class UEFIFirmwareImage
{
  public:
    UEFIFirmwareImage(std::string path);
    void dumpRomFV();
    void* getFileContent(std::string GUID);
    uint32_t getFileSize(std::string GUID);
    ~UEFIFirmwareImage();

  private:
    std::vector<FileVolume> UefiFV;
    char* _Rom;
};
} // namespace UEFI
