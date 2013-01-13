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

#define _CRTDBG_MAP_ALLOC
#include <string.h>
#include "MimeTypes.h"
#include "MyString.h"
#include "ILibParsers.h"

#ifdef WIN32
#include <stdlib.h>
#include <crtdbg.h>
#endif

#ifndef UNDER_CE
#include <wchar.h>
#endif

#ifdef UNDER_CE
#define stricmp _stricmp
#define strcmpi _stricmp
#endif

#ifdef _POSIX
#define stricmp strcasecmp
#undef strcmpi
#define strcmpi strcasecmp
#endif

#define _TMP_CHAR_BUFFER 10

char* FileExtensionToMimeType (char* extension, int wide)
{
	char tempExtension[_TMP_CHAR_BUFFER], temp2[_TMP_CHAR_BUFFER];
	char *retVal = NULL;

	if (wide)
	{
		strToUtf8(temp2, extension, _TMP_CHAR_BUFFER, 1, NULL);
		extension = temp2;
	}
	
	if (extension[0] != '.')
	{
		tempExtension[0] = '.';
		strcpy(tempExtension+1, extension);
		extension = tempExtension;
	}

	if (stricmp(extension, EXTENSION_AUDIO_MPEG) == 0)
	{
		retVal = MIME_TYPE_AUDIO_MPEG;
	}
	else if (stricmp(extension, EXTENSION_AUDIO_WMA) == 0)
	{
		retVal = MIME_TYPE_AUDIO_WMA;
	}
	else if (stricmp(extension, EXTENSION_AUDIO_OGG) == 0)
	{
		retVal = MIME_TYPE_AUDIO_OGG;
	}
	else if (stricmp(extension, EXTENSION_PLAYLIST_ASX) == 0)
	{
		retVal = MIME_TYPE_PLAYLIST_ASX;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_ASF) == 0)
	{
		retVal = MIME_TYPE_VIDEO_ASF;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_WMV) == 0)
	{
		retVal = MIME_TYPE_VIDEO_WMV;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_MPEG) == 0)
	{
		retVal = MIME_TYPE_VIDEO_MPEG;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_MOV) == 0)
	{
		retVal = MIME_TYPE_VIDEO_MOV;
	}
	else if (stricmp(extension, EXTENSION_IMAGE_JPG) == 0)
	{
		retVal = MIME_TYPE_IMAGE_JPG;
	}	
	else if (stricmp(extension, EXTENSION_IMAGE_JPEG) == 0)
	{
		retVal = MIME_TYPE_IMAGE_JPEG;
	}	
	else if (stricmp(extension, EXTENSION_IMAGE_BMP) == 0)
	{
		retVal = MIME_TYPE_IMAGE_BMP;
	}	
	else if (strcmpi(extension, EXTENSION_PLAYLIST_M3U) == 0)
	{
		retVal = MIME_TYPE_PLAYLIST_M3U;
	}
	else
	{
		retVal = "application/octet-stream";
	}

	return retVal;
}

