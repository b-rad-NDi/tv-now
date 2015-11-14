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

#ifndef _CDS_STRINGS_H
#define _CDS_STRINGS_H

/*
 *	Number of chars needed to represent 32 and 64 bit integers, including sign and null char.
 */
#define SIZE_INT32_AS_CHARS				12
#define SIZE_INT64_AS_CHARS				24
/*
 *	Defines a number of well-defined CDS related strings
 *	for DIDL-Lite as well as UPnP argument names.
 */

#define CDS_STRING_URN_CDS				"urn:schemas-upnp-org:service:ContentDirectory:1"
#define CDS_STRING_BROWSE				"Browse"
#define CDS_STRING_SEARCH				"Search"
#define CDS_STRING_RESULT				"Result"
#define CDS_STRING_NUMBER_RETURNED		"NumberReturned"
#define CDS_STRING_TOTAL_MATCHES		"TotalMatches"
#define CDS_STRING_UPDATE_ID			"UpdateID"

#define CDS_ROOT_CONTAINER_ID			"0"

#define CDS_STRING_BROWSE_DIRECT_CHILDREN		"BrowseDirectChildren"
#define CDS_STRING_BROWSE_METADATA				"BrowseMetadata"

#define CDS_DIDL_TITLE							"\r\n<dc:title>%s</dc:title>"
#define CDS_DIDL_TITLE_LEN						23

#define CDS_DIDL_CREATOR						"\r\n<dc:creator>%s</dc:creator>"
#define CDS_DIDL_CREATOR_LEN					27

#define CDS_DIDL_CLASS							"\r\n<upnp:class>%s</upnp:class>"
#define CDS_DIDL_CLASS_LEN						27

#define CDS_DIDL_ITEM_START						"\r\n<item id=\"%s\" parentID=\"%s\" restricted=\"%d\">"
#define CDS_DIDL_ITEM_START_LEN					40

#define CDS_DIDL_REFITEM_START					"\r\n<item id=\"%s\" parentID=\"%s\" restricted=\"%d\" refID=\"%s\">"
#define CDS_DIDL_REFITEM_START_LEN				49

#define CDS_DIDL_ITEM_END						"\r\n</item>"
#define CDS_DIDL_ITEM_END_LEN					9

#define CDS_DIDL_CONTAINER_START				"\r\n<container id=\"%s\" parentID=\"%s\" restricted=\"%d\" searchable=\"%d\">"
#define CDS_DIDL_CONTAINER_START_LEN			59

#define CDS_DIDL_CONTAINER_END					"\r\n</container>"
#define CDS_DIDL_CONTAINER_END_LEN				16

#define CDS_DIDL_RES_START						"\r\n<res protocolInfo=\"%s\""
#define CDS_DIDL_RES_START_LEN					22

#define CDS_DIDL_ATTRIB_RESOLUTION				" resolution=\"%dx%d\""
#define CDS_DIDL_ATTRIB_RESOLUTION_LEN			15

#define CDS_DIDL_ATTRIB_DURATION				" duration=\"%s\""
#define CDS_DIDL_ATTRIB_DURATION_LEN			12

#define CDS_DIDL_ATTRIB_BITRATE					" bitrate=\"%d\""
#define CDS_DIDL_ATTRIB_BITRATE_LEN				11

#define CDS_DIDL_ATTRIB_COLORDEPTH				" colorDepth=\"%d\""
#define CDS_DIDL_ATTRIB_COLORDEPTH_LEN			14

#define CDS_DIDL_ATTRIB_SIZE					" size=\"%" PRIu64 "\""
#define CDS_DIDL_ATTRIB_SIZE_LEN				8

#define CDS_DIDL_RES_VALUE						">%s</res>"
#define CDS_DIDL_RES_VALUE_LEN					7


#define CDS_DIDL_HEADER_ESCAPED					"&lt;DIDL-Lite xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/&quot; xmlns:dc=&quot;http://purl.org/dc/elements/1.1/&quot; xmlns:upnp=&quot;urn:schemas-upnp-org:metadata-1-0/upnp/&quot;&gt;"
#define CDS_DIDL_HEADER_ESCAPED_LEN				197

#define CDS_DIDL_FOOTER_ESCAPED					"\r\n&lt;/DIDL-Lite&gt;"
#define CDS_DIDL_FOOTER_ESCAPED_LEN				20

