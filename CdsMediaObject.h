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

#ifndef _CDS_MEDIA_OBJECT_H
#define _CDS_MEDIA_OBJECT_H

/*
 *	Provides the convenient means of manipulating/reading of CdsMediaObject.MediaClass field.
 */
#include "CdsMediaClass.h"

/*
 *	This module provides minimal data structures to represent
 *	media objects and their resource elements.
 *
 *	Vendors that want to increase the supported metadata elements
 *	should modify these structures appropriately.
 *
 *	This library does not presume use specifically for control point
 *	or device-implementation software.
 */

enum CdsObjectProperties
{
	CDS_OBJPROP_FLAGS_Restricted = 0x0001,		/* restricted attribute of media object */
	CDS_OBJPROP_FLAGS_Searchable = 0x0002		/* container is searchable */
};

enum CdsAllocatable
{
	CDS_ALLOC_ID		= 0x0001,
	CDS_ALLOC_ParentID	= 0x0002,
	CDS_ALLOC_RefID		= 0x0004,
	CDS_ALLOC_Title		= 0x0008,
	CDS_ALLOC_Creator	= 0x0010,
	CDS_ALLOC_Album		= 0x0020,
	CDS_ALLOC_Genre		= 0x0040
};

enum CdsResAllocatable
{
	CDS_ALLOC_ProtInfo	= 0x0001,
	CDS_ALLOC_Protection= 0x0002
};

/*
 *	Minimalistic representation of a resource.
 *
 *	All strings are assumed to be in UTF8 form.
 */
struct CdsMediaResource
{
	unsigned char Allocated;	/*
								 *	Indicates if protocolInfo and protection need to be deallocated. 
								 *	Mapped by CDS_ALLOC_ProtInfo and CDS_ALLOC_Protection.
								 */

	char *Value;				/*
								 *	This is the text that provide's the resource's value.
								 *	Usually, this a URI. URI must be properly URI-escaped
								 *	according to the rules of the URI's scheme. Note, URI-escaping
								 *	is different than XML-escaped. 
								 */
	char *ProtocolInfo;			/* the protocolInfo of the resource */

	int ResolutionX;			/* the horizontal resolution. Negative value means value is not set.*/
	int ResolutionY;			/* the vertical resolution. Negative value means value is not set.*/

	int Duration;				/* the duration, in number of seconds. Negative value means value is not set.*/
	int Bitrate;				/* the bitrate of the resource. If negative, treat as an unset value. */

	int ColorDepth;				/* the color depth of the resource. If negative, treat as an unset value. */
	long Size;					/* the file size of the resource. If negative, treat as an unset value. */

	int BitsPerSample;			/* the number of bits per sample - should always accompany a valid SampleFrequency. Negative if unknown. */
	int SampleFrequency;		/* the sampling frequency - should always accompany BitsPerSample. Negative if unknown. */

	int NrAudioChannels;		/* number of audio channels. Negative indicates unknown */
	char *Protection;			/* app-defined DRM protection string. NULL if not present. */

	struct CdsMediaResource *Next;	/* allows the struct to be used in a linked list */
};

/*
 *	Provides a minimal representation of a media object.
 */
struct CdsMediaObject
{
	/*
	 *	Bit string masked by enum CdsAllocatable.
	 *	If the bit is set, then the corresponding field should not be
	 *	deleted in a call to CDS_DestroyObjects().
	 */
	unsigned int DeallocateThese;

	char *ID;			/* Object ID */
	char *ParentID;		/* Parent object ID */
	char *RefID;		/* Object ID of underlying item: for reference item only*/

	char *Title;		/* Title metadata */
	char *Creator;		/* Creator metadata */

	char *Album;		/* Album metadata */
	char *Genre;		/* Genre metadata */

	/* media class of object. See [CdsMediaObject.h] */
	unsigned int MediaClass;

	/* number of children that this object has - valid only for container objects */
	int ChildCount;

	/*
	 *	Boolean flags for representing object boolean properties. 
	 *	Individual bits can be accessed through (enum CdsObjectProperties).
	 */
	unsigned int Flags;

	struct CdsMediaResource *Res;		/* first resource for the media object */
	struct CdsMediaObject *Next;		/* allows struct to be used in a linked list*/
};

/* Allocates the memory for a CdsMediaObject, with all memory set to zero */
struct CdsMediaObject* CDS_AllocateObject();

struct CdsMediaResource* CDS_AllocateResource();

/* Deallocates all of the memory for the linked list of CdsMediaObject and its associated resources. */
void CDS_DestroyObjects(struct CdsMediaObject *cdsObjList);

/* Deallocates all of the memory for the linked list of CdsResources */
void CDS_DestroyResources(struct CdsMediaResource *resList);

#endif

