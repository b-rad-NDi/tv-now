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

#include "CdsMediaClass.h"

/*
 *	The relative order of strings within these arrays must correspond to the MMSCP_CLASS_MASK_xxx bitmask mappings.
 *	The first element must always be an empty string, to account for the fact that a sub-bitstring of the 
 *	mediaClass value may evaluate to 0.
 *
 *	All of these strings represent the normative set of media classes defined in UPnP AV.
 *
 *	It should be noted that none of these strings should EVER need to be change for XML escaping.
 */
const char* CDS_CLASS_OBJECT_TYPE[] = {"", "object.item", "object.container"};
const char* CDS_CLASS_MAJOR_TYPE[] = {"", "imageItem", "audioItem", "videoItem", "playlistItem", "textItem", "person", "playlistContainer", "album", "genre", "storageSystem", "storageVolume", "storageFolder", "epgContainer", "epgItem"};
const char* CDS_CLASS_MINOR1_TYPE[] = {"", "photo", "musicTrack", "audioBroadcast", "audioBook", "movie", "videoBroadcast", "musicVideClip", "musicArtist", "musicAlbum", "photoAlbum", "musicGenre", "movieGenre", "videoProgram", "audioProgram"};
const char* CDS_CLASS_MINOR2_TYPE[] = {""};
