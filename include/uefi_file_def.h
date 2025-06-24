/*
// Copyright (c) 2016, 2021-2025 Hewlett Packard Enterprise Development, LP
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

#ifndef __UEFI_FILE_DEF_H__
#define __UEFI_FILE_DEF_H__

#define PLATDEF_DATA_FILE        "/home/root/platdef.dat"

typedef UINT32  EFI_FVB_ATTRIBUTES_2;

typedef struct
{
    UINT32 NumBlocks;
    UINT32 Length;
} EFI_FV_BLOCK_MAP_ENTRY;

//
// Describes the features and layout of the firmware volume.
//
typedef struct
{
    UINT8                     ZeroVector[16];
    EFI_GUID                  FileSystemGuid;
    UINT64                    FvLength;
    UINT32                    Signature;
    EFI_FVB_ATTRIBUTES_2      Attributes;
    UINT16                    HeaderLength;
    UINT16                    Checksum;
    UINT16                    ExtHeaderOffset;
    UINT8                     Reserved[1];
    UINT8                     Revision;
    EFI_FV_BLOCK_MAP_ENTRY    BlockMap[1];
} EFI_FIRMWARE_VOLUME_HEADER;

//
// Extension header pointed by ExtHeaderOffset of volume header.
// 
typedef struct
{
    EFI_GUID  FvName;
    UINT32    ExtHeaderSize;
} EFI_FIRMWARE_VOLUME_EXT_HEADER;

typedef struct
{
    UINT16 ExtEntrySize;
    UINT16 ExtEntryType;
} EFI_FIRMWARE_VOLUME_EXT_ENTRY;
//
// Define macros to build data structure signatures from characters.
//
#define EFI_SIGNATURE_16(A, B)        ((A) | (B << 8))
#define EFI_SIGNATURE_32(A, B, C, D)  (EFI_SIGNATURE_16 (A, B) | (EFI_SIGNATURE_16 (C, D) << 16))
#define EFI_FVH_SIGNATURE EFI_SIGNATURE_32 ('_', 'F', 'V', 'H')

#pragma pack(1)
//
// Used to verify the integrity of the file.
// 
typedef union
{
    struct
    {
        UINT8   Header;
        UINT8   File;
    } Checksum;
    UINT16    Checksum16;
} EFI_FFS_INTEGRITY_CHECK;
#pragma pack()

typedef UINT8 EFI_FV_FILETYPE;
typedef UINT8 EFI_FFS_FILE_ATTRIBUTES;
typedef UINT8 EFI_FFS_FILE_STATE;


//
// File Types Definitions
// 
#define EFI_FV_FILETYPE_ALL                   0x00
#define EFI_FV_FILETYPE_RAW                   0x01
#define EFI_FV_FILETYPE_FREEFORM              0x02
#define EFI_FV_FILETYPE_SECURITY_CORE         0x03
#define EFI_FV_FILETYPE_PEI_CORE              0x04
#define EFI_FV_FILETYPE_DXE_CORE              0x05
#define EFI_FV_FILETYPE_PEIM                  0x06
#define EFI_FV_FILETYPE_DRIVER                0x07
#define EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER  0x08
#define EFI_FV_FILETYPE_APPLICATION           0x09
#define EFI_FV_FILETYPE_SMM                   0x0A
#define EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE 0x0B
#define EFI_FV_FILETYPE_COMBINED_SMM_DXE      0x0C
#define EFI_FV_FILETYPE_SMM_CORE              0x0D
#define EFI_FV_FILETYPE_OEM_MIN               0xc0
#define EFI_FV_FILETYPE_OEM_MAX               0xdf
#define EFI_FV_FILETYPE_DEBUG_MIN             0xe0
#define EFI_FV_FILETYPE_DEBUG_MAX             0xef
#define EFI_FV_FILETYPE_FFS_MIN               0xf0
#define EFI_FV_FILETYPE_FFS_MAX               0xff
#define EFI_FV_FILETYPE_FFS_PAD               0xf0
// 
// FFS File Attributes.
// 
#define FFS_ATTRIB_LARGE_FILE         0x01
#define FFS_ATTRIB_FIXED              0x04
#define FFS_ATTRIB_DATA_ALIGNMENT     0x38
#define FFS_ATTRIB_CHECKSUM           0x40
//
// FFS_FIXED_CHECKSUM is the checksum value used when the
// FFS_ATTRIB_CHECKSUM attribute bit is clear
//
#define FFS_FIXED_CHECKSUM  0xAA

// 
// FFS File State Bits.
// 
#define EFI_FILE_HEADER_CONSTRUCTION  0x01
#define EFI_FILE_HEADER_VALID         0x02
#define EFI_FILE_DATA_VALID           0x04
#define EFI_FILE_MARKED_FOR_UPDATE    0x08
#define EFI_FILE_DELETED              0x10
#define EFI_FILE_HEADER_INVALID       0x20

#define EFI_FILE_ALL_STATE_BITS       (EFI_FILE_HEADER_CONSTRUCTION | \
                                 EFI_FILE_HEADER_VALID | \
                                 EFI_FILE_DATA_VALID | \
                                 EFI_FILE_MARKED_FOR_UPDATE | \
                                 EFI_FILE_DELETED | \
                                 EFI_FILE_HEADER_INVALID \
                                  )

typedef struct
{
    EFI_GUID                Name;
    EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
    EFI_FV_FILETYPE         Type;
    EFI_FFS_FILE_ATTRIBUTES Attributes;
    UINT8                   Size[3];
    EFI_FFS_FILE_STATE      State;
} EFI_FFS_FILE_HEADER;


typedef UINT8 EFI_SECTION_TYPE;

//
// Pseudo type. It is
// used as a wild card when retrieving sections. The section
// type EFI_SECTION_ALL matches all section types.
//
#define EFI_SECTION_ALL                   0x00

//
// Encapsulation section Type values
//
#define EFI_SECTION_COMPRESSION           0x01
#define EFI_SECTION_GUID_DEFINED          0x02
#define EFI_SECTION_DISPOSABLE            0x03

//
// Leaf section Type values
//
#define EFI_SECTION_PE32                  0x10
#define EFI_SECTION_PIC                   0x11
#define EFI_SECTION_TE                    0x12
#define EFI_SECTION_DXE_DEPEX             0x13
#define EFI_SECTION_VERSION               0x14
#define EFI_SECTION_USER_INTERFACE        0x15
#define EFI_SECTION_COMPATIBILITY16       0x16
#define EFI_SECTION_FIRMWARE_VOLUME_IMAGE 0x17
#define EFI_SECTION_FREEFORM_SUBTYPE_GUID 0x18
#define EFI_SECTION_RAW                   0x19
#define EFI_SECTION_PEI_DEPEX             0x1B
#define EFI_SECTION_SMM_DEPEX             0x1C

typedef struct
{
    UINT8             Size[3];
    EFI_SECTION_TYPE  Type;
} EFI_COMMON_SECTION_HEADER;

typedef struct {
  EFI_COMMON_SECTION_HEADER   CommonHeader; //(4 bytes)
  EFI_GUID                    SectionDefinitionGuid; //(16 bytes)
  UINT16                      DataOffset;
  UINT16                      Attributes;
} EFI_GUID_DEFINED_SECTION;

///
/// Firmware Volume Header Revision definition
///
#define EFI_FVH_REVISION  0x02

#endif  // __UEFI_FILE_DEF_H__

