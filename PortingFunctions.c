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

#define LIVETV_FILESIZE 9384503541759 / 10

#define _CRTDBG_MAP_ALLOC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MyString.h"
#include "MicroMediaServer.h"
#include "CdsMediaObject.h"
#include "MimeTypes.h"

#include "PortingFunctions.h"

#include "dvbteeserver.h"

/* Windows 32 */
#ifdef WIN32
#include <windows.h>
#include <time.h>
#include <crtdbg.h>
#endif

/* Win CE */
#ifdef UNDER_CE
#include <Winbase.h>
#endif

/* POSIX */
#ifdef _POSIX
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#endif

/* tv-now root directories */
#define TVNOW_ROOT_ITEMS 3
static int rootIter = 0;
static char rootDirs[TVNOW_ROOT_ITEMS][12] = { "Channels", "EPG", "VOD" };

#define MAX_PATH_LENGTH 1024

void PCRandomize()
{
#ifdef WIN32
	srand((int)GetTickCount());
#endif

#ifdef UNDER_CE
	srand((int)GetTickCount());
#endif

#ifdef _POSIX
	struct timeval tv;
	gettimeofday(&tv,NULL);
	srand((int)tv.tv_sec);
#endif
}

// Windows Version
void PCCloseDir(void* handle)
{
	printf("%s\n", __func__);
#if NDi_LiveTV
	return;
#endif
#ifdef WIN32
	FindClose(*((HANDLE*)handle));
	free(handle);
#endif

#ifdef UNDER_CE
	FindClose(*((HANDLE*)handle));
	free(handle);
#endif

#ifdef _POSIX
	DIR* dirObj = (DIR*) handle;
	closedir(dirObj);
#endif
}

int PCFileClose(void *fileHandle)
{
	printf("%s\n", __func__);
	/* TODO: Modify to enable unicode char support */
	return fclose((FILE*)fileHandle);
}

void* PCFileOpen(const char* fullPath, const char *mode)
{
	printf("%s\n", __func__);
	/* TODO: Modify to enable unicode char support */
	return (void*) fopen64(fullPath, mode);
}

int PCFileRead(void*dest, int itemSize, int itemCount, void *fileHandle)
{
	printf("%s\n", __func__);
	/* TODO: Modify to enable unicode char support */
	return (int) fread(dest, itemSize, itemCount, (FILE*)fileHandle);
}

int PCFileSeek(void *fileHandle, uint64_t offset, int origin)
{
	printf("%s\n", __func__);
	/* TODO: Modify to enable unicode char support */
	return fseek((FILE*)fileHandle, offset, origin);
}

uint64_t PCFileTell(void *fileHandle)
{
	printf("%s\n", __func__);
	/* TODO: Modify to enable unicode char support */
	return ftello((FILE*)fileHandle);
}

static int isDate(const char* date_string)
{
	const char *time_details = date_string;
	char* endPtr;
	struct tm tm;
//	printf("%s( %s )\n", __func__, date_string);

	endPtr = strptime(time_details, "%m-%d-%Y", &tm);
	if (endPtr == NULL || *endPtr != '\0')
	{
		printf("%s( %s ) - Error\n", __func__, date_string);
		return 0;
	}
	return 1;
}