#define CDS_DIDL_TITLE_ESCAPED					"\r\n&lt;dc:title&gt;%s&lt;/dc:title&gt;"
#define CDS_DIDL_TITLE_ESCAPED_LEN				35

#define CDS_DIDL_TITLE1_ESCAPED					"\r\n&lt;dc:title&gt;"
#define CDS_DIDL_TITLE2_ESCAPED					"&lt;/dc:title&gt;"

#define CDS_DIDL_CREATOR_ESCAPED				"\r\n&lt;dc:creator&gt;%s&lt;/dc:creator&gt;"
#define CDS_DIDL_CREATOR_ESCAPED_LEN			39

#define CDS_DIDL_CREATOR1_ESCAPED				"\r\n&lt;dc:creator&gt;"
#define CDS_DIDL_CREATOR2_ESCAPED				"&lt;/dc:creator&gt;"

#define CDS_DIDL_ALBUM_ESCAPED					"\r\n&lt;upnp:album&gt;%s&lt;/upnp:album&gt;"
#define CDS_DIDL_ALBUM_ESCAPED_LEN				39

#define CDS_DIDL_ALBUM1_ESCAPED					"\r\n&lt;upnp:album&gt;"
#define CDS_DIDL_ALBUM2_ESCAPED					"&lt;/upnp:album&gt;"

#define CDS_DIDL_GENRE_ESCAPED					"\r\n&lt;upnp:genre&gt;%s&lt;/upnp:genre&gt;"
#define CDS_DIDL_GENRE_ESCAPED_LEN				39

#define CDS_DIDL_GENRE1_ESCAPED					"\r\n&lt;upnp:genre&gt;"
#define CDS_DIDL_GENRE2_ESCAPED					"&lt;/upnp:genre&gt;"

#define CDS_DIDL_GENRE_EXT1_ESCAPED				"\r\n&lt;upnp:genre extended=\"%s\"&gt;"
#define CDS_DIDL_GENRE_EXT2_ESCAPED				"&lt;/upnp:genre&gt;"
#define CDS_DIDL_GENRE_EXT2_ESCAPED_LEN			55

#define CDS_DIDL_CLASS_ESCAPED					"\r\n&lt;upnp:class&gt;%s&lt;/upnp:class&gt;"
#define CDS_DIDL_CLASS_ESCAPED_LEN				39

#define CDS_DIDL_CLASS1_ESCAPED					"\r\n&lt;upnp:class&gt;"
#define CDS_DIDL_CLASS2_ESCAPED					"&lt;/upnp:class&gt;"

#define CDS_DIDL_ITEM_START_ESCAPED				"\r\n&lt;item id=&quot;%s&quot; parentID=&quot;%s&quot; restricted=&quot;%d&quot;&gt;"
#define CDS_DIDL_ITEM_START_ESCAPED_LEN			76

#define CDS_DIDL_ITEM_START1_ESCAPED			"\r\n&lt;item id=&quot;"
#define CDS_DIDL_ITEM_START2_ESCAPED			"&quot; parentID=&quot;"
#define CDS_DIDL_ITEM_START3_ESCAPED			"&quot; restricted=&quot;"
#define CDS_DIDL_ITEM_START4_ESCAPED			"&quot;&gt;"

#define CDS_DIDL_REFITEM_START_ESCAPED			"\r\n&lt;item id=&quot;%s&quot; parentID=&quot;%s&quot; restricted=&quot;%d&quot; refID=&quot;%s&quot;&gt;"
#define CDS_DIDL_REFITEM_START_ESCAPED_LEN		95

#define CDS_DIDL_REFITEM_START1_ESCAPED			"\r\n&lt;item id=&quot;"
#define CDS_DIDL_REFITEM_START2_ESCAPED			"&quot; parentID=&quot;"
#define CDS_DIDL_REFITEM_START3_ESCAPED			"&quot; restricted=&quot;"
#define CDS_DIDL_REFITEM_START4_ESCAPED			"&quot; refID=&quot;"
#define CDS_DIDL_REFITEM_START5_ESCAPED			"&quot;&gt;"

#define CDS_DIDL_ITEM_END_ESCAPED				"\r\n&lt;/item&gt;"
#define CDS_DIDL_ITEM_END_ESCAPED_LEN			15

