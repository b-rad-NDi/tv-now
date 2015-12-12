/*****************************************************************************
 * Copyright (C) 2012 Brad Love : b-rad at next dimension dot cc
 *   http://nextdimension.cc
 *   http://b-rad.cc
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
 * You should have received a copy of the GNU General Public License
 * along with TV-Now.  If not, see <http://www.gnu.org/licenses/>.
 *
 * An Apache 2.0 licensed version of this software is privately maintained
 * for licensing to interested commercial parties. Apache 2.0 license is
 * compatible with the GPLv3, which allows the Apache 2.0 version to be
 * included in proprietary systems, but keeping the public GPLv3 version
 * completely open source. Note the GPLv3 license is NOT compatible with
 * the GPLv3, so you cannot release the public GPLv3 version under Apache 2.0.
 * To inquire about licensing the commercial TV-Now contact:
 *   tv-now at nextdimension dot cc
 *
 * Note about contributions and patch submissions:
 *  The commercial Apache 2.0 version of TV-Now is used as the master.
 *  The GPLv3 version of TV-Now will be identical to the Apache 2.0 version.
 *  All contributions and patches are therefore licensed under Apache 2.0
 *  By submitting a patch you implicitly agree to
 *    http://www.apache.org/licenses/icla.txt
 *  You retain ownership and when the patch is merged to the GPLv3 version
 *  the patch license will be upgraded to GPLv3.
 *
 *****************************************************************************/

struct program_info
{
	char event_id[128];
	char title[128];
	char longDesc[1024];
	time_t  start;
	int  duration;
};

void dvbtee_start();
void dvbtee_stop();

void* firstchannel(char* chan_name);
void* nextchannel(void* c_iter, char* chan_name);
const int ischannel(char* channelID);

