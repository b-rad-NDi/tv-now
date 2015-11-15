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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "CdsObjectToDidl.h"
#include "CdsStrings.h"
#include "CdsMediaClass.h"
#include "ILibParsers.h"

#ifdef WIN32
#ifndef _WIN32_WCE
#include <crtdbg.h>
#endif
#define strncasecmp(x,y,z) _strnicmp(x,y,z)
#endif

#ifdef _WIN32_WCE
#define strncasecmp(x,y,z) _strnicmp(x,y,z)
#endif

#define MAX_TIME_STRING_SIZE 17

/*
 *	Helper function.
 *	Returns the length for the specified string in its doubly-escaped form.
 */
int CdsObjectToDidl_Helper_DoubleEscapeLength(const char* data)
{
	int i = 0, j = 0;
	while (data[i] != 0)
	{
		switch (data[i])
		{
			case '"':
			j += 10;
			break;
			case '\'':
			j += 10;
			break;
			case '<':
			j += 8;
			break;
			case '>':
			j += 8;
			break;
			case '&':
			j += 9;
			break;
			default:
			j++;
		}
		i++;
	}
	return j;
}

int CdsObjectToDidl_Helper_DoubleEscape(char* outdata, const char* indata)
{
	int i=0;
	int inlen;
	char* out;
	
	out = outdata;
	inlen = (int)strlen(indata);
	
	for (i=0; i < inlen; i++)
	{
		if (indata[i] == '"')
		{
			memcpy(out, "&amp;quot;", 10);
			out = out + 10;
		}
		else
		if (indata[i] == '\'')
		{
			memcpy(out, "&amp;apos;", 10);
			out = out + 10;
		}
		else
		if (indata[i] == '<')
		{
			memcpy(out, "&amp;lt;", 8);
			out = out + 8;
		}
		else
		if (indata[i] == '>')
		{
			memcpy(out, "&amp;gt;", 8);
			out = out + 8;
		}
		else
		if (indata[i] == '&')
		{
			memcpy(out, "&amp;amp;", 9);
			out = out + 9;
		}
		else
		{
			out[0] = indata[i];
			out++;
		}
	}
	
	out[0] = 0;
	
	return (int)(out - outdata);
}

/*
 *	Prints the number of seconds in hh:mm:ss format.
 *	Negative values do not write anything.
 */
void CdsObjectToDidl_Helper_WriteTimeString(char* str, int intTime)
{
	if (intTime > 0)
	{
		sprintf(str,"%02d:%02d:%02d",((intTime/3600)%60),((intTime/60)%60),(intTime%60));
	}
}