#define CDS_DIDL_CONTAINER_START_ESCAPED		"\r\n&lt;container id=&quot;%s&quot; parentID=&quot;%s&quot; restricted=&quot;%d&quot;&gt; searchable=&quot;%d&quot; childCount=&quot;%d&quot;"
#define CDS_DIDL_CONTAINER_START_ESCAPED_LEN	129

#define CDS_DIDL_CONTAINER_START1_ESCAPED		"\r\n&lt;container id=&quot;"
#define CDS_DIDL_CONTAINER_START2_ESCAPED		"&quot; parentID=&quot;"
#define CDS_DIDL_CONTAINER_START3_ESCAPED       "&quot; restricted=&quot;"
#define CDS_DIDL_CONTAINER_START4_ESCAPED       "&quot; searchable=&quot;"
#define CDS_DIDL_CONTAINER_START5_ESCAPED       "&quot; childCount=&quot;%d"
#define CDS_DIDL_CONTAINER_START6_ESCAPED		"&quot;&gt;"

#define CDS_DIDL_CONTAINER_END_ESCAPED			"\r\n&lt;/container&gt;"
#define CDS_DIDL_CONTAINER_END_ESCAPED_LEN		20

#define CDS_DIDL_RES_START_ESCAPED				"\r\n&lt;res protocolInfo=&quot;%s&quot;"
#define CDS_DIDL_RES_START_ESCAPED_LEN			35

#define CDS_DIDL_RES_START1_ESCAPED				"\r\n&lt;res protocolInfo=&quot;"
#define CDS_DIDL_RES_START2_ESCAPED				"&quot;"

#define CDS_DIDL_ATTRIB_RESOLUTION_ESCAPED		" resolution=&quot;%dx%d&quot;"
#define CDS_DIDL_ATTRIB_RESOLUTION_ESCAPED_LEN	24

#define CDS_DIDL_ATTRIB_DURATION_ESCAPED		" duration=&quot;%s&quot;"
#define CDS_DIDL_ATTRIB_DURATION_ESCAPED_LEN	22

#define CDS_DIDL_ATTRIB_BITRATE_ESCAPED			" bitrate=&quot;%d&quot;"
#define CDS_DIDL_ATTRIB_BITRATE_ESCAPED_LEN		21

#define CDS_DIDL_ATTRIB_BITSPERSAMPLE_ESCAPED	" bitsPerSample=&quot;%d&quot;"
#define CDS_DIDL_ATTRIB_BITSPERSAMPLE_ESCAPED_LEN	27

#define CDS_DIDL_ATTRIB_COLORDEPTH_ESCAPED		" colorDepth=&quot;%d&quot;"
#define CDS_DIDL_ATTRIB_COLORDEPTH_ESCAPED_LEN	24

#define CDS_DIDL_ATTRIB_NRAUDIOCHANNELS_ESCAPED	" nrAudioChannels=&quot;%d&quot;"
#define CDS_DIDL_ATTRIB_NRAUDIOCHANNELS_ESCAPED_LEN	29

#define CDS_DIDL_ATTRIB_PROTECTION_ESCAPED		" protection=&quot;%s&quot;"
#define CDS_DIDL_ATTRIB_PROTECTION_ESCAPED_LEN	24

#define CDS_DIDL_ATTRIB_PROTECTION_START		" protection=&quot;"
#define CDS_DIDL_ATTRIB_PROTECTION_END			"&quot;"

#define CDS_DIDL_ATTRIB_SAMPLEFREQUENCY_ESCAPED	" sampleFrequency=&quot;%d&quot;"
#define CDS_DIDL_ATTRIB_SAMPLEFREQUENCY_ESCAPED_LEN	29

#define CDS_DIDL_ATTRIB_SIZE_ESCAPED			" size=&quot;%" PRIu64 "&quot;"
#define CDS_DIDL_ATTRIB_SIZE_ESCAPED_LEN		18

#define CDS_DIDL_RES_VALUE_ESCAPED				"&gt;%s&lt;/res&gt;"
#define CDS_DIDL_RES_VALUE_ESCAPED_LEN			16

#define CDS_DIDL_RES_VALUE1_ESCAPED				"&gt;"
#define CDS_DIDL_RES_VALUE2_ESCAPED				"&lt;/res&gt;"

