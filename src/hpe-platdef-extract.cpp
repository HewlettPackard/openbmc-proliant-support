/*
// Copyright (c) 2001, 2023-2025 Hewlett Packard Enterprise Development, LP
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

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <strings.h>
#include <unistd.h>
#include <stdint-gcc.h>

typedef uint64_t   UINT64;
typedef uint32_t   UINT32;
typedef uint16_t   UINT16;
typedef uint8_t    UINT8;

typedef int32_t    INT32;
typedef int16_t    INT16;
typedef int8_t     INT8;


#include "uefi_platdef.h"

// GUIDs
// {8C8CE578-8A3D-4f1c-9935-896185C32DD3}
const EFI_GUID EFI_FIRMWARE_FILE_SYSTEM2_GUID = // indicator of UEFI FFS FV format
{ \
    0x8c8ce578, 0x8a3d, 0x4f1c, {0x99, 0x35, 0x89, 0x61, 0x85, 0xc3, 0x2d, 0xd3 } \
};

// {1BA0062E-C779-4582-8566-336AE8F78F09}
const EFI_GUID EFI_FFS_VOLUME_TOP_FILE_GUID =
{ \
    0x1BA0062E, 0xC779, 0x4582, {0x85, 0x66, 0x33, 0x6A, 0xE8, 0xF7, 0x8F, 0x9 } \
};

// {FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF}
const EFI_GUID EFI_FFS_GARBAGE_FILE_GUID =
{ \
    0xFFFFFFFF, 0xFFFF, 0xFFFF, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } \
};

const EFI_GUID EFIGUID_FV_APML =  // APML firmware volume
{ 0x7EBF5AB8, 0x525E, 0x417C, { 0x9B, 0x6B, 0x5E, 0xF3, 0x67, 0x85, 0x69, 0x54 }};

const EFI_GUID EFIGUID_File_APML =  // APML binary
{ 0xC5F6001C, 0x39B4, 0x43DD, { 0x9B, 0x9B, 0x68, 0x32, 0xF1, 0xBB, 0x4B, 0xE9 }};

const EFI_GUID EFIGUID_FV_OEM_APML =  // Optional OEM APML firmware volume
{ 0x5A515240, 0xD1F1, 0x4C58, { 0x95, 0x90, 0x27, 0xB1, 0xF0, 0xE8, 0x68, 0x27}};

const EFI_GUID EFIGUID_File_OEM_APML =  // Optional OEM APML binary
{ 0xC4D3B6FA, 0x451A, 0x36FC, { 0x63, 0x8D, 0x7E, 0xA0, 0xAB, 0x27, 0x8A, 0x11}};

#define ROM_IMAGE_START 0x2000000  //32M
#define ROM_IMAGE_SIZE  0x4000000  //64M

#define PLATDEF_UPDATE_BUF_SZ       (256*1024)

UINT8 platbuf[PLATDEF_UPDATE_BUF_SZ];

#define USLEEP_MILLISECONDS 1000

#define MAX_DEVICE_NAME_LEN 256

#define HOST_PRIME_PATH     "/dev/mtd/by-name/host-prime"

static inline void Sleep_Ms(const unsigned int milliseconds)
{
    usleep( milliseconds * USLEEP_MILLISECONDS );
}

static UEFI_RC uefi_util_file_find( const EFI_GUID* pFwVolGUID, const EFI_GUID* pFwFileGUID,
                                    UINT32* pOffset, UINT32* pSize, UINT8* pChecksum )
{
    UEFI_RC rc = UEFI_RC_NOTFOUND;
    int read_result, write_result;
    UINT32 offset = 0;
    UINT32 i;
    UINT32 fileoffset = 0;
    UINT32 FVOffset;
    UINT32 FVStart;
    UINT32 FVOffsetEnd;
    UINT32 getFV;
    FILE *fptr, *pfptr;
   
    union {
        EFI_FIRMWARE_VOLUME_HEADER      FvHeader;
        struct {  // a combo of the ext header + 1st entry header
            EFI_FIRMWARE_VOLUME_EXT_HEADER  FvExtHeader;
            EFI_FIRMWARE_VOLUME_EXT_ENTRY   FvExtEntry;
        }                               FvExt;
        EFI_FIRMWARE_VOLUME_EXT_ENTRY   FvExtEntry;
        struct {
            EFI_FFS_FILE_HEADER             FfsFileHeader;
            EFI_COMMON_SECTION_HEADER       SectionHeader;
        } FvFile;
    } myBuffer;
    UINT32 filesize = 0;
    UINT8  file_checksum = 0;
    UINT32 fvlength;
    EFI_GUID EFIGUID_Get_FV = { 0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }};

    if( pFwVolGUID == NULL || pFwFileGUID == NULL ) {
        printf("uefi_bad_param\n");
        return UEFI_RC_BADPARAM;
    }

    printf("uefi_util_file_find\n");

    // For Gen12, the rom image is in the upper 32M of the 64M part.
    FVOffset = ROM_IMAGE_START;
    FVOffsetEnd = ROM_IMAGE_SIZE;

    // Caller desires FV offset/length
    getFV = (memcmp(pFwFileGUID, &EFIGUID_Get_FV, sizeof(EFI_GUID)) == 0) ? 1 : 0;

    printf("Reading from %s starting at: FVOffset: %d, FVOffsetEnd: %d\n", HOST_PRIME_PATH, FVOffset, FVOffsetEnd);

    fptr = fopen(HOST_PRIME_PATH, "rb");
    if (!fptr) {
        printf("Failed to open device: %s\n", HOST_PRIME_PATH);
        return UEFI_RC_ERROR;
    }

    while( FVOffset < FVOffsetEnd ) {

        printf("read bios file from offset %x\n", FVOffset);
        if (fseek(fptr, FVOffset, SEEK_SET)) {
            printf("Failed to seek to offset %x\n", FVOffset);
            fclose(fptr);
            return UEFI_RC_ERROR;
        }

        read_result = fread((UINT8*) &myBuffer.FvHeader, 1, sizeof(myBuffer.FvHeader), fptr);
        if( read_result != 0 ) {

            printf("read success\n");
            offset = FVOffset;
            FVStart = FVOffset;

            // Assume the next FV to examine is +64KB
            if( myBuffer.FvHeader.Signature != EFI_FVH_SIGNATURE ) {
                printf("no EFI_FVH_SIGNATURE\n");
                FVOffset += 0x10000;
                continue;
            }

            if( memcmp(&myBuffer.FvHeader.FileSystemGuid, &EFI_FIRMWARE_FILE_SYSTEM2_GUID, sizeof(EFI_GUID)) != 0 ) {
                printf("EFI_FIRMWARE_FILE_SYSTEM2_GUID no match - continue\n");
                FVOffset += 0x10000;
                continue;
            } else {
                printf("EFI_FIRMWARE_FILE_SYSTEM2_GUID match found\n");
            }

            // checksum check the FV header (16 bit walk, 16 bit sum)
            {
                UINT16 sum = 0;
                UINT16* ptr = (UINT16 *) &myBuffer.FvHeader;

                for( i = 0; i < sizeof(EFI_FIRMWARE_VOLUME_HEADER) / sizeof(UINT16); i++ ) {
                    sum += ptr[i];
                }

                if( sum != 0 ) {
                    printf("UEFI FV header checksum error:  offset=%u, size=%lu, checksum=%u\n",
                        offset, sizeof(EFI_FIRMWARE_VOLUME_HEADER), sum);
                    return UEFI_RC_CHECKSUM_ERR;
                }
            }

            // Find FV length
            // we can't really support fvlength > 32 bit value
            if( myBuffer.FvHeader.FvLength > 0xFFFFFFFF ) {
                printf("fvlength > 32 bit value \n");
                return UEFI_RC_ERROR;
            }
            fvlength = (UINT32) myBuffer.FvHeader.FvLength;

            // Assume the next FV to examine is past this FV aligned to 64KB
            if( fvlength & 0x0FFFF ) {
                fvlength = (fvlength & ~0x0FFFF) + 0x10000;
            }

            FVOffset += fvlength;
            printf("FVOffset changed to: %x\n", FVOffset);

            // If no extended header we are not interested
            if( myBuffer.FvHeader.ExtHeaderOffset == 0 ) {
                printf("no extended header \n");
                continue;
            }

            printf("offset: %x, myBuffer.FvHeader.ExtHeaderOffset: %x\n", offset, myBuffer.FvHeader.ExtHeaderOffset);

            // read the extension header + first extention entry
            offset += myBuffer.FvHeader.ExtHeaderOffset;
            printf("reading FvExt from offset: %x\n", offset);

            if (fseek(fptr, offset, SEEK_SET)) {
                fclose(fptr);
                return UEFI_RC_ERROR;
            }

            printf("reading from: %x\n", offset);

            read_result = fread((UINT8*) &myBuffer.FvExt, 1, sizeof(myBuffer.FvExt), fptr);
            if( read_result == 0 ) {
                printf("1 File read fail result %d\n", read_result);
                fclose(fptr);
                return UEFI_RC_ERROR;
            }

            // Test for requested volume name
            if( memcmp(&myBuffer.FvExt.FvExtHeader.FvName, pFwVolGUID, sizeof(EFI_GUID)) != 0 ) {
                printf("Test for requested volume name failed, continueing...\n");
                continue;
            }

            // Some callers want FV information, not file
            if( getFV ) {
                fileoffset = FVStart;
                filesize = fvlength;
                file_checksum = 0;
                rc = UEFI_RC_OK;
                printf("Got FV, returning...\n");
                break;
            }

            // offset to
            offset += myBuffer.FvExt.FvExtHeader.ExtHeaderSize;
            printf("walking the file entries\n");
            // now walk the file entries
            do {
                int is_top_volume_file = 0;

                if( offset & 0x07 ) {
                    offset = (offset & ~0x07) + 0x08;   // 8 byte align
                }

                if (fseek(fptr, offset, SEEK_SET)) {
                    fclose(fptr);
                    return UEFI_RC_ERROR;
                }

                read_result = fread((UINT8*) &myBuffer.FvFile, 1, sizeof(myBuffer.FvFile), fptr);
                if (read_result == 0) {
                    printf("uefi_util_file_find, spiread fail result: %d\n", read_result);
                    fclose(fptr);
                    return UEFI_RC_ERROR;
                }

                // check for EFI_FFS_GARBAGE_FILE_GUID
                // because the ISS UEFI ROM seems to just leave FFs after the file array
                if (memcmp(&myBuffer.FvFile.FfsFileHeader.Name, &EFI_FFS_GARBAGE_FILE_GUID, sizeof(EFI_GUID)) == 0) {
                    printf("found EFI_FFS_GARBAGE_FILE_GUID, bailing... \n");
                    break;
                }

                // check for EFI_FFS_VOLUME_TOP_FILE_GUID
                if( memcmp(&myBuffer.FvFile.FfsFileHeader.Name, &EFI_FFS_VOLUME_TOP_FILE_GUID, sizeof(EFI_GUID)) == 0 )
                    is_top_volume_file = 1;

                filesize = ((int) myBuffer.FvFile.FfsFileHeader.Size[2]) << 16;
                filesize |= ((int) myBuffer.FvFile.FfsFileHeader.Size[1]) << 8;
                filesize |= ((int) myBuffer.FvFile.FfsFileHeader.Size[0]);
                filesize -= sizeof(EFI_FFS_FILE_HEADER);
                offset += sizeof(EFI_FFS_FILE_HEADER);

                // if this is the matching file name, and is valid
                if( memcmp(&myBuffer.FvFile.FfsFileHeader.Name, pFwFileGUID, sizeof(EFI_GUID)) == 0 &&
                    //(myBuffer.FvFile.FfsFileHeader.State & EFI_FILE_HEADER_VALID) &&
                    //(myBuffer.FvFile.FfsFileHeader.State & EFI_FILE_BODY_VALID) &&
                    (myBuffer.FvFile.FfsFileHeader.Attributes & FFS_ATTRIB_LARGE_FILE) == 0 ) {

                    UINT8 header_checksum = 0;

                    // make note of the file checksum
                    file_checksum = myBuffer.FvFile.FfsFileHeader.IntegrityCheck.Checksum.File;

                    // Verify Header Checksum
                    {
                        UINT8* buffer = (UINT8*) &myBuffer.FvFile.FfsFileHeader;

                        // Assume (per spec) that these two fields are assumed to be zero
                        myBuffer.FvFile.FfsFileHeader.IntegrityCheck.Checksum.File = 0;
                        myBuffer.FvFile.FfsFileHeader.State = 0;

                        for( i = 0; i < sizeof(myBuffer.FvFile.FfsFileHeader); i++ )
                            header_checksum += buffer[i];
                    }

                    if( header_checksum == 0 ) {
                        printf("header checksum passed\n");

                        // EFI_FV_FILETYPE_RAW:  following bits are the file contents
                        if( myBuffer.FvFile.FfsFileHeader.Type == EFI_FV_FILETYPE_RAW ) {
                            printf("EFI_FV_FILETYPE_RAW found\n");

                            //its all good - the following bits are the file
                            fileoffset = offset;
                            rc = UEFI_RC_OK;
                        }
                        // EFI_FV_FILETYPE_FREEFORM:  parse section header for contents
                        else if( myBuffer.FvFile.FfsFileHeader.Type == EFI_FV_FILETYPE_FREEFORM ) {
                            printf("EFI_FV_FILETYPE_FREEFORM found\n");

                            // make sure it is a RAW section header
                            if( myBuffer.FvFile.SectionHeader.Type == EFI_SECTION_RAW ) {
                                printf("EFI_SECTION_RAW found\n");

                                // skip EFI_COMMON_SECTION_HEADER
                                offset += sizeof(EFI_COMMON_SECTION_HEADER);
                                fileoffset = offset;

                                // back off the file size by the section header size (value includes section header)
                                filesize -= sizeof(EFI_COMMON_SECTION_HEADER);

                                // subtract out the section header bytes from the checksum
                                file_checksum -= myBuffer.FvFile.SectionHeader.Type;
                                file_checksum -= myBuffer.FvFile.SectionHeader.Size[0];
                                file_checksum -= myBuffer.FvFile.SectionHeader.Size[1];
                                file_checksum -= myBuffer.FvFile.SectionHeader.Size[2];

                                rc = UEFI_RC_OK;
                            }
                            // GUIDed defined section
                            else if (myBuffer.FvFile.SectionHeader.Type == EFI_SECTION_GUID_DEFINED) {

                                printf("EFI_SECTION_GUID_DEFINED\n");

                                // skip EFI_GUID_DEFINED_SECTION
                                offset += sizeof(EFI_GUID_DEFINED_SECTION);
                                fileoffset = offset;

                                // back off the file size by the section header size (value includes section header)
                                filesize -= sizeof(EFI_GUID_DEFINED_SECTION);

                                // subtract out the section header bytes from the checksum
                                file_checksum -= myBuffer.FvFile.SectionHeader.Type;
                                file_checksum -= myBuffer.FvFile.SectionHeader.Size[0];
                                file_checksum -= myBuffer.FvFile.SectionHeader.Size[1];
                                file_checksum -= myBuffer.FvFile.SectionHeader.Size[2];
                                // TODO: Correct checksum

                                rc = UEFI_RC_OK;
                            }

                        } else {
                            printf("unsupported EFI_FV_FILETYPE\n");
                            ;// epic fail - unsupported EFI_FV_FILETYPE_xxx
                        }

                    } else {
                        printf("UEFI file header checksum error:  offset=%u, size=%lu, checksum=%u\n",
                            offset, sizeof(myBuffer.FvFile.FfsFileHeader), header_checksum);
                        rc = UEFI_RC_CHECKSUM_ERR;
                    }

                    // Regardless of success, at this point we are finished.
                    break;

                } else {
                    printf("no matching file name...\n");
                }

                // if is last file, break out without success
                if( is_top_volume_file )
                    break;

                // update offset and look at next file
                offset += filesize;

            } while( offset < FVOffset );

            // if found, we are finished
            // if checksum error we are also finished
            if( rc == UEFI_RC_OK || rc == UEFI_RC_CHECKSUM_ERR )
                break;

        } else {
            printf("File Read Failed: %d\n",read_result);
            break;
        }
    }

    if (pOffset) {
        *pOffset = (rc == UEFI_RC_OK) ? fileoffset : 0;
    }

    if (pSize) {
        *pSize = (rc == UEFI_RC_OK) ? filesize : 0;
    }

    if (pChecksum) {
        *pChecksum = (rc == UEFI_RC_OK) ? file_checksum : 0;
    }

    pfptr = fopen(PLATDEF_DATA_FILE, "wb");
    if (!pfptr) {
        printf("Failed to create %s\n", PLATDEF_DATA_FILE);
        return UEFI_RC_ERROR;
    }

    // seek to where the platdef data is stored
    if (fseek(fptr, *pOffset, SEEK_SET)) {
        printf("Failed final seek to offset %d\n", FVOffset);
        fclose(fptr);
        fclose(pfptr);
        return UEFI_RC_ERROR;
    }

    // read all of the compressed platdef data into the platdef buffer.
    read_result = fread(platbuf, 1, *pSize, fptr);
    if( read_result != 0 ) {
        // now write it out to a file
        write_result = fwrite(platbuf, 1, *pSize, pfptr);
        if (write_result != (int)*pSize) {
            printf("Failed to write out all platdef data\n");
        }
        fflush(pfptr);
    }

    fclose(pfptr);
    fclose(fptr);
    printf("uefi util find return code: %d\n",rc);
    return rc;
}

UEFI_RC uefi_util_file_find_with_retries( const EFI_GUID* pFwVolGUID, const EFI_GUID* pFwFileGUID,
                                          UINT32* pOffset, UINT32* pSize, UINT8* pChecksum,
                                          unsigned int retry_count, unsigned int ms_retry_interval )
{
    UINT32  retry= 5;
    UEFI_RC rc = UEFI_RC_ERROR;

    printf("\nSTART uefi_util_file_find_with_retries\n");
    do {
        rc = uefi_util_file_find(pFwVolGUID, pFwFileGUID, pOffset, pSize, pChecksum);
        if( rc == UEFI_RC_OK ) {
            printf("found uefi_util\n");
            break;
        } else {
            Sleep_Ms(ms_retry_interval);
        }
    } while( retry++ < retry_count );

    printf("uefi_util_file_find_with_retries, RETURN %d\n", rc);
    return rc;
}

/***********************************************************
 * main (uefi_util_platdef_store):
 *
 * This API finds the APML FW volume in BIOS Image.
 * Once it finds the APML FW volume it copies the
 * APML content into UEFI Var Store.
 *
 * Also the offset and sizes are updated in NVRAM
 * config file for use while loading Platdef
 ***********************************************************/

int main(void)
{
    UINT32 offset, size;
    HPE_BIOS_PARTS_NVRAM_CFG bios_parts_nvram_cfg;
    UEFI_RC rc;
    UINT8 checksum;

    memset(&bios_parts_nvram_cfg,0,sizeof(bios_parts_nvram_cfg));
    printf("UEFI: Check for APML file in NAND.\n");

    rc = uefi_util_file_find_with_retries(&EFIGUID_FV_APML, &EFIGUID_File_APML, &offset, &size, &checksum, VOL_DE_READ_RETRIES, VOL_DE_READ_MS_DELAY);
    if (rc && (rc != UEFI_RC_POWER_ON)) {
        printf("UEFI: Base Platdef not found. Unexpected!\n");
        return UEFI_RC_ERROR;
    }

    // At this point the compressed platdef data is in a file
    // so all we need to do is read it in and uncompress it into the memory buffer.

    return (int)rc;
}