char* FileExtensionToProtocolInfo (char* extension, int wide)
{
	char tempExtension[_TMP_CHAR_BUFFER], temp2[_TMP_CHAR_BUFFER];
	char *retVal = NULL;

	if (wide)
	{
		strToUtf8(temp2, extension, _TMP_CHAR_BUFFER, 1, NULL);
		extension = temp2;
	}
	
	if (extension[0] != '.')
	{
		tempExtension[0] = '.';
		strcpy(tempExtension+1, extension);
		extension = tempExtension;
	}

	if (stricmp(extension, EXTENSION_AUDIO_MPEG) == 0)
	{
		retVal = PROTINFO_AUDIO_MPEG;
	}
	else if (stricmp(extension, EXTENSION_AUDIO_WMA) == 0)
	{
		retVal = PROTINFO_AUDIO_WMA;
	}
	else if (stricmp(extension, EXTENSION_AUDIO_OGG) == 0)
	{
		retVal = PROTINFO_AUDIO_OGG;
	}
	else if (stricmp(extension, EXTENSION_PLAYLIST_ASX) == 0)
	{
		retVal = PROTINFO_PLAYLIST_ASX;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_ASF) == 0)
	{
		retVal = PROTINFO_VIDEO_ASF;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_WMV) == 0)
	{
		retVal = PROTINFO_VIDEO_WMV;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_MPEG) == 0)
	{
		retVal = PROTINFO_VIDEO_MPEG;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_MOV) == 0)
	{
		retVal = PROTINFO_VIDEO_MOV;
	}
	else if (stricmp(extension, EXTENSION_IMAGE_JPG) == 0)
	{
		retVal = PROTINFO_IMAGE_JPG;
	}	
	else if (stricmp(extension, EXTENSION_IMAGE_JPEG) == 0)
	{
		retVal = PROTINFO_IMAGE_JPEG;
	}	
	else if (stricmp(extension, EXTENSION_IMAGE_BMP) == 0)
	{
		retVal = PROTINFO_IMAGE_BMP;
	}	
	else if (strcmpi(extension, EXTENSION_PLAYLIST_M3U) == 0)
	{
		retVal = PROTINFO_PLAYLIST_M3U;
	}
	else
	{
		retVal = "http-get:*:application/octet-stream:*";
	}

	return retVal;
}

char* MimeTypeToFileExtension (char* mime_type)
{
	if (stricmp(mime_type, MIME_TYPE_AUDIO_MPEG) == 0)
	{
		return EXTENSION_AUDIO_MPEG;
	}
	else if (stricmp(mime_type, MIME_TYPE_AUDIO_WMA) == 0)
	{
		return EXTENSION_AUDIO_WMA;
	}
	else if (stricmp(mime_type, MIME_TYPE_AUDIO_OGG) == 0)
	{
		return EXTENSION_AUDIO_OGG;
	}
	else if (stricmp(mime_type, MIME_TYPE_PLAYLIST_ASX) == 0)
	{
		return EXTENSION_PLAYLIST_ASX;
	}
	else if (stricmp(mime_type, MIME_TYPE_VIDEO_ASF) == 0)
	{
		return EXTENSION_VIDEO_ASF;
	}
	else if (stricmp(mime_type, MIME_TYPE_VIDEO_WMV) == 0)
	{
		return EXTENSION_VIDEO_WMV;
	}
	else if (stricmp(mime_type, MIME_TYPE_VIDEO_MPEG) == 0)
	{
		return EXTENSION_VIDEO_MPEG;
	}
	else if (stricmp(mime_type, MIME_TYPE_VIDEO_MOV) == 0)
	{
		return EXTENSION_VIDEO_MOV;
	}
	else if (stricmp(mime_type, MIME_TYPE_IMAGE_JPG) == 0)
	{
		return EXTENSION_IMAGE_JPG;
	}
	else if (stricmp(mime_type, MIME_TYPE_IMAGE_JPEG) == 0)
	{
		return EXTENSION_IMAGE_JPEG;
	}
	else if (stricmp(mime_type, MIME_TYPE_IMAGE_BMP) == 0)
	{
		return EXTENSION_IMAGE_BMP;
	}
	else if (strcmpi(mime_type, MIME_TYPE_PLAYLIST_M3U) == 0)
	{
		return EXTENSION_PLAYLIST_M3U;
	}


	return ".bin";
}

