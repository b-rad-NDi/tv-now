/*****************************************************************************
 * Copyright (C) 2012- Brad Love : b-rad at next dimension dot cc
 *    Next Dimension Innovations : http://nextdimension.cc
 *    http://b-rad.cc
 * Copyright 2006 - 2011 Intel Corporation
 *
 * This file is part of TV-Now
 *   TV-Now is an Open Source DLNA Media Server. TV-Now's purpose is
 *   to serve Live TV (and recorded content) over the local network
 *   to televisions, computers, media players, tablets, and consoles.
 *   TV-Now delivers EPG data in the DLNA container for compatible
 *   clients and also offers an html5+jquery tv player with full EPG.
 *   TV-Now uses the libdvbtee library as its backend.
 *
 * TV-Now is compatible with:
 *   - ATSC
 *   - Clear QAM
 *   - DVB-T
 *
 * TV-Now is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Note: All additional terms from Section 7 of GPLv3 apply to this software.
 * This includes requiring preservation of specified reasonable legal
 * notices or author attributions in the material or in the Appropriate
 * Legal Notices displayed by this software.
 *
 * You should have received a copy of the GNU General Public License v3
 * along with TV-Now.  If not, see <http://www.gnu.org/licenses/>.
 *
 * An Apache 2.0 licensed version of this software is privately maintained
 * for licensing to interested commercial parties. Apache 2.0 license is
 * compatible with the GPLv3, which allows the Apache 2.0 version to be
 * included in proprietary systems, while keeping the public GPLv3 version
 * completely open source. GPLv3 can NOT be re-licensed as Apache 2.0, since
 * Apache 2.0 license is only a subset of GPLv3. To inquire about licensing
 * the commercial version of TV-Now contact:  tv-now at nextdimension dot cc
 *
 * Note about contributions and patch submissions:
 *  The commercial Apache 2.0 version of TV-Now is used as the master.
 *  The GPLv3 version of TV-Now will be identical to the Apache 2.0 version.
 *  All contributions and patches are licensed under Apache 2.0
 *  By submitting a patch you implicitly agree to
 *    http://www.apache.org/licenses/icla.txt
 *  You retain ownership and when merged the license will be upgraded to GPLv3.
 *
 *****************************************************************************/

#ifndef PORTINGFUNCTIONS_H
#define PORTINGFUNCTIONS

#include <inttypes.h>

#define NDi_LiveTV 1
struct CdsMediaObject;

void PCRandomize();

/*
 *	Use to a close a file after streaming is complete.
 *		fileHandle returned from PCFileOpen.
 */
int PCFileClose(void *fileHandle);

/*
 *	Use to open a file for streaming.
 *		Returns a void* fileHandle for use with other PCFilexxx methods.
 *		Arguments behave exactly like fopen().
 */
void* PCFileOpen(const char* fullPath, const char *mode);

/*
 *	Use to read a file.
 *		fileHandle returned from PCFileOpen.
 *		Other arguments behave exactly like fread().
 */
int PCFileRead(void *dest, int itemSize, int itemCount, void *fileHandle);

/*
 *	Use to move the file pointer to a specific offset.
 *		fileHandle returned from PCFileOpen.
 *		Other arguments behave exactly like fread().
 */
int PCFileSeek(void *fileHandle, uint64_t offset, int origin);

/*
 *	Use to obtain the current file pointer position.
 *		fileHandle returned from PCFileOpen.
 */
uint64_t PCFileTell(void *fileHandle);

/*
 *   Use to obtain expanded metadata fields.
 *      path is analyzed, cdsObj is filled where desired
 */
void GetMetaData(const char* path, struct CdsMediaObject *cdsObj);


/*
 *	Returns a state number for the file system.
 *	Ideally, we would return the most recent file-system
 *	date to indicate the latest state. Currently we only
 *	return 0.
 */
unsigned int PCGetSystemUpdateID();

/*
 *	Returns a state number for a directory, given a path.
 *	Ideally, we would return the msot recent date for an entry
 *	within the directory, but currently we only return 0.
 */
unsigned int PCGetContainerUpdateID(const char* path);

/*
 *	Returns an integer describing the type of content at this path.
 *		0 = Does Not Exist
 *		1 = Is a File
 *		2 = Is a Directory
 *
 *	Provided path is a UTF8-encoded path into the file system.
 */
int PCGetFileDirType(char* path);

/*
 *	Closes a directory or specific path.
 *
 *	dirHandle returned from PCGetDirFirstFile.
 */
void PCCloseDir(void* dirHandle);

/*
 *	Opens a directory for enumeration or a specific file.
 *		Returns void* dirHandle.
 *
 *		directory		is the directory portion of the path
 *		filename		allocated byte array, contains the first filename in that directory path upon return
 *		filenamelength	the number of bytes available in filename
 *		fileSize		the size of the file (specified by dirName+filename)
 */
void* PCGetDirFirstFile(const char* dirName, /*INOUT*/char* filename, /*IN*/int filenamelength, /*INOUT*/uint64_t* fileSize);

/*
 *	Obtains the next entry in the directory.
 *		dirHandle		the void* returned in PCGetDirFirstFile
 *		dirName			the directory name
 *		filename		allocated byte array, contains the next filename in that directory path upon return
 *		filenamelength	the number of bytes available in filename
 *		fileSize		the size of the file (specified by dirName+filename)
 */
int PCGetDirNextFile(void* dirHandle, const char* dirName, /*INOUT*/char* filename, int filenamelength, uint64_t* fileSize);

/*
 *	Obtains the filesize of the file specified by fullpath.
 */
uint64_t PCGetFileSize(const char* fullPath);

/*
 *	Returns the number of entries specified in fullPath.
 *
 *		dirDelimiter	single character to identify the character used to delimit directories within a path.
 *						Posix usually uses '/' whereas windows usually uses '\'
 */
int PCGetGetDirEntryCount(const char* fullPath, char *dirDelimiter);

/* \brief Grab the contents of a file into a char* and return the filesize
 * \param[in] filename  location of file, should be text file only!
 * \param[out] result   pointer to pointer is passed in, function mallocs and fills it
 */
int file_get_contents(const char *filename, /*OUT*/char **result);

void EndThisThread();
void LockBrowse();
void UnlockBrowse();
void InitBrowseLock();
void DestroyBrowseLock();
void* SpawnNormalThread(void* method, void* arg);
int ProceedWithDirEntry(const char* dirName, const char* filename, int maxPathLength);
#endif
