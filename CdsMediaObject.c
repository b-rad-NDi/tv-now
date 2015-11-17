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

#include <stdlib.h>
#include <string.h>
#include "CdsMediaObject.h"

#ifdef WIN32
#include <crtdbg.h>
#endif

struct CdsMediaObject* CDS_AllocateObject()
{
	struct CdsMediaObject *cdsObj = (struct CdsMediaObject *) malloc (sizeof(struct CdsMediaObject));
	memset(cdsObj, 0, sizeof(struct CdsMediaObject));
	cdsObj->Recordable = -1;
	return cdsObj;
}

struct CdsMediaResource* CDS_AllocateResource()
{
	struct CdsMediaResource *res = (struct CdsMediaResource *) malloc (sizeof(struct CdsMediaResource));
	res->Allocated = 0;
	res->Next = NULL;
	res->Value = res->Protection = res->ProtocolInfo = NULL;
	res->Size = res->ColorDepth = res->Bitrate = res->Duration = res->ResolutionX = res->ResolutionY = res->BitsPerSample = res->SampleFrequency = res->NrAudioChannels = -1;
	return res;
}

void CDS_DestroyObjects(struct CdsMediaObject *cdsObjList)
{
	struct CdsMediaObject *cdsObj = cdsObjList, *nextCds;

	while (cdsObj != NULL)
	{
		nextCds = cdsObj->Next;

		if ((cdsObj->DeallocateThese & CDS_ALLOC_Creator)	&& (cdsObj->Creator != NULL))	{ free(cdsObj->Creator); }
		if ((cdsObj->DeallocateThese & CDS_ALLOC_ID)		&& (cdsObj->ID != NULL))		{ free(cdsObj->ID); }
		if ((cdsObj->DeallocateThese & CDS_ALLOC_ParentID)	&& (cdsObj->ParentID != NULL))	{ free(cdsObj->ParentID); }
		if ((cdsObj->DeallocateThese & CDS_ALLOC_RefID)		&& (cdsObj->RefID != NULL))		{ free(cdsObj->RefID); }
		if ((cdsObj->DeallocateThese & CDS_ALLOC_Title)		&& (cdsObj->Title != NULL))		{ free(cdsObj->Title); }
		if ((cdsObj->DeallocateThese & CDS_ALLOC_Album)		&& (cdsObj->Album != NULL))		{ free(cdsObj->Album); }
		if ((cdsObj->DeallocateThese & CDS_ALLOC_Genre)		&& (cdsObj->Genre != NULL))		{ free(cdsObj->Genre); }
		CDS_DestroyResources(cdsObj->Res);
		
		free (cdsObj);
		cdsObj = nextCds;
	}
}

void CDS_DestroyResources(struct CdsMediaResource *resList)
{
	struct CdsMediaResource *res = resList, *next;

	while (res != NULL)
	{
		next = res->Next;
		if (res->Value != NULL) { free (res->Value); }
		
		if ((res->Allocated & CDS_ALLOC_ProtInfo) && (res->ProtocolInfo != NULL)) 
		{
			free (res->ProtocolInfo); 
		}
		
		if ((res->Allocated & CDS_ALLOC_Protection) && (res->Protection != NULL)) 
		{
			free (res->Protection); 
		}

		free (res);
		res = next;
	}
}