char* FileExtensionToUpnpClass (char* extension, int wide)
{
	char tempExtension[_TMP_CHAR_BUFFER], temp2[_TMP_CHAR_BUFFER];
	char *retVal = NULL;

	if (wide)
	{
		strToUtf8(temp2, extension, _TMP_CHAR_BUFFER, 1, NULL);
		extension = temp2;
	}
	
	if (extension[0] != '.')
	{
		tempExtension[0] = '.';
		strcpy(tempExtension+1, extension);
		extension = tempExtension;
	}

	if (stricmp(extension, EXTENSION_AUDIO_MPEG) == 0)
	{
		retVal = CLASS_AUDIO_ITEM;
	}
	else if (stricmp(extension, EXTENSION_AUDIO_WMA) == 0)
	{
		retVal = CLASS_AUDIO_ITEM;
	}
	else if (stricmp(extension, EXTENSION_AUDIO_OGG) == 0)
	{
		retVal = CLASS_AUDIO_ITEM;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_ASF) == 0)
	{
		retVal = CLASS_VIDEO_ITEM;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_WMV) == 0)
	{
		retVal = CLASS_VIDEO_ITEM;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_MPEG) == 0)
	{
		retVal = CLASS_VIDEO_ITEM;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_MOV) == 0)
	{
		retVal = CLASS_VIDEO_ITEM;
	}
	else if (stricmp(extension, EXTENSION_IMAGE_JPG) == 0)
	{
		retVal = CLASS_IMAGE_ITEM;
	}	
	else if (stricmp(extension, EXTENSION_IMAGE_JPEG) == 0)
	{
		retVal = CLASS_IMAGE_ITEM;
	}	
	else if (stricmp(extension, EXTENSION_IMAGE_BMP) == 0)
	{
		retVal = CLASS_IMAGE_ITEM;
	}	
	else if (strcmpi(extension, EXTENSION_PLAYLIST_M3U) == 0)
	{
		retVal = CLASS_PLAYLIST_M3U;
	}
	else if (stricmp(extension, EXTENSION_PLAYLIST_ASX) == 0)
	{
		retVal = CLASS_PLAYLIST_ASX;
	}
	else
	{
		retVal = CLASS_ITEM;
	}

	return retVal;
}

unsigned int FileExtensionToClassCode (char* extension, int wide)
{
	char tempExtension[_TMP_CHAR_BUFFER], temp2[_TMP_CHAR_BUFFER];
	unsigned int retVal;

	if (wide)
	{
		strToUtf8(temp2, extension, _TMP_CHAR_BUFFER, 1, NULL);
		extension = temp2;
	}
	
	if (extension[0] != '.')
	{
		tempExtension[0] = '.';
		strcpy(tempExtension+1, extension);
		extension = tempExtension;
	}

	if (stricmp(extension, EXTENSION_AUDIO_MPEG) == 0)
	{
		retVal = CDS_MEDIACLASS_AUDIOITEM;
	}
	else if (stricmp(extension, EXTENSION_AUDIO_WMA) == 0)
	{
		retVal = CDS_MEDIACLASS_AUDIOITEM;
	}
	else if (stricmp(extension, EXTENSION_AUDIO_OGG) == 0)
	{
		retVal = CDS_MEDIACLASS_AUDIOITEM;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_ASF) == 0)
	{
		retVal = CDS_MEDIACLASS_VIDEOITEM;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_WMV) == 0)
	{
		retVal = CDS_MEDIACLASS_VIDEOITEM;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_MPEG) == 0)
	{
		retVal = CDS_MEDIACLASS_VIDEOITEM;
	}
	else if (stricmp(extension, EXTENSION_VIDEO_MOV) == 0)
	{
		retVal = CDS_MEDIACLASS_VIDEOITEM;
	}
	else if (stricmp(extension, EXTENSION_IMAGE_JPG) == 0)
	{
		retVal = CDS_MEDIACLASS_IMAGEITEM;
	}	
	else if (stricmp(extension, EXTENSION_IMAGE_JPEG) == 0)
	{
		retVal = CDS_MEDIACLASS_IMAGEITEM;
	}	
	else if (stricmp(extension, EXTENSION_IMAGE_BMP) == 0)
	{
		retVal = CDS_MEDIACLASS_IMAGEITEM;
	}	
	else if (strcmpi(extension, EXTENSION_PLAYLIST_M3U) == 0)
	{
		retVal = CDS_MEDIACLASS_PLAYLISTCONTAINER;
	}
	else if (strcmpi(extension, EXTENSION_PLAYLIST_ASX) == 0)
	{
		retVal = CDS_MEDIACLASS_PLAYLISTCONTAINER;
	}
	else
	{
		retVal = CDS_MEDIACLASS_ITEM;
	}

	return retVal;
}