/* Analyze cdsObj and allocate+fill in all desired metadata fields */
void GetMetaData(const char* path, struct CdsMediaObject *cdsObj)
{
	int fdType;
	char *title = GetFileName(path, "/", 1);
	char *parentDir  = NULL;
	char *parentTitle = NULL;
	char *parentDir2 = NULL;
	char *parentTitle2 = NULL;

	cdsObj->ProtocolInfo = PROTINFO_VIDEO_MPEG;

	if (strncmp(path, "./Channels", 10) == 0)
	{
		fdType = PCGetFileDirType(path);

		if (fdType == 2)
		{
//			cdsObj->MediaClass = CDS_MEDIACLASS_TUNERCONTAINER;
		}
		else if (fdType == 1)
		{
			/*
			 * <dc:title></dc:title>
			 * <upnp:channelNr>19</upnp:channelNr>
			 * <upnp:callSign></upnp:callSign>
			 * <upnp:networkAffiliation></upnp:networkAffiliation>
			 * <upnp:channelID>19654</upnp:channelID>
			 * <upnp:recordable>1</upnp:recordable>
			 */

			cdsObj->MediaClass = CDS_MEDIACLASS_VIDEOBROADCAST;

			cdsObj->CallSign = malloc(64);
			channel_name(title, cdsObj->CallSign);

			cdsObj->ChannelID = malloc(128);
			sprintf(cdsObj->ChannelID, "%s", title);

			cdsObj->ChannelNr = (char*)malloc(16);
			channel_number(title, cdsObj->ChannelNr);

			cdsObj->DeallocateThese |= CDS_ALLOC_Title;
			cdsObj->Title = (char*)malloc(strlen(cdsObj->ChannelNr) + strlen(cdsObj->CallSign) + 6);
			sprintf(cdsObj->Title, "%s - %s", cdsObj->ChannelNr, cdsObj->CallSign);

			cdsObj->ProgramID = NULL;
			char* descr_ptr = NULL;

			if (get_epg_data_simple(title, &cdsObj->ProgramID, &cdsObj->LongDescription, &descr_ptr, &cdsObj->ScheduledStartTime, &cdsObj->ScheduledDurationTime, &cdsObj->ScheduledEndTime) == 0)
			{
				free(descr_ptr);
			}

			cdsObj->Recordable = 1;

			cdsObj->uri_target = (char*)malloc(255);
			snprintf(cdsObj->uri_target, 255, ":%d/tune=%s&stream/video.mpg", 62080, cdsObj->ChannelID);
		}
	}
	else if (strncmp(path, "./EPG", 5) == 0)
	{
		fdType = PCGetFileDirType(path);
		parentDir  = GetParentPath(path, "/", 1);
		parentTitle = (parentDir != NULL) ? GetFileName(parentDir, "/", 1) : NULL;
		parentDir2 = (parentDir != NULL) ? GetParentPath(parentDir, "/", 1) : NULL;
		parentTitle2 = (parentDir2 != NULL) ? GetFileName(parentDir2, "/", 1) : NULL;

		if (fdType == 2)
		{
			if (strcmp(path, "./EPG") == 0)
			{
				/*
				 * <upnp:epgProviderName></upnp:epgProviderName>
				 * <upnp:serviceProvider></upnp:serviceProvider>
				 */
			}
			else if (parentDir != NULL && strcmp(parentDir, "./EPG/") == 0) /*  /EPG/ch  */
			{
				/*
				 * <dc:title>channel callsign</dc:title>
				 * <upnp:channelID>19654</upnp:channelID>
				 */
				cdsObj->ChannelID = malloc(128);
				sprintf(cdsObj->ChannelID, "%s", title);

				cdsObj->CallSign = malloc(64);
				channel_name(title, cdsObj->CallSign);

				cdsObj->ChannelNr = malloc(16);
				channel_number(title, cdsObj->ChannelNr);

				cdsObj->DeallocateThese |= CDS_ALLOC_Title;
				cdsObj->Title = (char*)malloc(strlen(cdsObj->ChannelNr) +
				                              strlen(cdsObj->CallSign) + 12);

				sprintf(cdsObj->Title, "%s - %s", cdsObj->ChannelNr, cdsObj->CallSign);
			}
			else if (parentDir2 != NULL && strcmp(parentDir2,"./EPG/") == 0) /*  /EPG/ch/day  */
			{
				/*
				 * <dc:title>channel # callsign : date</dc:title>
				 * <upnp:channelID>19654</upnp:channelID>
				 * <upnp:dateTimeRange>2015-11-12T00:00:00Z/2015-11-13T00:00:00Z</upnp:dateTimeRange>
				 */
				cdsObj->ChannelID = malloc(128);
				sprintf(cdsObj->ChannelID, "%s", parentTitle);

				cdsObj->CallSign = malloc(64);
				channel_name(parentTitle, cdsObj->CallSign);

				cdsObj->ChannelNr = malloc(16);
				channel_number(parentTitle, cdsObj->ChannelNr);

				cdsObj->DeallocateThese |= CDS_ALLOC_Title;
				cdsObj->Title = (char*)malloc(strlen(cdsObj->ChannelNr) +
				                              strlen(cdsObj->CallSign) +
											  strlen(title) + 12);
				sprintf(cdsObj->Title, "%s - %s : %s", cdsObj->ChannelNr, cdsObj->CallSign, title);

				struct tm tm = { 0 };
				strptime(title, "%m-%d-%Y", &tm);
				tm.tm_hour = 0;
				tm.tm_min = 0;
				tm.tm_sec = 0;
				time_t t_time = mktime(&tm);
				cdsObj->DateTimeRange.start = t_time;
				cdsObj->DateTimeRange.duration = (24 * 60 * 60);
			}
			cdsObj->MediaClass = CDS_MEDIACLASS_EPGCONTAINER;
		}
		else
		{
			/*
			 * id="EPG/Airings/15242959"
			 *
			 * <dc:title></dc:title>
			 * <upnp:channelID>19654</upnp:channelID>
			 * <upnp:scheduledStartTime usage="SCHEDULED_PROGRAM">2015-11-12T00:00:00Z</upnp:scheduledStartTime>
			 * <upnp:scheduledEndTime>2015-11-12T00:30:00Z</upnp:scheduledEndTime>
			 * <upnp:scheduledDurationTime>P0:30:00</upnp:scheduledDurationTime>
			 * <upnp:longDescription></upnp:longDescription>
			 * <upnp:programID type="xxx.COM"></upnp:programID>
			 * <upnp:actor></upnp:actor>
			 * <upnp:episodeNumber></upnp:episodeNumber>
			 * <upnp:episodeSeason></upnp:episodeSeason>
			 * <upnp:episodeType>FIRST-RUN</upnp:episodeType>
			 * <upnp:seriesID></upnp:seriesID>
			 * <dc:date>2015-11-11T18:00:00Z</dc:date>
			 * <upnp:genre extended="Talk,Interview">Talk</upnp:genre>
			 * <dc:language>English</dc:language>
			 * <upnp:rating type="TVGUIDELINES.ORG">TVG</upnp:rating>
			 */
			cdsObj->MediaClass = CDS_MEDIACLASS_EPG_VIDEO;
			cdsObj->ChannelID = malloc(128);
			sprintf(cdsObj->ChannelID, "%s", parentTitle2);

			char* old_title = cdsObj->Title;
			cdsObj->Title = NULL;

			/* description missing */
			if (get_epg_data_simple(parentTitle2, &title, &cdsObj->Title, &cdsObj->LongDescription, &cdsObj->ScheduledStartTime, &cdsObj->ScheduledDurationTime, &cdsObj->ScheduledEndTime) != 0)
			{
				cdsObj->Title = old_title;
			}

			cdsObj->uri_target = (char*)malloc(255);
			snprintf(cdsObj->uri_target, 255, ":%d/tune=%s&stream/video.mpg", 62080, cdsObj->ChannelID);
		}
	}
	if (title != NULL) free(title);
	if (parentDir != NULL) free(parentDir);
	if (parentTitle != NULL) free(parentTitle);
	if (parentDir2 != NULL) free(parentDir2);
	if (parentTitle2 != NULL) free(parentTitle2);
}