#define CDS_DIDL_CHANNEL_NUM1					"\r\n&lt;upnp:channelNr&gt;"
#define CDS_DIDL_CHANNEL_NUM2					"&lt;/upnp:channelNr&gt;"
#define CDS_DIDL_CHANNEL_NUM_LEN				47


#define CDS_DIDL_CHANNEL_NAME1					"\r\n&lt;upnp:channelName&gt;"
#define CDS_DIDL_CHANNEL_NAME2					"&lt;/upnp:channelName&gt;"
#define CDS_DIDL_CHANNEL_NAME_LEN				51

#define CDS_DIDL_CHANNEL_ID1					"\r\n&lt;upnp:channelID&gt;"
#define CDS_DIDL_CHANNEL_ID2					"&lt;/upnp:channelID&gt;"
#define CDS_DIDL_CHANNEL_ID_LEN					48

#define CDS_DIDL_EPG_PROVIDER1					"\r\n&lt;upnp:epgProviderName&gt;"
#define CDS_DIDL_EPG_PROVIDER2					"&lt;/upnp:epgProviderName&gt;"
#define CDS_DIDL_EPG_PROVIDER_LEN				59

#define CDS_DIDL_SVC_PROVIDER1					"\r\n&lt;upnp:serviceProvider&gt;"
#define CDS_DIDL_SVC_PROVIDER2					"&lt;/upnp:serviceProvider&gt;"
#define CDS_DIDL_SVC_PROVIDER_LEN				59

#define CDS_DIDL_CALL_SIGN1						"\r\n&lt;upnp:callSign&gt;"
#define CDS_DIDL_CALL_SIGN2						"&lt;/upnp:callSign&gt;"
#define CDS_DIDL_CALL_SIGN_LEN					45

#define CDS_DIDL_NETWORK_AFFIL1					"\r\n&lt;upnp:networkAffiliation&gt;"
#define CDS_DIDL_NETWORK_AFFIL2					"&lt;/upnp:networkAffiliation&gt;"
#define CDS_DIDL_NETWORK_AFFIL_LEN				65

#define CDS_DIDL_RECORDABLE1					"\r\n&lt;upnp:recordable&gt;"
#define CDS_DIDL_RECORDABLE2					"&lt;/upnp:recordable&gt;"
#define CDS_DIDL_RECORDABLE_LEN					49

#define CDS_DIDL_DATE_RANGE1					"\r\n&lt;upnp:dateTimeRange&gt;"
#define CDS_DIDL_DATE_RANGE2					"&lt;/upnp:dateTimeRange&gt;"
#define CDS_DIDL_DATE_RANGE_LEN					55

#define CDS_DIDL_START_TIME1					"\r\n&lt;upnp:scheduledStartTime&gt;"
#define CDS_DIDL_START_TIME2					"&lt;/upnp:scheduledStartTime&gt;"
#define CDS_DIDL_START_TIME_LEN					65

#define CDS_DIDL_END_TIME1						"\r\n&lt;upnp:scheduledEndTime&gt;"
#define CDS_DIDL_END_TIME2						"&lt;/upnp:scheduledEndTime&gt;"
#define CDS_DIDL_END_TIME_LEN					61

#define CDS_DIDL_DURATION1						"\r\n&lt;upnp:scheduledDurationTime&gt;"
#define CDS_DIDL_DURATION2						"&lt;/upnp:scheduledDurationTime&gt;"
#define CDS_DIDL_DURATION_LEN					71

#define CDS_DIDL_PROGRAM_ID1					"\r\n&lt;upnp:programID&gt;"
#define CDS_DIDL_PROGRAM_ID2					"&lt;/upnp:programID&gt;"
#define CDS_DIDL_PROGRAM_ID_LEN					47

#define CDS_DIDL_SERIES_ID1						"\r\n&lt;upnp:seriesID&gt;"
#define CDS_DIDL_SERIES_ID2						"&lt;/upnp:seriesID&gt;"
#define CDS_DIDL_SERIES_ID_LEN					55

#define CDS_DIDL_EPISODE_NR1					"\r\n&lt;upnp:episodeNumber&gt;"
#define CDS_DIDL_EPISODE_NR2					"&lt;/upnp:episodeNumber&gt;"
#define CDS_DIDL_EPISODE_NR_LEN					55

