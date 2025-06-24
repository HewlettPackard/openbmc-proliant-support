/*
// Copyright (c) 2001-2002, 2021-2025 Hewlett Packard Enterprise Development, LP
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

/****************************************************************************
*  UEFI.h
*
*  Header file for UEFI functionality
*
****************************************************************************/
#ifndef UEFI_H
   #define UEFI_H


   /****************************************************************************
   * Includes
   ****************************************************************************/
//#include "..\..\blobstore\include\blobstore.h"

   typedef struct {
       UINT32  Data1;
       UINT16  Data2;
       UINT16  Data3;
       UINT8   Data4[8];
   } EFI_GUID;

#include "uefi_file_def.h"
         
   /****************************************************************************
   * Defines
   ****************************************************************************/
   typedef enum {
       UEFI_RC_OK,
       UEFI_RC_ERROR,
       UEFI_RC_BADPARAM,
       UEFI_RC_POWER_ON,
       UEFI_RC_NOTFOUND,
       UEFI_RC_CHECKSUM_ERR,
       UEFI_RC_BADCOMMAND,
       UEFI_RC_NOMEMORY,
       UEFI_RC_NOTMODIFIED,
       UEFI_RC_BADDATA,
       UEFI_RC_COULDNOTOBTAINLOCK,
       UEFI_RC_MEDIA_ERROR,
       UEFI_RC_PARTITION_ERROR,
       UEFI_RC_END_OF_MEDIA,
       UEFI_RC_FILE_ERROR,
       UEFI_RC_PATH_ERROR,
       UEFI_RC_SIZE_ERROR,
       UEFI_RC_SPI_ERROR
   } UEFI_RC;

   typedef enum
   {
       UEFI_FS_NONE = 0,
       UEFI_FS_ILO,
       UEFI_FS_PTID,
       UEFI_FS_EEPROM,
       UEFI_FS_MAX
   } UEFI_PTID_FILE_SYSTEM;

   typedef enum
   {
       UEFI_OP_END = 0,
       UEFI_OP_MAKE_DIR,
       UEFI_OP_REMOVE_DIR,
       UEFI_OP_WRITE_FILE,
       UEFI_OP_WRITE_BLOB,
       UEFI_OP_REMOVE_FILE,
       UEFI_OP_RECURSIVE_REMOVE,
       UEFI_OP_STAIN,
       UEFI_OP_MAX
   } UEFI_PTID_OPERATION;


  /****************************************************************************
   * Structures and Types
   ****************************************************************************/
   /* Simulated boolean since there doesn't seem to be a standard one */
   typedef UINT8   uefi_bool;
#define UEFI_FALSE    0
#define UEFI_TRUE     1

#define MAX_PATH             (160)

#define VOL_DE_READ_RETRIES  (1)   // ROM Data Extract read retry count
#define VOL_DE_READ_MS_DELAY (100) // ROM Data Extract read retry delay mSecs

#define AMP_SROM_PARTS_UEFI_MAP_FILE  "i:/vol0/cfg/sromMap_details.bin"