void* PCGetDirFirstFile(const char* directory, char* filename, int filenamelength, uint64_t* filesize)
{
//	printf("%s(%s, filename (out), %d, %" PRIu64 ")\n", __func__, directory, filenamelength, filesize != NULL ? *filesize : 0);
#ifdef WIN32
	WIN32_FIND_DATA FileData;
	HANDLE* hSearch;
	char* direx;

	hSearch = malloc(sizeof(HANDLE));
	direx = malloc(filenamelength + 5);

	if (directory[(int) strlen(directory) - 1] == '\\')
	{
		sprintf(direx,"%s*.*",directory);
	}
	else
	{
		sprintf(direx,"%s\\*.*",directory);
	}

	*hSearch = FindFirstFile(direx, &FileData);
	free(direx);

	if (*hSearch == INVALID_HANDLE_VALUE)
	{
		free(hSearch);
		hSearch = NULL;
	}
	else
	{
		if (filename != NULL)
		{
			strToUtf8(filename,FileData.cFileName, filenamelength, 0, NULL);
		}

		if (filesize != NULL)
		{
			*filesize = FileData.nFileSizeLow;
		}
	}

	return hSearch;
#endif


#ifdef UNDER_CE
	WIN32_FIND_DATA FileData;
	HANDLE* hSearch;			/* must MMS_FREE */
	wchar_t* wdirectory;		/* must MMS_FREE */
	int wDirLen;
	int wDirSize;
	int mbDirLen;
	int mbDirSize;
	char* direx;				/* must MMS_FREE */

	hSearch = malloc(sizeof(HANDLE));
	direx = malloc(filenamelength + 5);

	if (directory[(int) strlen(directory) - 1] == '\\')
	{
		sprintf(direx,"%s*.*",directory);
	}
	else
	{
		sprintf(direx,"%s\\*.*",directory);
	}

	mbDirLen = (int) strlen(direx);
	mbDirSize = mbDirLen+1;
	wDirLen = mbDirLen * 2;
	wDirSize = mbDirSize * 2;
	wdirectory = (wchar_t*)malloc(wDirSize);

	if (mbstowcs(wdirectory,direx,wDirSize) == -1)
	{
		free(hSearch);
		hSearch = NULL;
	}
	else
	{
		*hSearch = FindFirstFile(wdirectory, &FileData);
		if (*hSearch == INVALID_HANDLE_VALUE)
		{
			free(hSearch);
			hSearch = NULL;
		}
		else
		{
			if (filename != NULL)
			{
				if (strToUtf8(filename,(char*)FileData.cFileName,filenamelength, 1, NULL) == -1)
				{
					FindClose(*hSearch);
					free(hSearch);
					hSearch = NULL;
				}
				else
				{
					if (filesize != NULL)
					{
						*filesize = FileData.nFileSizeLow;
					}
				}
			}
		}
	}

	free(direx);
	free(wdirectory);

	return hSearch;
#endif

#ifdef _POSIX
	DIR* dirObj;
	struct dirent* dirEntry;	/* dirEntry is a pointer to static memory in the C runtime lib for readdir()*/
	struct stat64 _si;
	char fullPath[1024];
	char *title      = NULL;
	char *parentDir  = NULL;
	char *parentTitle = NULL;
	char *parentDir2 = NULL;
	void* retval = NULL;

#if NDi_LiveTV
	if (strcmp(directory, "./") == 0)
	{
		/* TODO: do better */
		rootIter = 0;
		if (filename != NULL) sprintf(filename, "%s", rootDirs[rootIter++]);
		if (filesize != NULL) *filesize = 0;
//		printf("%s(%s, filename (%s), %d, %" PRIu64 ")\n", __func__, directory, filename == NULL ? "" : filename, filenamelength, filesize != NULL ? *filesize : 0);
		return &rootIter;
	}
	else if (strncmp(directory, "./Channels", 10) == 0)
	{
		struct dvb_channel* tmpC = firstchannel();
		if (tmpC != NULL) {
			printf("%s/%s.ts\n",directory,tmpC->channelID);
			if (filename != NULL) sprintf(filename, "%s", tmpC->channelID);
			if (filename != NULL && filesize != NULL) {
				*filesize = LIVETV_FILESIZE;
			}
		}
		return (void*)tmpC;
	}
	else if (strncmp(directory, "./EPG", 5) == 0)
	{
		title      = GetFileName(directory, "/", 1);
		parentDir  = GetParentPath(directory, "/", 1);
		parentTitle = GetFileName(parentDir, "/", 1);
		parentDir2 = (parentDir != NULL) ? GetParentPath(parentDir, "/", 1) : NULL;

		if (strcmp(directory, "./EPG/") == 0 || strcmp(directory, "./EPG/") == 0)
		{
			char channelName[64] = { 0 };
			struct dvb_channel* tmpC = firstchannel();
			if (tmpC != NULL) {
				printf("%s/%s\n",directory,tmpC->channelID);
				if (filename != NULL) sprintf(filename, "%s", tmpC->channelID);
				if (filename != NULL && filesize != NULL) {
					*filesize = LIVETV_FILESIZE;
				}
			}
			retval = (void*)tmpC;
		}
		else if (parentDir != NULL && strcmp(parentDir,"./EPG/") == 0 && ischannel(title))
		{
			char day_string[64] = { 0 };
			void *x = firstEpgDay(title, day_string);

			if (filename != NULL) sprintf(filename, "%s", day_string);
			if (filename != NULL && filesize != NULL) {
				*filesize = 0;
			}
			retval = (void*)x;
		}
		else if (parentDir2 != NULL && strcmp(parentDir2,"./EPG/") == 0 &&
		         ischannel(parentTitle) && isDate(title))
		{
			char event_string[64] = { 0 };
			void* x = firstEpgEvent(parentTitle, title, event_string);

			if (filename != NULL) sprintf(filename, "%s", event_string);
			if (filename != NULL && filesize != NULL) {
				*filesize = LIVETV_FILESIZE;
			}
			retval = (void*)x;
		}
		if (title != NULL) free(title);
		if (parentDir != NULL) free(parentDir);
		if (parentTitle != NULL) free(parentTitle);
		if (parentDir2 != NULL) free(parentDir2);
		return retval;
	}
	else
		return NULL;
#endif	
	dirObj = opendir(directory);

	if (dirObj != NULL)
	{
		dirEntry = readdir(dirObj);

		if ((dirEntry != NULL) && ((int) strlen(dirEntry->d_name) < filenamelength))
		{
			if (filename != NULL)
			{
				strToUtf8(filename, dirEntry->d_name, filenamelength, 0, NULL);
				sprintf(fullPath, "%s%s", directory, dirEntry->d_name);

				if (filesize != NULL)
				{
					if (stat64(fullPath, &_si) != -1)
					{
						if ((_si.st_mode & S_IFDIR) == S_IFDIR)
						{
							*filesize = 0;
						}
						else
						{
							*filesize = _si.st_size;
						}
					}
				}
			}
		}
	}

	return dirObj;
#endif
}