#define CDS_DIDL_EPISODE_SEASON1				"\r\n&lt;upnp:episodeSeason&gt;"
#define CDS_DIDL_EPISODE_SEASON2				"&lt;/upnp:episodeSeason&gt;"
#define CDS_DIDL_EPISODE_SEASON_LEN				55

#define CDS_DIDL_EPISODE_TYPE1					"\r\n&lt;upnp:episodeType&gt;"
#define CDS_DIDL_EPISODE_TYPE2					"&lt;/upnp:episodeType&gt;"
#define CDS_DIDL_EPISODE_TYPE_LEN				51

#define CDS_DIDL_LONG_DESCR1					"\r\n&lt;upnp:longDescription&gt;"
#define CDS_DIDL_LONG_DESCR2					"&lt;/upnp:longDescription&gt;"
#define CDS_DIDL_LONG_DESCR_LEN					59

#define CDS_DIDL_DATE1							"\r\n&lt;dc:date&gt;"
#define CDS_DIDL_DATE2							"&lt;/dc:date&gt;"
#define CDS_DIDL_DATE_LEN						33

#define CDS_DIDL_RATING1						"\r\n&lt;upnp:rating&gt;"
#define CDS_DIDL_RATING2						"&lt;/upnp:rating&gt;"
#define CDS_DIDL_RATING_LEN						41

#define CDS_DIDL_RATING_TYPE1					"\r\n&lt;upnp:rating type=\"%s\"&gt;"
#define CDS_DIDL_RATING_TYPE2					"&lt;/upnp:rating&gt;"
#define CDS_DIDL_RATING_TYPE_LEN				41

#define CDS_DIDL_LANGUAGE1						"\r\n&lt;dc:language&gt;"
#define CDS_DIDL_LANGUAGE2						"&lt;/dc:language&gt;"
#define CDS_DIDL_LANGUAGE_LEN					41

#define CDS_DIDL_ACTOR1							"\r\n&lt;upnp:actor&gt;"
#define CDS_DIDL_ACTOR2							"&lt;/upnp:actor&gt;"
#define CDS_DIDL_ACTOR_LEN						39

#define CDS_DIDL_RES_THUMBNAIL					"&lt;upnp:albumArtURI dlna:profileID=&quot;JPEG_TN&quot; xmlns:dlna=&quot;urn:schemas-dlna-org:metadata-1-0/&quot;&gt;"
#define CDS_DIDL_RES_THUMBNAIL2					"&lt;/upnp:albumArtURI&gt;"
#define CDS_DIDL_RES_THUMBNAIL_LEN				142

/********************************************/

#define CDS_FILTER_CHILDCOUNT					"@childCount"
#define CDS_FILTER_CHILDCOUNT_LEN				11

#define CDS_FILTER_CONTAINER_CHILDCOUNT			"container@childCount"
#define CDS_FILTER_CONTAINER_CHILDCOUNT_LEN		20

#define CDS_FILTER_CONTAINER_SEARCHABLE			"container@searchable"
#define CDS_FILTER_CONTAINER_SEARCHABLE_LEN		20

#define CDS_FILTER_CREATOR						"dc:creator"
#define CDS_FILTER_CREATOR_LEN					10

#define CDS_FILTER_ALBUM						"upnp:album"
#define CDS_FILTER_ALBUM_LEN					10

#define CDS_FILTER_GENRE						"upnp:genre"
#define CDS_FILTER_GENRE_LEN					10

#define CDS_FILTER_RES							"res"
#define CDS_FILTER_RES_LEN						3

#define CDS_FILTER_RES_BITRATE					"res@bitrate"
#define CDS_FILTER_RES_BITRATE_LEN				11

#define CDS_FILTER_RES_BITSPERSAMPLE			"res@bitsPerSample"
#define CDS_FILTER_RES_BITSPERSAMPLE_LEN		17

#define CDS_FILTER_RES_COLORDEPTH				"res@colorDepth"
#define CDS_FILTER_RES_COLORDEPTH_LEN			14

#define CDS_FILTER_RES_DURATION					"res@duration"
#define CDS_FILTER_RES_DURATION_LEN				12

#define CDS_FILTER_RES_PROTECTION				"res@protection"
#define CDS_FILTER_RES_PROTECTION_LEN			14