/* see header file */
unsigned int CdsToDidl_GetFilterBitString(const char *filter)
{
	unsigned int retVal = 0;
	int i=0;
	int nextComma = 0;

	if (filter != NULL)
	{
		if (filter[0] != '*')
		{
			/*
			 *	The filter string is comma-delimited.
			 *	Do a linear parse and set bits in the retVal
			 *	for supported fields.
			 *
			 *	All supported, filterable metadata fields begin with
			 *	dc:, upnp:, res, container@, or @.
			 */

			while (filter[i] != '\0')
			{
				if (nextComma == 0)
				{
					switch (filter[i])
					{
					case '@':
						nextComma = 1;
						/* Only supported filterable field are container@childCount and container@searchable */
						if (strncasecmp(filter+i, CDS_FILTER_CHILDCOUNT, CDS_FILTER_CHILDCOUNT_LEN) == 0)
						{
							retVal |= CdsFilter_ChildCount;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_SEARCHABLE, CDS_FILTER_SEARCHABLE_LEN) == 0)
						{
							retVal |= CdsFilter_Searchable;
						}
						break;

					case 'c':
						nextComma = 1;
						/* Only supported filterable field are container@childCount and container@searchable */
						if (strncasecmp(filter+i, CDS_FILTER_CONTAINER_CHILDCOUNT, CDS_FILTER_CONTAINER_CHILDCOUNT_LEN) == 0)
						{
							retVal |= CdsFilter_ChildCount;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_CONTAINER_SEARCHABLE, CDS_FILTER_CONTAINER_SEARCHABLE_LEN) == 0)
						{
							retVal |= CdsFilter_Searchable;
						}
						break;

					case 'd':
						nextComma = 1;

						/* Only supported filterable field is dc:creator. (dc:title is always required.) */
						if (strncasecmp(filter+i, CDS_FILTER_CREATOR, CDS_FILTER_CREATOR_LEN) == 0)
						{
							retVal |= CdsFilter_Creator;
						}
						break;

					case 'u':
						/* Only supported filterable field is upnp:album and upnp:genre */
						if (strncasecmp(filter+i, CDS_FILTER_ALBUM, CDS_FILTER_ALBUM_LEN) == 0)
						{
							retVal |= CdsFilter_Album;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_GENRE, CDS_FILTER_GENRE_LEN) == 0)
						{
							retVal |= CdsFilter_Genre;
						}
						nextComma = 1;
						break;

					case 'r':
						nextComma = 1;
						/* only supported fields are: res, resolution, duration, bitrate, colordepth, size */
						
						if (strncasecmp(filter+i, CDS_FILTER_RES_BITRATE, CDS_FILTER_RES_BITRATE_LEN) == 0)
						{
							retVal |= CdsFilter_Res;
							retVal |= CdsFilter_Bitrate;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_RES_BITSPERSAMPLE, CDS_FILTER_RES_BITSPERSAMPLE_LEN) == 0)
						{
							retVal |= CdsFilter_Res;
							retVal |= CdsFilter_BitsPerSample;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_RES_COLORDEPTH, CDS_FILTER_RES_COLORDEPTH_LEN) == 0)
						{
							retVal |= CdsFilter_Res;
							retVal |= CdsFilter_ColorDepth;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_RES_DURATION, CDS_FILTER_RES_DURATION_LEN) == 0)
						{
							retVal |= CdsFilter_Res;
							retVal |= CdsFilter_Duration;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_RES_NRAUDIOCHANNELS, CDS_FILTER_RES_NRAUDIOCHANNELS_LEN) == 0)
						{
							retVal |= CdsFilter_Res;
							retVal |= CdsFilter_nrAudioChannels;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_RES_PROTECTION, CDS_FILTER_RES_PROTECTION_LEN) == 0)
						{
							retVal |= CdsFilter_Res;
							retVal |= CdsFilter_Protection;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_RES_RESOLUTION, CDS_FILTER_RES_RESOLUTION_LEN) == 0)
						{
							retVal |= CdsFilter_Res;
							retVal |= CdsFilter_Resolution;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_RES_SAMPLEFREQUENCY, CDS_FILTER_RES_SAMPLEFREQUENCY_LEN) == 0)
						{
							retVal |= CdsFilter_Res;
							retVal |= CdsFilter_SampleFrequency;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_RES_SIZE, CDS_FILTER_RES_SIZE_LEN) == 0)
						{
							retVal |= CdsFilter_Res;
							retVal |= CdsFilter_Size;
						}
						else if (strncasecmp(filter+i, CDS_FILTER_RES, CDS_FILTER_RES_LEN) == 0)
						{
							/* "res" must be tested after all res attributes */
							retVal |= CdsFilter_Res;
						}
						break;

					default:
						/*
						 *	If the character is an alphabetic char, then go to the next comma
						 *	because we know that this character is not for something we support.
						 */
						if (isalpha(filter[i]))
						{
							nextComma = 1;
						}
					}
				}
				else
				{
					/* We're trying to find the next comma... so if we find it, set the nextComma flag to zero. */
					if (filter[i] == ',')
					{
						nextComma = 0;
					}
				}

				/* move to the next char */
				i++;
			}
		}
		else
		{
			/* assume unsigned int is 32 bits wide */
			retVal = 0xFFFFFFFF;
		}
	}

	return retVal;
}

typedef int (*CdsToDidl_Fn_XmlEscapeLength) (const char* string);
typedef int (*CdsToDidl_Fn_XmlEscape) (char *dest, const char* src);