// Windows Version
// 0 = No More Files
// 1 = Next File
int PCGetDirNextFile(void* handle, const char* dirName, char* filename, int filenamelength, uint64_t* filesize)
{
	printf("%s(void*, %s, filename (out), %d, %" PRIu64 ")\n", __func__, dirName, filenamelength, filesize != NULL ? *filesize : 0);
#ifdef WIN32
	WIN32_FIND_DATA FileData;
	
	if (FindNextFile(*((HANDLE*)handle), &FileData) == 0) {return 0;}
	strToUtf8(filename,FileData.cFileName,filenamelength,0, NULL);

	if (filesize != NULL)
	{
		*filesize = FileData.nFileSizeLow;
	}

	return 1;
#endif

#ifdef UNDER_CE
    WIN32_FIND_DATA FileData;
    int fnf = 0;
    int conv = -1;
    
    fnf = FindNextFile(*((HANDLE*)handle), &FileData);
    if (fnf == 0) {return 0;}

    conv = strToUtf8(filename, (char*)FileData.cFileName, filenamelength, 1, NULL);
    if (conv == -1) {return 0;}

	if (filesize != NULL)
	{
		*filesize = FileData.nFileSizeLow;
	}
    return 1;
#endif

#ifdef _POSIX
	DIR* dirObj;
	struct dirent* dirEntry;	/* dirEntry is a pointer to static memory in the C runtime lib for readdir()*/
	struct stat64 _si;
	char fullPath[1024];
	int retval = 0;
	char *title      = NULL;
	char *parentDir  = NULL;
	char *parentTitle = NULL;
	char *parentDir2 = NULL;
	char *parentTitle2 = NULL;

#if NDi_LiveTV
	if (strcmp(dirName, "./") == 0)
	{
		if (rootIter == 3) return 0;
		if (filename != NULL) sprintf(filename, "%s", rootDirs[rootIter++]);
		if (filesize != NULL) *filesize = 0;
		retval = 1;
	}
	else if (strncmp(dirName, "./Channels/", 11) == 0)
	{
		struct dvb_channel* tmpC = nextchannel();
		if (tmpC != NULL) {
			if (filename != NULL) sprintf(filename, "%s", tmpC->channelID);

			if (filesize != NULL) {
				*filesize = LIVETV_FILESIZE;
			}
			retval = 1;
		}
	}
	else if (strncmp(dirName, "./EPG/", 6) == 0)
	{
		title       = GetFileName(dirName, "/", 1);
		parentDir   = GetParentPath(dirName, "/", 1);
		parentTitle = GetFileName(parentDir, "/", 1);
		parentDir2  = (parentDir != NULL) ? GetParentPath(parentDir, "/", 1) : NULL;
		parentTitle2 = (parentDir2 != NULL) ? GetFileName(parentDir2, "/", 1) : NULL;

		if (strcmp(dirName, "./EPG/") == 0)
		{
			struct dvb_channel* tmpC = nextchannel();
			if (tmpC != NULL) {
				if (filename != NULL) sprintf(filename, "%s", tmpC->channelID);

				if (filesize != NULL) {
					*filesize = LIVETV_FILESIZE;
				}
				retval = 1;
			}
		}
		else if (parentDir != NULL && strcmp(parentDir,"./EPG/") == 0 && ischannel(title))
		{
			char day_string[128] = { 0 };
			if (nextEpgDay(handle, title, day_string) != NULL)
			{
				if (filename != NULL) sprintf(filename, "%s", day_string);
				if (filename != NULL && filesize != NULL) {
					*filesize = 0;
				}
				retval = 1;
			}
			else
				retval = 0;
		}
		else if (parentDir2 != NULL && strcmp(parentDir2,"./EPG/") == 0 &&
		         ischannel(parentTitle) && isDate(title))
		{
			char event_string[128] = { 0 };
			if (nextEpgEvent(handle, parentTitle, title, event_string) != NULL)
			{
				if (filename != NULL) sprintf(filename, "%s", event_string);
				if (filename != NULL && filesize != NULL) {
					*filesize = LIVETV_FILESIZE;
				}
				retval = 1;
			}
			else
				retval = 0;
		}

	}

	if (title != NULL) free(title);
	if (parentDir != NULL) free(parentDir);
	if (parentTitle != NULL) free(parentTitle);
	if (parentDir2 != NULL) free(parentDir2);
	if (parentTitle2 != NULL) free(parentTitle2);

	return retval;
#endif
	dirObj = (DIR*) handle;
	dirEntry = readdir(dirObj);

	if ((dirEntry != NULL) && ((int) strlen(dirEntry->d_name) < filenamelength))
	{
		strToUtf8(filename, dirEntry->d_name, filenamelength, 0, NULL);
		sprintf(fullPath, "%s%s", dirName, dirEntry->d_name);

		if (filesize != NULL)
		{
			/* WTF? Cygwin has a memory leak with stat. */
			if (stat64(fullPath, &_si) != -1)
			{
				if ((_si.st_mode & S_IFDIR) == S_IFDIR)
				{
					*filesize = 0;
				}
				else
				{
					*filesize = _si.st_size;
				}
			}
		}

		return 1;
	}

	return 0;
#endif
}

