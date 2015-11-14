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

#ifndef _CDS_OBJECT_TO_DIDL_H
#define _CDS_OBJECT_TO_DIDL_H

/*
 *	This module provides the functionality for translating the structures defined in CdsObject.h
 *	into DIDL-Lite valid XML strings. The implementation has a strong dependency on the fields
 *	described in CdsObject.h.
 *
 *	The primary reason why this file does not declare the CdsMediaObject and CdsMediaResource is
 *	to allow CDS devices and control points to use the same object representations for media objects
 *	and resources.
 *
 *	That being said, CDS control points need a module designed to convert from DIDL-Lite
 *	to the object representations, whilst a device will need a module designed to convert from
 *	the object forms to DIDL-Lite.
 */

#include "CdsMediaObject.h"

/*
 *	All values must be negative.
 */
enum Errors_CdsObjectToDidl
{
	Error_CdsObjectToDidl_InvalidMediaClass		= -1,		/* media class has value of zero */
	Error_CdsObjectToDidl_UndefinedObjectType	= -2,		/* media class has undefined object type */
	Error_CdsObjectToDidl_UndefinedMajorType	= -3,		/* media class has undefined major type */
	Error_CdsObjectToDidl_UndefinedMinor1Type	= -4,		/* media class has undefined minor1 type */
	Error_CdsObjectToDidl_UndefinedMinor2Type	= -5,		/* media class has undefined minor2 type */
	Error_CdsObjectToDidl_MismatchContainerRefID= -6,		/* media class is container, but data has refID */
	Error_CdsObjectToDidl_EmptyTitle			= -7,		/* media objects cannot have an empty title */
	Error_CdsObjectToDidl_EmptyObjectID			= -8,		/* media objects cannot have empty object ID values */
	Error_CdsObjectToDidl_EmptyParentID			= -9,		/* media objects cannot have empty parent ID values. Parents of root containers should be "-1". */
	
	Error_CdsObjectToDidl_CorruptedMemory		= -99		/* LOGIC error - the code did not allocate enough memory */
};

/*
 *	Enumerates the optional fields that should be included in the response.
 *	You can bitwise-OR these values with to set bits in a bitstring
 *	to allow a compact and convenient representation of the 
 */
enum CdsFilterBits
{
	CdsFilter_Creator		= 0x0001,		/* for dc:creator */

	CdsFilter_Res			= 0x00000002,	/* for minimal <res> element: only protocolInfo and value */
	CdsFilter_Resolution	= 0x00000004,	/* infers CdsFilter_Res bit is true. Includes resolution attribute of <res> element. */
	CdsFilter_Duration		= 0x00000008,	/* infers CdsFilter_Res bit is true. Includes duration attribute of <res> element. */
	CdsFilter_Bitrate		= 0x00000010,	/* infers CdsFilter_Res bit is true. Includes bitrate attribute of <res> element. */
	CdsFilter_ColorDepth	= 0x00000020,	/* infers CdsFilter_Res bit is true. Includes colorDepth attribute of <res> element. */
	CdsFilter_Size			= 0x00000040,	/* infers CdsFilter_Res bit is true. Includes size attribute of <res> element. */

	CdsFilter_ChildCount	= 0x00000080,	/* childCount attribute of container */
	CdsFilter_Searchable	= 0x00000100,	/* searchable attribute of container */

	CdsFilter_Album			= 0x00000200,	/* for upnp:album */
	CdsFilter_Genre			= 0x00000400,	/* for upnp:genre */

	CdsFilter_BitsPerSample		= 0x00000800,
	CdsFilter_SampleFrequency	= 0x00001000,
	CdsFilter_nrAudioChannels	= 0x00002000,
	CdsFilter_Protection		= 0x00004000,

	/* Broadcast Item related */
	CdsFilter_ChannelNr			= 0x00010000,
	CdsFilter_ChannelName		= 0x00020000,
	CdsFilter_ChannelID			= 0x00040000,

	/* EPG related */
	CdsFilter_epgProviderName	= 0x00060000,
	CdsFilter_ServiceProvider	= 0x00080000,
	CdsFilter_CallSign			= 0x000A0000,
	CdsFilter_NetworkAffil		= 0x000C0000,
	CdsFilter_Recordable		= 0x000E0000,
	CdsFilter_DateTimeRange		= 0x00100000,
	CdsFilter_SchedStartTime	= 0x00200000,
	CdsFilter_SchedEndTime		= 0x00400000,
	CdsFilter_SchedDurationTime = 0x00600000,
	CdsFilter_ProgramID			= 0x00800000,
	CdsFilter_SeriesID			= 0x00A00000,
	CdsFilter_EpisodeNumber		= 0x00C00000,
	CdsFilter_EpisodeSeason		= 0x00E00000,
	CdsFilter_EpisodeType		= 0x01000000,
	CdsFilter_LongDescription	= 0x02000000,
	CdsFilter_Date				= 0x04000000,
	CdsFilter_Rating			= 0x06000000,
	CdsFilter_Language			= 0x08000000,
	CdsFilter_Actor				= 0x0A000000,
	CdsFilter_AlbumArtURI		= 0x0C000000,

	CdsFilter_ResAllAttribs = 
			CdsFilter_Res | 
			CdsFilter_Resolution | 
			CdsFilter_Duration | 
			CdsFilter_Bitrate | 
			CdsFilter_ColorDepth | 
			CdsFilter_BitsPerSample |
			CdsFilter_SampleFrequency |
			CdsFilter_nrAudioChannels |
			CdsFilter_Protection |
			CdsFilter_Size
};

/*
 *	Returns a bistring where each bit indicates if a supported optional metadata field should be included
 *	in a DIDL-Lite response (to a Browse/Search) request. The implementation of this method matches
 *	the supported metadata described in CdsMediaObject and CdsMediaResource. Should those structures be
 *	changed to accomodate more or less metadata, then this method implementation should change to reflect it.
 *
 *		Returns unsigned int	: the bitstring, where each bit is accessible with 'enum CdsFilterBits'.
 *
 *		filter					: Fomma-delimited string of metadata filters. If null, then the return value is zero.
 *								: If "*", then all bits are set to 1.
 */
unsigned int CdsToDidl_GetFilterBitString(const char *filter);

/*
 *	Returns the DIDL-Lite <item> or <container> element as an XML-escaped string,
 *	such that the unescaped value sufficiently represents the provided mediaObject, 
 *	including its resources. The method also applies metadata filtering.
 *
 *		Returns char*			: the <item> or <container> element that represents the specified media object.
 *
 *		mediaObj				: the media object to serialize into DIDL-Lite XML form
 *
 *		metadataXmlEscaped		: if nonzero, then all of the metadata has been properly XML-escaped already
 *
 *		filter					: bitmask of desired metadata fields to include in the DIDL-Lite.
 *								: Value can be obtained from CdsToDidl_GetFilterBitString().
 *
 *		includeHeaderFooter		: the returned string should be encapsulated in a <DIDL-Lite> element document tag.
 *
 *		outErrorOrDidlLen		: the length of the string, returned through outDidl. 
 *								: If the returned value is null, this value will contain a negative error code
 *								: corresponding from Errors_CdsObjectToDidl.
 */
char* CdsToDidl_GetMediaObjectDidlEscaped (struct CdsMediaObject *mediaObj, int metadataXmlEscaped, unsigned int filter, int includeHeaderFooter, int *outErrorOrDidlLen);

#endif