/* see header file */
char* CdsToDidl_GetMediaObjectDidlEscaped (struct CdsMediaObject *mediaObj, int metadataXmlEscaped, unsigned int filter, int includeHeaderFooter, int *outErrorOrDidlLen)
{
	char *retVal = NULL;	
	int size = 1;			/* allocation size of retVal: minimum of a null character needed; if value is ever negative, then there's an error. */
	int len = 0;			/* actual length of the string in retVal */
	int imc_Object = -1, imc_Major = -1, imc_Minor1 = -1, imc_Minor2 = -1; /* index into the various CDS_CLASS_xxx string arrays */
	struct CdsMediaResource *res;
	unsigned int printThese = 0;		/* similar to filter, except indicates which fields will actually get printed */
	char *cp = NULL;
	char timeString[MAX_TIME_STRING_SIZE];

	CdsToDidl_Fn_XmlEscapeLength fnEscapeLength;
	CdsToDidl_Fn_XmlEscape		 fnEscape;

	/*
	 *	Given the info about the metadata,
	 *	obtain function pointers to the appropriate
	 *	functions for calculating lengths and escaping strings.
	 */
	if (metadataXmlEscaped)
	{
		fnEscapeLength = ILibXmlEscapeLength;
		fnEscape = ILibXmlEscape;
	}
	else
	{
		fnEscapeLength = CdsObjectToDidl_Helper_DoubleEscapeLength;
		fnEscape = CdsObjectToDidl_Helper_DoubleEscape;
	}


	/*
	 *	Include length needed for metadata content.
	 *	We need to doubly-escape because the data is a value of an XML element or attribute,
	 *	but the DIDL-Lite XML itself needs to be escaped, so this means all of the
	 *	values of XML attributes and elements need escaping too.
	 *
	 *	If at any point we determine there's an error with the media object, we
	 *	set "size" to an appropriate negative error value.
	 */
	if ((mediaObj->ID != NULL) && ((int)strlen(mediaObj->ID) > 0))
	{
		size += fnEscapeLength(mediaObj->ID);

		if ((mediaObj->ParentID != NULL) && (strlen(mediaObj->ParentID) > 0))
		{

			size += fnEscapeLength(mediaObj->ParentID);	

			if ((mediaObj->Title != NULL) && (strlen(mediaObj->Title) > 0))
			{
				size += (CDS_DIDL_TITLE_ESCAPED_LEN + fnEscapeLength(mediaObj->Title));		

				/* ObjectID, ParentID, and Title are valid... */

				if (mediaObj->RefID != NULL)		
				{
					size += fnEscapeLength(mediaObj->RefID);		
				}
				
				/* do not add memory for creator unless requested by the metadata filter */
				if ((mediaObj->Creator != NULL) && (filter & CdsFilter_Creator))
				{
					size += (CDS_DIDL_CREATOR_ESCAPED_LEN + fnEscapeLength(mediaObj->Creator));
					printThese |= CdsFilter_Creator;
				}

				/* do not add memory for album unless requested by the metadata filter */
				if ((mediaObj->Album != NULL) && (filter & CdsFilter_Album))
				{
					size += (CDS_DIDL_ALBUM_ESCAPED_LEN + fnEscapeLength(mediaObj->Album));
					printThese |= CdsFilter_Album;
				}

				/* do not add memory for genre unless requested by the metadata filter */
				if ((mediaObj->Genre != NULL) && (filter & CdsFilter_Genre))
				{
					size += (CDS_DIDL_GENRE_ESCAPED_LEN + fnEscapeLength(mediaObj->Genre));
					printThese |= CdsFilter_Genre;
				}

				/* validate the media class */
				if ((mediaObj->MediaClass != 0) & (size > 0))
				{
					if (
						(mediaObj->RefID != NULL) &&
						((mediaObj->MediaClass & CDS_CLASS_MASK_CONTAINER) != 0)
						)
					{
						/* media class is a container of some sort, but we have refID value - this is an error */
						size = Error_CdsObjectToDidl_MismatchContainerRefID;
					}
					else
					{
						imc_Object = mediaObj->MediaClass & CDS_CLASS_MASK_OBJECTTYPE;
						imc_Major  = (mediaObj->MediaClass & CDS_CLASS_MASK_MAJOR) >> CDS_SHIFT_MAJOR_TYPE;
						imc_Minor1 = (mediaObj->MediaClass & CDS_CLASS_MASK_MINOR1) >> CDS_SHIFT_MINOR1_TYPE;
						imc_Minor2 = (mediaObj->MediaClass & CDS_CLASS_MASK_MINOR2) >> CDS_SHIFT_MINOR2_TYPE;

						/* check for various error conditions with media class */
						if ((imc_Object > CDS_CLASS_OBJECT_TYPE_LEN) || (imc_Object < 1))
						{
							size = Error_CdsObjectToDidl_UndefinedObjectType;
						}
						else if ((imc_Major > CDS_CLASS_MAJOR_TYPE_LEN) || (imc_Major < 0))
						{
							size = Error_CdsObjectToDidl_UndefinedMajorType;
						}
						else if	((imc_Minor1 > CDS_CLASS_MINOR1_TYPE_LEN) || (imc_Minor1 < 0))
						{
							size = Error_CdsObjectToDidl_UndefinedMinor1Type;
						}
						else if	((imc_Minor2 > CDS_CLASS_MINOR2_TYPE_LEN) || (imc_Minor2 < 0))
						{
							size = Error_CdsObjectToDidl_UndefinedMinor2Type;
						}
						else
						{
							/* media class is valid - calculate length - assume no strings need escaping */
							size += (int) strlen(CDS_CLASS_OBJECT_TYPE[imc_Object]);
							size += (int) strlen(CDS_CLASS_MAJOR_TYPE[imc_Major]);
							size += (int) strlen(CDS_CLASS_MINOR1_TYPE[imc_Minor1]);
							size += (int) strlen(CDS_CLASS_MINOR2_TYPE[imc_Minor2]);
							size += CDS_DIDL_CLASS_ESCAPED_LEN + 4;	/*add 4 for additional . chars */

							/*
							*	Note the length needed for item/container element open and close tags.
							*	Also note additional length for restricted, searchable attributes.
							*/
							size++; /* restricted */
							if ((mediaObj->MediaClass & CDS_CLASS_MASK_CONTAINER) != 0)
							{
								size++; /* byte for searchable flag */
								size +=10; /* bytes for childCount attribute */
								size += CDS_DIDL_CONTAINER_START_ESCAPED_LEN;
								size += CDS_DIDL_CONTAINER_END_ESCAPED_LEN;

								if ((mediaObj->MediaClass & CDS_MEDIACLASS_EPGCONTAINER) != 0)
								{
									if (mediaObj->epgProviderName != NULL && filter & CdsFilter_epgProviderName)
									{
										size += CDS_DIDL_EPG_PROVIDER_LEN;
										size += fnEscapeLength(mediaObj->epgProviderName);
										printThese |= CdsFilter_epgProviderName;
									}
									if (mediaObj->serviceProvider && filter & CdsFilter_ServiceProvider)
									{
										printThese |= CdsFilter_ServiceProvider;
										size += CDS_DIDL_SVC_PROVIDER_LEN;
										size += fnEscapeLength(mediaObj->serviceProvider);
									}
									if (mediaObj->ChannelID && filter & CdsFilter_ChannelID)
									{
										printThese |= CdsFilter_ChannelID;
										size += CDS_DIDL_CHANNEL_ID_LEN;
										size += fnEscapeLength(mediaObj->ChannelID);
									}
									if (mediaObj->DateTimeRange.start > 0 && mediaObj->DateTimeRange.duration > 0 &&
									    filter & CdsFilter_DateTimeRange)
									{
										printThese |= CdsFilter_DateTimeRange;
										size += CDS_DIDL_DATE_RANGE_LEN;
										size += 64; /* length should be 41, pad for various formats */
									}
								}
							}
							else if ((mediaObj->MediaClass & CDS_CLASS_MASK_ITEM) != 0)
							{
								if (mediaObj->RefID != NULL)
								{
									size += CDS_DIDL_REFITEM_START_ESCAPED_LEN;
								}
								else
								{
									size += CDS_DIDL_ITEM_START_ESCAPED_LEN;
								}

								size += CDS_DIDL_ITEM_END_ESCAPED_LEN;
							}

							/*
							 *	At this point, we add the amount of memory needed
							 *	for the resource elements to the size... if and only
							 *	if <res> elements were requsted.
							 */
							if (filter & CdsFilter_Res)
							{
								printThese |= CdsFilter_Res;

								res = mediaObj->Res;
								while (res != NULL)
								{
									/* add size for minimal <res> element and its metadata */
									size += (CDS_DIDL_RES_START_ESCAPED_LEN + CDS_DIDL_RES_VALUE_ESCAPED_LEN);

									size += fnEscapeLength(res->ProtocolInfo);

									if (res->Value != NULL)
									{
										size += fnEscapeLength(res->Value);
									}

									if ((res->Bitrate >= 0) && (filter & CdsFilter_Bitrate))
									{
										printThese |= CdsFilter_Bitrate;
										size += (CDS_DIDL_ATTRIB_BITRATE_ESCAPED_LEN + SIZE_INT32_AS_CHARS);
									}
									if ((res->BitsPerSample >= 0) && (filter & CdsFilter_BitsPerSample))
									{
										printThese |= CdsFilter_BitsPerSample;
										size += (CDS_DIDL_ATTRIB_BITSPERSAMPLE_ESCAPED_LEN + SIZE_INT32_AS_CHARS);
									}
									if ((res->ColorDepth >= 0) && (filter & CdsFilter_ColorDepth))
									{
										printThese |= CdsFilter_ColorDepth;
										size += (CDS_DIDL_ATTRIB_COLORDEPTH_ESCAPED_LEN	 + SIZE_INT32_AS_CHARS);
									}
									if ((res->Duration >= 0) && (filter & CdsFilter_Duration))
									{
										printThese |= CdsFilter_Duration;
										size += (CDS_DIDL_ATTRIB_DURATION_ESCAPED_LEN + MAX_TIME_STRING_SIZE);
									}
									if ((res->NrAudioChannels >= 0) && (filter & CdsFilter_nrAudioChannels))
									{
										printThese |= CdsFilter_nrAudioChannels;
										size += (CDS_DIDL_ATTRIB_NRAUDIOCHANNELS_ESCAPED_LEN + SIZE_INT32_AS_CHARS);
									}
									if ((res->Protection != NULL) && (filter & CdsFilter_Protection))
									{
										printThese |= CdsFilter_Protection;
										size += (CDS_DIDL_ATTRIB_PROTECTION_ESCAPED_LEN + fnEscapeLength(res->Protection) + 1);
									}
									if ((res->ResolutionX >= 0) && (res->ResolutionY > 0) && (filter & CdsFilter_Resolution))
									{
										printThese |= CdsFilter_Resolution;
										size += (CDS_DIDL_ATTRIB_RESOLUTION_ESCAPED_LEN + SIZE_INT32_AS_CHARS + SIZE_INT32_AS_CHARS);
									}
									if ((res->SampleFrequency >= 0) && (filter & CdsFilter_SampleFrequency))
									{
										printThese |= CdsFilter_SampleFrequency;
										size += (CDS_DIDL_ATTRIB_SAMPLEFREQUENCY_ESCAPED_LEN + SIZE_INT32_AS_CHARS);
									}
									if ((res->Size >= 0) && (filter & CdsFilter_Size))
									{
										printThese |= CdsFilter_Size;
										size += (CDS_DIDL_ATTRIB_SIZE_ESCAPED_LEN + SIZE_INT64_AS_CHARS);
									}

									res = res->Next;
								}
							}

							if (mediaObj->ChannelNr != NULL && filter & CdsFilter_ChannelNr)
							{
								printThese |= CdsFilter_ChannelNr;
								size += CDS_DIDL_CHANNEL_NUM_LEN;
								size += fnEscapeLength(mediaObj->ChannelNr);
							}
							if (mediaObj->ChannelName != NULL && filter & CdsFilter_ChannelName)
							{
								printThese |= CdsFilter_ChannelName;
								size += CDS_DIDL_CHANNEL_NAME_LEN;
								size += fnEscapeLength(mediaObj->ChannelName);
							}
							if (mediaObj->ChannelID != NULL && filter & CdsFilter_ChannelID)
							{
								printThese |= CdsFilter_ChannelID;
								size += CDS_DIDL_CHANNEL_ID_LEN;
								size += fnEscapeLength(mediaObj->ChannelID);
							}
							if (mediaObj->CallSign != NULL && filter & CdsFilter_CallSign)
							{
								printThese |= CdsFilter_CallSign;
								size += CDS_DIDL_CALL_SIGN_LEN;
								size += fnEscapeLength(mediaObj->CallSign);
							}
							if (mediaObj->NetworkAffiliation != NULL && filter & CdsFilter_NetworkAffil)
							{
								printThese |= CdsFilter_NetworkAffil;
								size += CDS_DIDL_NETWORK_AFFIL_LEN;
								size += fnEscapeLength(mediaObj->NetworkAffiliation);
							}
							if (mediaObj->Recordable >= 0 && filter & CdsFilter_Recordable)
							{
								printThese |= CdsFilter_Recordable;
								size += CDS_DIDL_RECORDABLE_LEN + 3;
							}
							if (mediaObj->Date > 0 && filter & CdsFilter_Date)
							{
								printThese |= CdsFilter_Date;
								size += CDS_DIDL_DATE_LEN + 32;
							}
							if (mediaObj->ScheduledStartTime > 0 && filter & CdsFilter_SchedStartTime)
							{
								printThese |= CdsFilter_SchedStartTime;
								size += CDS_DIDL_START_TIME_LEN + 32;
							}
							if (mediaObj->ScheduledEndTime > 0 && filter & CdsFilter_SchedEndTime)
							{
								printThese |= CdsFilter_SchedEndTime;
								size += CDS_DIDL_END_TIME_LEN + 32;
							}
							if (mediaObj->ScheduledDurationTime > 0 && filter & CdsFilter_SchedDurationTime)
							{
								printThese |= CdsFilter_SchedDurationTime;
								size += CDS_DIDL_DURATION_LEN + 16;
							}
							if (mediaObj->ProgramID != NULL && filter & CdsFilter_ProgramID)
							{
								printThese |= CdsFilter_ProgramID;
								size += CDS_DIDL_PROGRAM_ID_LEN;
								size += fnEscapeLength(mediaObj->ProgramID);
							}
							if (mediaObj->SeriesID != NULL && filter & CdsFilter_SeriesID)
							{
								printThese |= CdsFilter_SeriesID;
								size += CDS_DIDL_SERIES_ID_LEN;
								size += fnEscapeLength(mediaObj->SeriesID);
							}
							if (mediaObj->EpisodeNumber != NULL && filter & CdsFilter_EpisodeNumber)
							{
								printThese |= CdsFilter_EpisodeNumber;
								size += CDS_DIDL_EPISODE_NR_LEN;
								size += fnEscapeLength(mediaObj->EpisodeNumber);
							}
							if (mediaObj->EpisodeSeason != NULL && filter & CdsFilter_EpisodeSeason)
							{
								printThese |= CdsFilter_EpisodeSeason;
								size += CDS_DIDL_EPISODE_SEASON_LEN;
								size += fnEscapeLength(mediaObj->EpisodeSeason);
							}
							if (mediaObj->EpisodeType != NULL && filter & CdsFilter_EpisodeType)
							{
								printThese |= CdsFilter_EpisodeType;
								size += CDS_DIDL_EPISODE_TYPE_LEN;
								size += fnEscapeLength(mediaObj->EpisodeType);
							}
							if (mediaObj->LongDescription != NULL && filter & CdsFilter_LongDescription)
							{
								printThese |= CdsFilter_LongDescription;
								size += CDS_DIDL_LONG_DESCR_LEN;
								size += fnEscapeLength(mediaObj->LongDescription);
							}
							if (mediaObj->Rating != NULL && filter & CdsFilter_Rating)
							{
								printThese |= CdsFilter_Rating;
								size += CDS_DIDL_RATING_LEN;
								size += fnEscapeLength(mediaObj->Rating);
							}
							if (mediaObj->Language != NULL && filter & CdsFilter_Language)
							{
								printThese |= CdsFilter_Language;
								size += CDS_DIDL_LANGUAGE_LEN;
								size += fnEscapeLength(mediaObj->Language);
							}
							if (mediaObj->Actor != NULL && filter & CdsFilter_Actor)
							{
								/* TODO: support multiple actors */
								printThese |= CdsFilter_Actor;
								size += CDS_DIDL_ACTOR_LEN;
								size += fnEscapeLength(mediaObj->Actor);
							}
							if (mediaObj->AlbumArtURI != NULL && filter & CdsFilter_AlbumArtURI)
							{
								printThese |= CdsFilter_AlbumArtURI;
								size += CDS_DIDL_RES_THUMBNAIL_LEN;
								size += fnEscapeLength(mediaObj->AlbumArtURI);
							}

							if (includeHeaderFooter != 0)
							{
								/* appropriately include length for header/footer, if requested */
								size += CDS_DIDL_HEADER_ESCAPED_LEN;
								size += CDS_DIDL_FOOTER_ESCAPED_LEN;
							}
						}
					}
				}
				else
				{
					/* invalid media class - this is an error */
					size = 	Error_CdsObjectToDidl_InvalidMediaClass;
				}
			}
			else
			{
				/* title cannot be empty or null */
				size = Error_CdsObjectToDidl_EmptyTitle;
			}
		}
		else
		{
			/* parent ID must not be empty or null */
			size = Error_CdsObjectToDidl_EmptyParentID;
		}
	}
	else
	{
		/* object ID cannot be empty or null */
		size = Error_CdsObjectToDidl_EmptyObjectID;
	}

	if (size > 0)
	{
		/*
		 *	If this code executes, then the media object can be serialized to DIDL-Lite
		 *	without any problems.
		 */

		cp = retVal = (char*) malloc (size);

		/* print DIDL-Lite element if requested */
		if (includeHeaderFooter != 0)
		{
			cp += sprintf(cp, CDS_DIDL_HEADER_ESCAPED);
		}

		/* print <item> or <container> start */
		if ((mediaObj->MediaClass & CDS_CLASS_MASK_CONTAINER) != 0)
		{
			cp += sprintf(cp, CDS_DIDL_CONTAINER_START1_ESCAPED);
			cp += fnEscape(cp, mediaObj->ID);
			
			cp += sprintf(cp, CDS_DIDL_CONTAINER_START2_ESCAPED);
			cp += fnEscape(cp, mediaObj->ParentID);
			
			cp += sprintf(cp, CDS_DIDL_CONTAINER_START3_ESCAPED);
			if (mediaObj->Flags & CDS_OBJPROP_FLAGS_Restricted)
			{
				cp[0] = '1';
			}
			else
			{
				cp[0] = '0';
			}
			cp++;
			
			if (filter & CdsFilter_Searchable)
			{
				cp += sprintf(cp, CDS_DIDL_CONTAINER_START4_ESCAPED);
				if (mediaObj->Flags & CDS_OBJPROP_FLAGS_Searchable )
				{
					cp[0] = '1';
				}
				else
				{
					cp[0] = '0';
				}
				cp++;
			}

			if (filter & CdsFilter_ChildCount)
			{
				cp += sprintf(cp, CDS_DIDL_CONTAINER_START5_ESCAPED, mediaObj->ChildCount);
			}

			cp += sprintf(cp, CDS_DIDL_CONTAINER_START6_ESCAPED);
		}
		else
		{
			cp += sprintf(cp, CDS_DIDL_ITEM_START1_ESCAPED);
			cp += fnEscape(cp, mediaObj->ID);
			
			cp += sprintf(cp, CDS_DIDL_ITEM_START2_ESCAPED);
			cp += fnEscape(cp, mediaObj->ParentID);
			
			cp += sprintf(cp, CDS_DIDL_ITEM_START3_ESCAPED);
			if (mediaObj->Flags & CDS_OBJPROP_FLAGS_Restricted)
			{
				cp[0] = '1';
			}
			else
			{
				cp[0] = '0';
			}
			cp++;

			if ((mediaObj->RefID != NULL) && (strlen(mediaObj->RefID) > 0))
			{
				cp += sprintf(cp, CDS_DIDL_REFITEM_START4_ESCAPED);
				cp += fnEscape(cp, mediaObj->RefID);
				cp += sprintf(cp, CDS_DIDL_REFITEM_START5_ESCAPED);
			}
			else
			{
				cp += sprintf(cp, CDS_DIDL_ITEM_START4_ESCAPED);
			}
		}

		/* print title */
		cp += sprintf(cp, CDS_DIDL_TITLE1_ESCAPED);
		cp += fnEscape(cp, mediaObj->Title);
		cp += sprintf(cp, CDS_DIDL_TITLE2_ESCAPED);

		/* print media class */
		cp += sprintf(cp, CDS_DIDL_CLASS1_ESCAPED);
		cp += sprintf(cp, CDS_CLASS_OBJECT_TYPE[imc_Object]);
		if (imc_Major > 0) { cp += sprintf(cp, ".%s", CDS_CLASS_MAJOR_TYPE[imc_Major]); }
		if (imc_Minor1 > 0) { cp += sprintf(cp, ".%s", CDS_CLASS_MINOR1_TYPE[imc_Minor1]); }
		if (imc_Minor2 > 0) { cp += sprintf(cp, ".%s", CDS_CLASS_MINOR2_TYPE[imc_Minor2]); }
		cp += sprintf(cp, CDS_DIDL_CLASS2_ESCAPED);

		/* print resource and appropriate fields */
		if (printThese & CdsFilter_Res)
		{
			res = mediaObj->Res;

			while (res != NULL)
			{
				/*
				 *	Need to double escape because DIDL-Lite is XML and the DIDL-Lite is
				 *	going within a SOAP response message, which is also XML.
				 */
				cp += sprintf(cp, CDS_DIDL_RES_START1_ESCAPED);
				if (metadataXmlEscaped)
				{
					cp += ILibXmlEscape(cp, res->ProtocolInfo);
				}
				else
				{
					cp += CdsObjectToDidl_Helper_DoubleEscape(cp, res->ProtocolInfo);
				}
				cp += sprintf(cp, CDS_DIDL_RES_START2_ESCAPED);
				
				if ((res->Bitrate >= 0) && (printThese & CdsFilter_Bitrate))
				{
					/* CDS spec is strange in that bitRate is actually supposed to have the byte rate */
					cp += sprintf(cp, CDS_DIDL_ATTRIB_BITRATE_ESCAPED, (res->Bitrate / 8));
				}
				if ((res->BitsPerSample >= 0) && (printThese & CdsFilter_BitsPerSample))
				{
					cp += sprintf(cp, CDS_DIDL_ATTRIB_BITSPERSAMPLE_ESCAPED, res->BitsPerSample);
				}
				if ((res->ColorDepth >= 0) && (printThese & CdsFilter_ColorDepth))
				{
					cp += sprintf(cp, CDS_DIDL_ATTRIB_COLORDEPTH_ESCAPED, res->ColorDepth);
				}
				if ((res->Duration >= 0) && (printThese & CdsFilter_Duration))
				{
					CdsObjectToDidl_Helper_WriteTimeString(timeString, res->Duration);
					cp += sprintf(cp, CDS_DIDL_ATTRIB_DURATION_ESCAPED, timeString);
				}
				if ((res->NrAudioChannels >= 0) && (printThese & CdsFilter_nrAudioChannels))
				{
					cp += sprintf(cp, CDS_DIDL_ATTRIB_NRAUDIOCHANNELS_ESCAPED, res->NrAudioChannels);
				}
				if ((res->Protection != NULL) && (printThese & CdsFilter_Protection))
				{
					cp += sprintf(cp, CDS_DIDL_ATTRIB_PROTECTION_START);
					cp += fnEscape(cp, res->Protection);
					cp += sprintf(cp, CDS_DIDL_ATTRIB_PROTECTION_END);
				}
				if ((res->ResolutionX >= 0) && (res->ResolutionY >= 0) && (printThese & CdsFilter_Resolution))
				{
					cp += sprintf(cp, CDS_DIDL_ATTRIB_RESOLUTION_ESCAPED, res->ResolutionX, res->ResolutionY);
				}
				if ((res->SampleFrequency >= 0) && (printThese & CdsFilter_SampleFrequency))
				{
					cp += sprintf(cp, CDS_DIDL_ATTRIB_SAMPLEFREQUENCY_ESCAPED, res->SampleFrequency);
				}
				if ((res->Size >= 0) && (printThese & CdsFilter_Size))
				{
					cp += sprintf(cp, CDS_DIDL_ATTRIB_SIZE_ESCAPED, res->Size);
				}

				cp += sprintf(cp, CDS_DIDL_RES_VALUE1_ESCAPED);
				if (res->Value != NULL)
				{
					cp += fnEscape(cp, res->Value);
				}
				cp += sprintf(cp, CDS_DIDL_RES_VALUE2_ESCAPED);

				res = res->Next;
			}
		}
		if (printThese & CdsFilter_epgProviderName)
		{
			cp += sprintf(cp, CDS_DIDL_EPG_PROVIDER1);
			cp += fnEscape(cp, mediaObj->epgProviderName);
			cp += sprintf(cp, CDS_DIDL_EPG_PROVIDER2);
		}
		if (printThese & CdsFilter_ServiceProvider)
		{
			cp += sprintf(cp, CDS_DIDL_SVC_PROVIDER1);
			cp += fnEscape(cp, mediaObj->serviceProvider);
			cp += sprintf(cp, CDS_DIDL_SVC_PROVIDER2);
		}
		if (printThese & CdsFilter_ChannelID)
		{
			cp += sprintf(cp, CDS_DIDL_CHANNEL_ID1);
			cp += fnEscape(cp, mediaObj->ChannelID);
			cp += sprintf(cp, CDS_DIDL_CHANNEL_ID2);
		}
		if (printThese & CdsFilter_DateTimeRange)
		{
			cp += sprintf(cp, CDS_DIDL_DATE_RANGE1);
			/* TODO: format date range */
			cp += fnEscape(cp, "");
			cp += sprintf(cp, CDS_DIDL_DATE_RANGE2);
		}

		/* print creator */
		if (printThese & CdsFilter_Creator)
		{
			cp += sprintf(cp, CDS_DIDL_CREATOR1_ESCAPED);
			cp += fnEscape(cp, mediaObj->Creator);
			cp += sprintf(cp, CDS_DIDL_CREATOR2_ESCAPED);
		}

		/* print genre */
		if (printThese & CdsFilter_Genre)
		{
			/* TODO: check for mediaObj->GenreExtended */
			cp += sprintf(cp, CDS_DIDL_GENRE1_ESCAPED);
			cp += fnEscape(cp, mediaObj->Genre);
			cp += sprintf(cp, CDS_DIDL_GENRE2_ESCAPED);
		}

		/* print album */
		if (printThese & CdsFilter_Album)
		{
			cp += sprintf(cp, CDS_DIDL_ALBUM1_ESCAPED);
			cp += fnEscape(cp, mediaObj->Album);
			cp += sprintf(cp, CDS_DIDL_ALBUM2_ESCAPED);
		}

		if ((mediaObj->MediaClass & CDS_CLASS_MASK_CONTAINER) != 0)
		{
			cp += sprintf(cp, CDS_DIDL_CONTAINER_END_ESCAPED);
		}
		else
		{
			cp += sprintf(cp, CDS_DIDL_ITEM_END_ESCAPED);
		}

		if (includeHeaderFooter != 0)
		{
			sprintf(cp, CDS_DIDL_FOOTER_ESCAPED);
		}

		len = (int) (cp - retVal);
		if (len >= size)
		{
			/* we overwrote memory - this is bad */
			free (retVal);
			retVal = NULL;
			size = Error_CdsObjectToDidl_CorruptedMemory;
		}
		else
		{
			size = len;
		}
	}

	if (outErrorOrDidlLen != NULL)
	{
		*outErrorOrDidlLen = size;
	}

	return retVal;
}