/* returns
 * 0 - does not exist
 * 1 - 'file'
 * 2 - directory
 */
int PCGetFileDirType(char* directory)
{
#ifdef WIN32
	DWORD _si;
	int dirLen,dirSize;
	char *fullpath;

	dirLen = (int) strlen(directory);
	dirSize = dirLen+1;
	fullpath = (char*) malloc(dirSize);
	Utf8ToAnsi(fullpath, directory, dirSize);

	_si = GetFileAttributes(fullpath);
	
	free(fullpath);
	
	if (_si == 0xFFFFFFFF)
	{
		return 0;
	}

	if ((_si & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		return 1;
	}
	else 
	{
		return 2;
	}
#endif

#ifdef UNDER_CE
	wchar_t* wfullPath;
	DWORD _si;
	int mbDirSize;
	int wPathSize;
	int dirLen,dirSize;
	char *fullpath;
	int retVal = 0;

	dirLen = (int) strlen(directory);
	dirSize = dirLen+1;
	fullpath = (char*) malloc(dirSize);
	Utf8ToAnsi(fullpath, directory, dirSize);

	mbDirSize = (int) strlen(fullpath) + 1;
	wPathSize = mbDirSize * 2;

	wfullPath = (wchar_t*)malloc(wPathSize);
	if (mbstowcs(wfullPath,fullpath,wPathSize) == -1)
	{
		retVal = 0;
	}
	else
	{
		_si = GetFileAttributes(wfullPath);
		if (_si == 0xFFFFFFFF)
		{
			retVal = 0;
		}
		else
		{
			if ((_si & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				retVal = 1;
			}
			else 
			{
				retVal = 2;
			}
		}
	}

	free(fullpath);
	free(wfullPath);

	return retVal;
#endif

#ifdef _POSIX
	int retval = 0;
//	printf("%s(%s)\n", __func__, directory);
#if NDi_LiveTV
	if (strcmp(directory, "./") == 0)
		return 2;

	char *title      = GetFileName(directory, "/", 1);
	char *parentDir  = GetParentPath(directory, "/", 1);
	char *parentTitle = GetFileName(parentDir, "/", 1);
	char *parentDir2 = (parentDir != NULL) ? GetParentPath(parentDir, "/", 1) : NULL;
	char *parentTitle2 = (parentDir2 != NULL) ? GetFileName(parentDir2, "/", 1) : NULL;
/*
	printf("%s() title = %s\n", __func__, title = NULL ? "" : title);
	printf("%s() parentDir = %s\n", __func__, parentDir == NULL ? "" : parentDir);
	printf("%s() parentTitle = %s\n", __func__, parentTitle == NULL ? "" : parentTitle);
	printf("%s() parentDir2 = %s\n", __func__, parentDir2 == NULL ? "" : parentDir2);
	printf("%s() parentTitle2 = %s\n", __func__, parentTitle2 == NULL ? "" : parentTitle2);
*/
	if (strncmp(directory, "./Channels", 10) == 0)
	{
		if ( (strcmp(directory, "./Channels") == 0) ||
		    (strcmp(directory, "./Channels/") == 0) )
		{
			retval = 2;
		}
		if (ischannel(title)) {
//			printf("its a channel\n");
			retval = 1;
		}
	}
	else if (strncmp(directory, "./EPG", 5) == 0)
	{
		if ( (strcmp(directory, "./EPG") == 0) ||
		    (strcmp(directory, "./EPG/") == 0) )
		{
			retval = 2;
		}
		else if (strcmp(parentDir, "./EPG/") == 0)
		{
			if (ischannel(title)) {
//				printf("Channel Aggregation EPG\n");
				retval = 2;
			}
		}
		else if (parentTitle2 != NULL && ischannel(parentTitle2) &&
		         parentTitle != NULL && isDate(parentTitle))
		{
			if (1) // TODO: isListing(title)
			{
//				printf("Individual EPG listing\n");
				retval = 1;
			}
		}
		else if (parentDir2 != NULL && strcmp(parentDir2, "./EPG/") == 0 &&
		         ischannel(parentTitle) && isDate(title))
		{
//			printf("Channel Day of the Week EPG Listing\n");
			retval = 2;
		}
	}

	if (title != NULL) free(title);
	if (parentDir != NULL) free(parentDir);
	if (parentTitle != NULL) free(parentTitle);
	if (parentDir2 != NULL) free(parentDir2);
	if (parentTitle2 != NULL) free(parentTitle2);

	return retval;

#endif
	struct stat64 _si;

	int dirLen,dirSize;
	char *fullpath;
	int pathExists;
	int retVal = 0;

	dirLen = (int) strlen(directory);
	dirSize = dirLen+1;
	fullpath = (char*) malloc(dirSize);
	Utf8ToAnsi(fullpath, directory, dirSize);

	pathExists = stat64(fullpath, &_si);

	free(fullpath);

	if (pathExists != -1)
	{
		if ((_si.st_mode & S_IFDIR) == S_IFDIR)
		{
			retVal = 2;
		}
		else
		{
			retVal = 1;
		}
	}

	return retVal;
#endif
}

#ifdef _POSIX
/* only needed for posix because readdir returns statically allocated values */
pthread_mutex_t BrowseLock;
#endif

void InitBrowseLock()
{
#ifdef _POSIX
	pthread_mutex_init(&BrowseLock, NULL);
#endif
}

void LockBrowse()
{
#ifdef _POSIX
	pthread_mutex_lock(&BrowseLock);
#endif
}

void UnlockBrowse()
{
#ifdef _POSIX
	pthread_mutex_unlock(&BrowseLock);
#endif
}

void DestroyBrowseLock()
{
#ifdef _POSIX
	pthread_mutex_destroy(&BrowseLock);
#endif
}

void EndThisThread()
{
#ifdef _POSIX
	pthread_exit(NULL);
#endif

#ifdef WIN32
	ExitThread(0);
#endif

#ifdef UNDER_CE
	ExitThread(0);
#endif
}

void* SpawnNormalThread(void* method, void* arg)
{
#ifdef _POSIX
	int result;
	void* (*fptr) (void* a);
	pthread_t newThread;
	fptr = method;
	result = pthread_create(&newThread, NULL, fptr, arg);
	pthread_detach(newThread);
	return (void*) result;
#endif

#ifdef WIN32
	return CreateThread(NULL, 0, method, arg, 0, NULL );
#endif

#ifdef UNDER_CE
	return CreateThread(NULL, 0, method, arg, 0, NULL );
#endif
}

uint64_t PCGetFileSize(const char* fullPath)
{
//	printf("%s\n", __func__);
	uint64_t filesize = -1;

#ifdef _POSIX
#if NDi_LiveTV
	if (strcmp(fullPath, "./") == 0) return filesize;
	return LIVETV_FILESIZE;
#endif
	struct stat64 _si;

	int pathLen,pathSize;
	char *fp;

	pathLen = (int) strlen(fullPath);
	pathSize = pathLen+1;
	fp = (char*) malloc(pathSize);
	Utf8ToAnsi(fp, fullPath, pathSize);

	if (stat64(fp, &_si) != -1)
	{
		if (!((_si.st_mode & S_IFDIR) == S_IFDIR))
		{
			filesize = _si.st_size;
		}
	}

	free(fp);
#endif

#ifdef WIN32
	WIN32_FIND_DATA FileData;
	HANDLE* hSearch;			/* must MMS_FREE */
	int pathLen,pathSize;
	char *fp;

	pathLen = (int) strlen(fullPath);
	pathSize = pathLen+1;
	fp = (char*) malloc(pathSize);
	Utf8ToAnsi(fp, fullPath, pathSize);

	hSearch = malloc(sizeof(HANDLE));

	*hSearch = FindFirstFile(fp, &FileData);
	free(fp);

	if (*hSearch == INVALID_HANDLE_VALUE)
	{
		filesize = 0;
	}
	else
	{
		filesize = FileData.nFileSizeLow;
	}

	FindClose(*hSearch);
	free(hSearch);
#endif

#ifdef UNDER_CE
	WIN32_FIND_DATA FileData;
	HANDLE* hSearch;			/* must MMS_FREE */
	wchar_t* wdirectory;		/* must MMS_FREE */
	int wPathLen;
	int wPathSize;
	int fullPathLen;
	int fullPathSize;
	char* fp;

	fullPathLen = (int) strlen(fullPath);
	fullPathSize = fullPathLen + 1;
	fp = (char*) malloc(fullPathSize);
	Utf8ToAnsi(fp, fullPath, fullPathSize);

	hSearch = malloc(sizeof(HANDLE));

	wPathLen = fullPathLen * 2;
	wPathSize = fullPathSize * 2;
	wdirectory = (wchar_t*)malloc(wPathSize);

	if (mbstowcs(wdirectory,fp,wPathSize) == -1)
	{
		filesize = -1;
	}
	else
	{
		*hSearch = FindFirstFile(wdirectory, &FileData);
		if (*hSearch == INVALID_HANDLE_VALUE)
		{
			filesize = -1;
		}
		else
		{
			FindClose(*hSearch);
			filesize = FileData.nFileSizeLow;
		}
	}

	free(fp);
	free(wdirectory);
	free(hSearch);
#endif

	return filesize;
}

int PCGetGetDirEntryCount(const char* fullPath, char *dirDelimiter)
{
//	printf("%s( %s )\n", __func__, fullPath);
	char fn[MAX_PATH_LENGTH];
	void *dirObj;
	int retVal = 0;
	int ewDD;
	int nextFile;
	char* rFullPath = (char*)fullPath;

	dirObj = PCGetDirFirstFile(fullPath, fn, MAX_PATH_LENGTH, NULL);

	if (dirObj != NULL)
	{
		ewDD = EndsWith(rFullPath, dirDelimiter, 0);
		if (ewDD != 1) {
			rFullPath = malloc(strlen(fullPath) + 2);
			sprintf(rFullPath, "%s%s", fullPath, dirDelimiter);
		}

		do
		{
			if (ProceedWithDirEntry(rFullPath, fn, MAX_PATH_LENGTH) != 0)
			{
				retVal++;
			}

			nextFile = PCGetDirNextFile(dirObj,rFullPath,fn,MAX_PATH_LENGTH, NULL);
		}
		while (nextFile != 0);

		PCCloseDir(dirObj);
	}

//	printf("%s( %s ) entries : %d\n", __func__, fullPath, retVal);
	if (rFullPath != fullPath) {
		free(rFullPath);
	}
	return retVal;
}

/*
	 returns 0 if directory entry should not be processed.
	 returns nonzero if directory entry should be processed
*/
int ProceedWithDirEntry(const char* dirName, const char* filename, int maxPathLength)
{
//	printf("%s(%s, %s, %d)\n", __func__, dirName, filename, maxPathLength);
	int dirLen;
	int fnLen;
	int val;

	char *fullpath;

	dirLen = (int) strlen(dirName);
	fnLen = (int) strlen(filename);

	if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0))
	{
		/* NOP */
		return 0;
	}
	
	if ((dirLen+fnLen+2) > maxPathLength)
	{
		/* directory is too long */
		return 0;
	}

	if (fnLen > 0 && filename[0] == '.') {
		/* prevent hidden files from showing up */
		return 0;
	}

	fullpath = (char*) malloc(maxPathLength);
	memcpy(fullpath, dirName, dirLen);
	memcpy(fullpath+dirLen, filename, fnLen);
	fullpath[ dirLen+fnLen ] = '\0';
	val = PCGetFileDirType(fullpath);
	free(fullpath);
	if (val == 0)
	{
		return 0;
	}

		/*
		#ifdef UNDER_CE
		#define CF_CARD "CF Card"
		#define SD_CARD "SD Card"
		#define MMC_CARD "MMC Card"
		#define BUILT_IN_STORAGE "Built-in Storage"
		#define STORAGE "Storage"

				if (strcmp(dirName, "\\") == 0)
				{
					// only reveal directories that are storage cards from the root

					if (StartsWith(filename, CF_CARD, 1) != 0)
					{
						return 1;
					}
					if (StartsWith(filename, SD_CARD, 1) != 0)
					{
						return 1;
					}
					if (StartsWith(filename, MMC_CARD, 1) != 0)
					{
						return 1;
					}
					if (StartsWith(filename, BUILT_IN_STORAGE, 1) != 0)
					{
						return 1;
					}
					if (StartsWith(filename, STORAGE, 1) != 0)
					{
						return 1;
					}

					return 0;
				}
		#endif
		*/

		return 1;
}

