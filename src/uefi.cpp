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

#include <ctype.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream> // std::cout
#include <sstream>  // std::stringstream, std::stringbuf
#include <string>
#include <vector>

#include "uefi.h"

const char* FFS2 = "8c8ce578-8a3d-4f1c-9935-896185c32dd3";

static std::string GUIDtoChar(struct GUID buffer)
{
    char ptr[GUID_LEN + 1];
    sprintf(&ptr[0], "%08X-%04X-%04X-", buffer.start, buffer.mid, buffer.mid2);
    for (int j = 0; j < 2; j++)
    {
        sprintf(&ptr[GUID_LEN - 1 - 16 + 2 * j], "%02X",
                buffer.remaining[j] & 0xFF);
    }
    sprintf(&ptr[GUID_LEN - 1 - 12], "-");
    for (int j = 0; j < 6; j++)
    {
        sprintf(&ptr[GUID_LEN - 12 + 2 * j], "%02X",
                buffer.remaining[2 + j] & 0xFF);
    }
    // Need to find now if their is any FFS into the FV
    ptr[GUID_LEN] = '\0';
    return (std::string(ptr));
}

UEFI::FVFile::FVFile(const char* bfr, long offset)
{
    long fileOffset = offset;
    struct uefiFileHeader myfile;
    memcpy(&myfile, bfr + offset, sizeof(struct uefiFileHeader));
    if (myfile.Type == EFI_FV_FILETYPE_FREEFORM)
    {
        // We could have some section
        // Let's read the section header
        struct sectionHeader myHeader;
        fileOffset = fileOffset + sizeof(struct uefiFileHeader);
        // sections are 4 bytes aligned
        fileOffset = (fileOffset + (4 - 1)) & -4;
        memcpy(&myHeader, bfr + fileOffset, sizeof(struct sectionHeader));
        _Size = 0;
        _Size =
            myHeader.Size[2] << 16 | myHeader.Size[1] << 8 | myHeader.Size[0];
        // Let's go to the next entry
        fileOffset = fileOffset + _Size + sizeof(struct sectionHeader);
	_GUID = GUIDtoChar(myfile.fileID);

	fileOffset = (fileOffset + (fileOffset % 8)) & -8  ;

        _GUID = GUIDtoChar(myfile.fileID);
        _NextEntryOffset = fileOffset;
    }
    else
    {
        _Size = 0;
        _Size = myfile.Size[2] << 16 | myfile.Size[1] << 8 | myfile.Size[0];
        _GUID = GUIDtoChar(myfile.fileID);
        _Content = (void*)(bfr + fileOffset + sizeof(struct uefiFileHeader));
        fileOffset = fileOffset + _Size;
        fileOffset = (fileOffset + (8 - 1)) & -8;
        _NextEntryOffset = fileOffset;
    }
}
long UEFI::FVFile::next()
{
    return _NextEntryOffset;
}
void* UEFI::FVFile::Content()
{
    return _Content;
}
uint32_t UEFI::FVFile::Size()
{
    return _Size;
}
std::string UEFI::FVFile::GUID()
{
    return (std::string(this->_GUID));
}

UEFI::FileVolume::FileVolume(long offset, const char* bfr)
{
    const char* ptr = bfr + offset;
    std::string currentGUID;
    std::string FileSystemGUID;
    struct firmwareVolumeHeader currentHeader;
    struct firmwareVolumeExtHeader currentExtHeader;
    memcpy(&currentHeader, ptr, sizeof(struct firmwareVolumeHeader));
    if (currentHeader.ExtHeaderOffset == 0)
    {
        currentGUID = GUIDtoChar(currentHeader.FileSystemGUID);
        _GUID = currentGUID;
        _Length = currentHeader.Length;
        _Offset = offset;
    }
    else
    {
        FileSystemGUID = GUIDtoChar(currentHeader.FileSystemGUID);

        // Just looking for FFS/FFS2/FFS3 at the moment
        if ((strcmp(FileSystemGUID.c_str(),
                    "5473C07A-3DCB-4DCA-BD6F-1E9689E7349A") == 0) ||
            (strcmp(FileSystemGUID.c_str(),
                    "7A9354D9-0468-444A-81CE-0BF617D890DF") == 0) ||
            (strcmp(FileSystemGUID.c_str(),
                    "8C8CE578-8A3D-4F1C-9935-896185C32DD3") == 0))
        {
            // The File Volume GUID is the one specified into the Extended
            // offset
            if (currentHeader.ExtHeaderOffset != 0)
            {
                memcpy(&currentExtHeader, ptr + currentHeader.ExtHeaderOffset,
                       sizeof(struct firmwareVolumeExtHeader));
                currentGUID = GUIDtoChar(currentExtHeader.FVNameGUID);
                _GUID = currentGUID;
                _Length = currentHeader.Length;
                _Offset = offset;

                if (currentGUID != "")
                {
                    // We need now to look for the content and register it to
                    // the current FV

                    uint64_t fileOffset;
                    uint64_t addr;

                    // The Firmware File System contains some file
                    // Let's jump to the first one

                    fileOffset = currentHeader.ExtHeaderOffset +
                                 sizeof(struct firmwareVolumeExtHeader);
                    fileOffset = (fileOffset + (8 - 1)) & -8;
                    addr = offset + fileOffset;

                    //*DEBUGstruct uefiFileHeader myfile;

                    //*DEBUGuint32_t size;

                    // if size is smaller than the FV then we can loop for
                    // another file we have to scan all files contained into the
                    // current FileVolume Scanning address can't be bigger than
                    // the base offset of the FileVolume added by its length

                    while (addr < (offset + currentHeader.Length))
                    {

                        FVFile newFile(bfr, addr);
                        if (newFile.next() > 0)
                        {
                            addr = newFile.next();
                        }
                        else
                            break;
                        _LocalFiles.push_back(newFile);
                    }
                }
            }
        }
    }
}