#define CDS_FILTER_RES_RESOLUTION				"res@resolution"
#define CDS_FILTER_RES_RESOLUTION_LEN			14

#define CDS_FILTER_RES_SAMPLEFREQUENCY			"res@sampleFrequency"
#define CDS_FILTER_RES_SAMPLEFREQUENCY_LEN		19

#define CDS_FILTER_RES_NRAUDIOCHANNELS			"res@nrAudioChannels"
#define CDS_FILTER_RES_NRAUDIOCHANNELS_LEN		19

#define CDS_FILTER_RES_SIZE						"res@size"
#define CDS_FILTER_RES_SIZE_LEN					8

#define CDS_FILTER_SEARCHABLE					"@searchable"
#define CDS_FILTER_SEARCHABLE_LEN				11

#define CDS_FILTER_CHANNEL_NUMBER				"upnp:channelNr"
#define CDS_FILTER_CHANNEL_NUMBER_LEN			14

#define CDS_FILTER_CHANNEL_NAME					"upnp:channelName"
#define CDS_FILTER_CHANNEL_NAME_LEN				16

#define CDS_FILTER_EPG_PROVIDER					"upnp:epgProviderName"
#define CDS_FILTER_EPG_PROVIDER_LEN				20

#define CDS_FILTER_SVC_PROVIDER					"upnp:serviceProvider"
#define CDS_FILTER_SVC_PROVIDER_LEN				20

#define CDS_FILTER_CHANNEL_ID					"upnp:channelID"
#define CDS_FILTER_CHANNEL_ID_LEN				14

#define CDS_FILTER_CALL_SIGN					"upnp:callSign"
#define CDS_FILTER_CALL_SIGN_LEN				13

#define CDS_FILTER_NETWORK_AFFIL				"upnp:networkAffiliation"
#define CDS_FILTER_NETWORK_AFFIL_LEN			24

#define CDS_FILTER_RECORDABLE					"upnp:recordable"
#define CDS_FILTER_RECORDABLE_LEN				15

#define CDS_FILTER_DATE_RANGE					"upnp:dateTimeRange"
#define CDS_FILTER_DATE_RANGE_LEN				18

#define CDS_FILTER_START_TIME					"upnp:scheduledStartTime"
#define CDS_FILTER_START_TIME_LEN				23

#define CDS_FILTER_END_TIME						"upnp:scheduledEndTime"
#define CDS_FILTER_END_TIME_LEN					21

#define CDS_FILTER_DURATION						"upnp:scheduledDurationTime"
#define CDS_FILTER_DURATION_LEN					27

#define CDS_FILTER_PROGRAM_ID					"upnp:programID"
#define CDS_FILTER_PROGRAM_ID_LEN				14

#define CDS_FILTER_SERIES_ID					"upnp:seriesID"
#define CDS_FILTER_SERIES_ID_LEN				13

#define CDS_FILTER_EPISODE_NR					"upnp:episodeNumber"
#define CDS_FILTER_EPISODE_NR_LEN				18

#define CDS_FILTER_EPISODE_SEASON				"upnp:episodeSeason"
#define CDS_FILTER_EPISODE_SEASON_LEN			18

#define CDS_FILTER_EPISODE_TYPE					"upnp:episodeType"
#define CDS_FILTER_EPISODE_TYPE_LEN				16

#define CDS_FILTER_LONG_DESCR					"upnp:longDescription"
#define CDS_FILTER_LONG_DESCR_LEN				20

#define CDS_FILTER_DATE							"dc:date"
#define CDS_FILTER_DATE_LEN						7

#define CDS_FILTER_RATING						"upnp:rating"
#define CDS_FILTER_RATING_LEN					11

#define CDS_FILTER_LANGUAGE						"dc:language"
#define CDS_FILTER_LANGUAGE_LEN					11

#define CDS_FILTER_ACTOR						"upnp:actor"
#define CDS_FILTER_ACTOR_LEN					10

#define CDS_FILTER_THUMB						"upnp:albumArtURI"
#define CDS_FILTER_THUMB_LEN					16

#define CDS_FILTER_THUMB2						"upnp:albumArtURI@dlna:profileID"
#define CDS_FILTER_THUMB2_LEN					31

#endif