unsigned int PCGetSystemUpdateID()
{
	/*
	 *	TODO: Return a number indicating the state of the metadata store.
	 *	Whenever the metadata in the CDS changes, this value should
	 *	increase monotomically.
	 *
	 *	For file systems, the most reliable method to obtain this value would likely
	 *	be the most recent file system date.
	 */

	return 0;
}

unsigned int PCGetContainerUpdateID(const char* path)
{
	/*
	 *	TODO: Return a number indicating the state of the directory.
	 *	Whenever the entries in the directory change for any reason, this value should
	 *	increase monotomically for that directory. Each directory should have
	 *	its own UpdateID.
	 *
	 *	For file systems, the most reliable to obtain this value would likely
	 *	be the most recent file system date of a file/dir in the provided directory.
	 */

	return 0;
}

/*
 * return val -1 == file open failure
 * return val -2 == file reading fail
 * return val -3 == malloc error
 */
int file_get_contents(const char *filename, char **result)
{
	int size = 0;
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
	{
		*result = NULL;
		return -1;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if ((*result = (char *)malloc(size+1)) == NULL) return -3;
	if (size != fread(*result, sizeof(char), size, f))
	{
		free(*result);
		return -2;
	}
	fclose(f);
	(*result)[size] = 0;
	return size;
}

