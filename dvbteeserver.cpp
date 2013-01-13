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

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <list>

#include "feed.h"
#include "tune.h"
#include "serve.h"

#include "atsctext.h"

struct dvbtee_context
{
	tune tuner;
	serve *server;
};

extern "C" struct program_info
{
	char title[128];
	char longDesc[1024];
	int  start;
	int  end;
};

extern "C" struct dvb_channel
{
	char channelID[32];
	char callSign[64];
	program_info now;
	program_info next;
};

static std::list<struct dvb_channel*> channel_list;
static std::list<struct dvb_channel*>::iterator channel_iterator;
static int noChannel = 0;
static int killServer = 0;
static dvbtee_context context;

void stop_server(struct dvbtee_context* context);


void insert_sorted(std::list<struct dvb_channel*> &channels, dvb_channel *channel)
{
	std::list<dvb_channel*>::iterator it;
	for(it=channels.begin(); it!=channels.end(); ++it)
	{
		if (strcasecmp((*it)->callSign,channel->callSign) > 0)
		{
			channels.insert(it,channel);
			return;
		}
	}
	channels.push_back(channel);
}

void cleanup(struct dvbtee_context* context, bool quick = false)
{
	if (context->server)
		stop_server(context);

	if (quick) {
		context->tuner.feeder.stop_without_wait();
		context->tuner.feeder.close_file();
		context->tuner.close_demux();
	} else {
		context->tuner.stop_feed();
	}

	context->tuner.close_fe();
#if 1 /* FIXME */
	ATSCMultipleStringsDeInit();
#endif
}


void stop_server(struct dvbtee_context* context)
{
	if (!context->server)
		return;

	context->server->stop();

	delete context->server;
	context->server = NULL;

	return;
}

int start_server(struct dvbtee_context* context, unsigned int flags, int port, int eavesdropping_port = 0)
{
	context->server = new serve;
	context->server->add_tuner(&context->tuner);

	if (eavesdropping_port)
		context->tuner.feeder.parser.out.add_http_server(eavesdropping_port);

	context->server->set_scan_flags(0, flags);

	return context->server->start(port);
}

extern "C" void dvbtee_stop()
{
	killServer = 1;
	sleep(3);
}

extern "C" const dvb_channel* firstchannel() {
	printf("%s()\n", __func__);
	if (channel_list.empty())
		return NULL;

	printf("%s() channel_list ! empty\n", __func__);
	noChannel = 1;
	channel_iterator = channel_list.begin();
	return *channel_iterator;
}

extern "C" const dvb_channel* nextchannel() {
	if (noChannel == 0 || channel_list.empty() || ++channel_iterator == channel_list.end())
		return NULL;

	printf("%s()\n", __func__);

	return *channel_iterator;
}

extern "C" const int ischannel(char* channelID) {
	std::list<dvb_channel*>::iterator it;
	for(it=channel_list.begin(); it!=channel_list.end(); ++it)
	{
		if (strcmp((*it)->channelID, channelID) == 0)
			return 1;
	}
	return 0;
}

extern "C" const int channel_name(char* channelID, char* chanName) {
	std::list<dvb_channel*>::iterator it;
	for(it=channel_list.begin(); it!=channel_list.end(); ++it)
	{
		if (strcmp((*it)->channelID, channelID) == 0)
		{
			sprintf(chanName, "%s", (*it)->callSign);
			return 1;
		}
	}
	sprintf(chanName,"");
	return 0;
}

const char* chandump(void *context, parsed_channel_info_t *c)
{
	char channelno[16]; /* XXX.XXX */
	if (c->major + c->minor > 1)
		sprintf(channelno, "%02d.%02d", c->major, c->minor);
	else if (c->lcn)
		sprintf(channelno, "%d", c->lcn);
	else
		sprintf(channelno, "%d", c->physical_channel);

	struct dvb_channel* tmp;
	tmp = new dvb_channel;
	if (tmp == NULL)
		return NULL;

	sprintf(tmp->channelID, "%d~%d", c->physical_channel, c->program_number);
	sprintf(tmp->callSign, "%s - %s", channelno, c->service_name);

	insert_sorted(channel_list, tmp);

	/* xine format */
/*
	fprintf(stdout, "%s-%s:%d:%s:%d:%d:%d\n",
	        channelno,
	        c->service_name,
	        c->freq,
	        c->modulation,
	        c->vpid, c->apid, c->program_number);
*/
	return NULL;
}

bool list_channels(serve *server)
{
	if (!server)
		return false;

	return server->get_channels(chandump, NULL);
}

bool start_async_channel_scan(serve *server, unsigned int flags = 0)
{
	server->scan(flags);
}

bool channel_scan_and_dump(serve *server, unsigned int flags = 0)
{
	server->scan(flags, chandump, NULL);
}

void epg_callback(void *context, decoded_event_t *e)
{
	printf("received event id: %d on channel name: %s, major: %d, minor: %d, physical: %d, service id: %d, title: %s, desc: %s, start time (time_t) %ld, duration (sec) %d\n",
	        e->event_id, e->channel_name, e->chan_major, e->chan_minor, e->chan_physical, e->chan_svc_id, e->name, e->text, e->start_time, e->length_sec);
}
extern "C" void dvbtee_start(void* nothing)
{
	int opt;

	context.server = NULL;

	/* LinuxDVB context: */
	int dvb_adap = 0; /* ID X, /dev/dvb/adapterX/ */
	int demux_id = 0; /* ID Y, /dev/dvb/adapterX/demuxY */
	int dvr_id   = 0; /* ID Y, /dev/dvb/adapterX/dvrY */
	int fe_id    = 0; /* ID Y, /dev/dvb/adapterX/frontendY */

	unsigned int serv_flags  = 0;
	unsigned int scan_flags  = 0;

#if 1 /* FIXME */
	ATSCMultipleStringsInit();
#endif
	context.tuner.set_device_ids(dvb_adap, fe_id, demux_id, dvr_id, false);
	context.tuner.feeder.parser.limit_eit(-1);

	start_server(&context, scan_flags, 62080, 62081);
#if 0
	channel_scan_and_dump(context.server, scan_flags);
#else
	list_channels(context.server);
#endif

	context.server->get_epg(NULL, epg_callback, NULL);

	if (context.server) {
		while (context.server->is_running() && killServer != 1) sleep(1);
		stop_server(&context);
	}
//	cleanup(&context);
#if 1 /* FIXME */
	ATSCMultipleStringsDeInit();
#endif

}