#define MAX_RETRY_APML_FILE_ACCESS   3 //Used only in ARM based platforms.

   typedef struct {
       int      active;         // side index of active side (per NVRAM header byte)
       struct {
        int             valid;
        char            rom_sysid[8];
        unsigned int    rom_major_ver;
        unsigned int    rom_minor_ver;
        unsigned int    rom_pass;
        char            rom_date[16];
        char            rom_ver_str[32];
       } side[2];   // Side A/B  (A = 0, B = 1)
   } UEFI_BIOS_VERSION;

   // which side of the ROM do we want to address?
   typedef enum {
       UEFI_REDROM_SIDE_A,         // Side A of ROM (regardless of active/passive)
       UEFI_REDROM_SIDE_B,         // Side B of ROM (regardless of active/passive)
       UEFI_REDROM_SIDE_ACTIVE,    // Active side of ROM (based upon NVRAM header byte)
       UEFI_REDROM_SIDE_BACKUP     // Passive side of ROM (based upon NVRAM header byte)
   } UEFI_REDROM_SIDE;

   typedef struct {
      UINT32       file_position;
      union {
         struct {
             EFI_FIRMWARE_VOLUME_HEADER     volHdr;
             EFI_FIRMWARE_VOLUME_EXT_HEADER volExtHdr;
             char                           extHdr[40];
         } volInfo;
         struct {
             EFI_FFS_FILE_HEADER        fileHdr;
             EFI_COMMON_SECTION_HEADER  comHdr;
         } fileInfo;
      } header;
   } UEFI_PTID_FILE_INFO;

   // NVRAM file that holds the Platdef Offset and Size
   typedef struct
   {
       UINT32 valid;
       UINT32 offset;          //offset of bios part on BIOS ROM
       UINT32 size;            //size of bios part data
       UINT32 uefi_NOR_offset; //start addr of bios part on  UEFI ROM
       UINT32 platdef_version; //bios part version
       UINT32 reserved;
   } HPE_PLATDEFN_INFO;
   typedef struct
   {
       UINT32 valid;
       UINT32 offset;          //offset of bios part on BIOS ROM
       UINT32 size;            //size of bios part data
       UINT32 uefi_NOR_offset; //start addr of bios part on  UEFI ROM
       UINT32 version; //bios part version
       UINT32 reserved;
   } HPE_overlay_INFO;

   typedef struct
   {
       UINT32 valid;
       UINT32 offset;          //offset of bios part on BIOS ROM
       UINT32 size;            //size of bios part data
       UINT32 uefi_NOR_offset; //start addr of bios part on  UEFI ROM
       UINT32 version; //bios part version
       UINT32 reserved;
   } HPE_embed_INFO;

  typedef struct
   {
       UINT32 valid;
       UINT32 offset;          //offset of bios part on BIOS ROM
       UINT32 size;            //size of bios part data
       UINT32 uefi_NOR_offset; //start addr of bios part on  UEFI ROM
       UINT32 version; //bios part version
       UINT32 reserved;
   } HPE_EVTLOG_INFO;

typedef struct
{
    HPE_PLATDEFN_INFO platinfo_cfg;
    HPE_overlay_INFO overlay_prebootinfo_cfg;
    HPE_overlay_INFO overlay_oem_prebootinfo_cfg;
    HPE_overlay_INFO overlay_PCDinfo_cfg;
    HPE_embed_INFO embedinfo4GB_cfg;//4GB
    HPE_embed_INFO embedinfo2GB_cfg;//2G
    HPE_EVTLOG_INFO evtlog_cfg;
    UINT8 reserved[32];//for future
}HPE_BIOS_PARTS_NVRAM_CFG;


#define PLATDEF_UPDATE_BUF_SZ               (256*1024)  // Platdef buffer size //PLATDEF_UPDATE_BUF_SIZE//
#define MAGIC_NUM                           (0x5A5A)
#define PLATDEF_OFFSET_UEFISTORE            (0x10000)////160KB
#define NO_OEM_PREBOOT_OFFSET_UEFISTORE     (0x60000)//100KB
#define OEM_PREBOOT_OFFSET_UEFISTORE        (0x80000)//100KB
#define PDC_OFFSET_UEFISTORE                (0xa0000) //500bytes
#define EMBED_NAND4G_OFFSET_UEFISTORE       (0xa1000) //1056 bytes
#define EMBED_NAND2G_OFFSET_UEFISTORE       (0xa2000)//1056 bytes
#define SUPPL_DATA_OFFSET_UEFISTORE         (0xa3000) //128KB supplemental data extract from ROM FV
#define RESERVE_DATA_OFFSET_UEFISTORE       (0xc3000) //128KB supplemental data extract from ROM FV

#define ALLOCATED_PLATDEF_DATA_SIZE         0X50000
#define ALLOCATED_PREBOOT_DATA_SIZE         0X20000
#define ALLOCATED_PDC_DATA_SIZE             0X1000
#define ALLOCATED_PARTITION_DATA_SIZE       0X1000
#define ALLOCATED_SUPPL_DATA_SIZE           0X20000

#endif