UEFI::FileVolume::~FileVolume()
{}

std::string UEFI::FileVolume::GUID()
{
    return (std::string(this->_GUID));
}

uint64_t UEFI::FileVolume::Length()
{
    return this->_Length;
}

uint64_t UEFI::FileVolume::Offset()
{
    return this->_Offset;
}

std::string UEFI::FileVolume::getCurrentLocalFilesGUID()
{
    if (_LocalFiles.size() > 0)
    {
        if (_CurrentFile < _LocalFiles.size())
        {
            return (std::string(_LocalFiles[_CurrentFile++].GUID()));
        }
        else
        {
            _CurrentFile = 0;
            return "";
        }
    }
    else
        return "";
}

uint32_t UEFI::FileVolume::Size()
{
    return (this->_Length);
}

void* UEFI::FileVolume::getContent(std::string GUID)
{
    std::vector<FVFile>::iterator it;
    void* returnValue = NULL;
    for (it = _LocalFiles.begin(); it != _LocalFiles.end(); ++it)
    {
        if (it->GUID() == GUID)
        {
            returnValue = it->Content();
        }
    }
    return returnValue;
}

uint32_t UEFI::FileVolume::getSize(std::string GUID)
{
    std::vector<FVFile>::iterator it;
    uint32_t returnValue = 0;
    for (it = _LocalFiles.begin(); it != _LocalFiles.end(); ++it)
    {
        if (it->GUID() == GUID)
        {
            returnValue = it->Size();
        }
    }
    return returnValue;
}

UEFI::UEFIFirmwareImage::UEFIFirmwareImage(std::string path)
{
    int f, err;

    f = open(path.c_str(), O_RDONLY);
    if (f == -1)
        throw; //"Error: Can't open %s", path.c_str();
    _Rom = (char*)malloc(sizeof(const char) * ROM_SIZE);
    if (_Rom == NULL)
        throw; //"error: Can't allocate %ul bytes \n", sizeof(const char) *
               //ROM_SIZE;
    err = read(f, (void*)_Rom, sizeof(const char) * ROM_SIZE);
    if (err == -1)
        throw; //"error: Can't read from %s\n", path.c_str();

    for (unsigned long i = 0; i < (sizeof(const char) * ROM_SIZE - 4); i++)
    {
        if (strncmp(&_Rom[i], fvSignature, 4) == 0)
        {
            FileVolume LocalFv(i - 40, _Rom);
            if (strlen(LocalFv.GUID().c_str()) > 0)
            {
                UefiFV.push_back(LocalFv);
                i = i + LocalFv.Length() - 40;
            }
        }
    }
    close(f);
}

void UEFI::UEFIFirmwareImage::dumpRomFV()
{
    std::vector<FileVolume>::iterator it;
    for (it = UefiFV.begin(); it != UefiFV.end(); ++it)
    {
        printf("FileVolume %s Found at %lx with a size of %lx\n",
               it->GUID().c_str(), it->Offset(), it->Length());
        std::string currentLocalFileGUID = it->getCurrentLocalFilesGUID();
        while (currentLocalFileGUID != "")
        {
            printf("\tFile: %s\n", currentLocalFileGUID.c_str());
            currentLocalFileGUID = it->getCurrentLocalFilesGUID();
        }
    }
}

void* UEFI::UEFIFirmwareImage::getFileContent(std::string GUID)
{
    std::vector<FileVolume>::iterator it;
    void* returnValue = NULL;
    for (it = UefiFV.begin(); it != UefiFV.end(); ++it)
    {
        returnValue = it->getContent(GUID);
        if (returnValue != NULL)
            return (returnValue);
    }
    return (returnValue);
}
uint32_t UEFI::UEFIFirmwareImage::getFileSize(std::string GUID)
{
    std::vector<FileVolume>::iterator it;
    uint32_t returnValue = 0;
    for (it = UefiFV.begin(); it != UefiFV.end(); ++it)
    {
        returnValue = it->getSize(GUID);
        if (returnValue != 0)
            return (returnValue);
    }
    return (0);
}

UEFI::UEFIFirmwareImage::~UEFIFirmwareImage()
{
    if (_Rom != NULL)
        free(_Rom);
}

