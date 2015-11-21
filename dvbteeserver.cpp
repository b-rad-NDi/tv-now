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
#define LINUXTV 1
#if LINUXTV
#include "linuxtv_tuner.h"
#else
#include "hdhr_tuner.h"
#endif
#include "serve.h"

#include "atsctext.h"

struct dvbtee_context
{
#if LINUXTV
	linuxtv_tuner tuner;
#else
	hdhr_tuner tuner;
#endif
	serve *server;
};

extern "C" struct program_info
{
	char title[128];
	char description[1024];
	time_t  start;
	int  duration;
};

extern "C" struct dvb_channel
{
	char channelID[32];
	char callSign[64];
	std::list<struct program_info*> program_list;
};

static std::list<struct dvb_channel*> channel_list;
static std::list<struct dvb_channel*>::iterator channel_iterator;
static int noChannel = 0;
static int killServer = 0;
static dvbtee_context *context;

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

void insert_sorted_epg(std::list<struct program_info*> &programs, program_info *program)
{
	std::list<program_info*>::iterator it;
	time_t cur_t;
	time(&cur_t);

	for(it=programs.begin(); it!=programs.end(); )
	{
		if ((*it)->start+(*it)->duration <= cur_t)
		{
			it = programs.erase(it);
			continue;
		}
		if ((*it)->start == program->start)
		{
			it = programs.erase(it);
			continue;
		}
		if ((*it)->start > program->start)
		{
			programs.insert(it,program);
			return;
		}
		++it;
	}
	programs.push_back(program);
}

void cleanup(struct dvbtee_context* context, bool quick = false)
{
	if (context->server)
		stop_server(context);

	if (quick) {
		context->tuner.feeder.stop_without_wait();
		context->tuner.feeder.close_file();
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

	context->server->set_scan_flags(&context->tuner, flags);

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

extern "C" void* firstEpgDay(const char* channel, char* day_string)
{
	std::list<dvb_channel*>::iterator it;

//	printf("%s( %s )\n", __func__, channel);
	for(it=channel_list.begin(); it!=channel_list.end(); ++it)
	{
		if (strcmp((*it)->channelID, channel) == 0)
		{
			epg_iter *e_iter = new epg_iter;
			e_iter->program_list = &(*it)->program_list;
			for(e_iter->it=e_iter->program_list->begin(); e_iter->it!=e_iter->program_list->end(); e_iter->it++)
			{
//				printf("%s() - %s : %s\n", __func__, (*e_iter->it)->title, ctime(&(*e_iter->it)->start));

				time_t cur_t;
				time(&cur_t);

				if ((*e_iter->it)->start+(*e_iter->it)->duration <= cur_t)
				{
					e_iter->it = e_iter->program_list->erase(e_iter->it);
					continue;
				}
				if (cur_t >= (*e_iter->it)->start)
				{
					struct tm* tmDate = localtime(&(*e_iter->it)->start);
					if (tmDate != NULL) strftime(&day_string[0], 128, "%m-%d-%Y", tmDate);
					return (void*)e_iter;
				}
			}
			delete e_iter;
		}
	}
	return NULL;
}

extern "C" void* nextEpgDay(void* handle, const char* channel, char* day_string)
{
	epg_iter *e_iter = (epg_iter*)handle;

	if (e_iter->it == e_iter->program_list->end())
	{
		return NULL;
	}

	time_t cur_t;
	time(&cur_t);
	time_t cur_day = (*e_iter->it)->start;
	struct tm* tm = localtime(&cur_day);
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;
	next_day = mktime(tm);
	next_day += 24 * 60 * 60;

	e_iter->it++;
	while (e_iter->it != e_iter->program_list->end())
	{
//		printf("%s() - %s : %s - %d - %s\n", __func__, (*e_iter->it)->title, ctime(&(*e_iter->it)->start), mktime(tm), ctime(&next_day));
		if ((*e_iter->it)->start+(*e_iter->it)->duration <= cur_t)
		{
			e_iter->it = e_iter->program_list->erase(e_iter->it);
			continue;
		}
		if ((*e_iter->it)->start >= next_day) /* TODO: plus duration? */
		{
			struct tm* tmDate = localtime(&(*e_iter->it)->start);
			if (tmDate != NULL) strftime(&day_string[0], 128, "%m-%d-%Y", tmDate);

			return (void*)e_iter;
		}
		e_iter->it++;
	}
	return NULL;
}

extern "C" void* firstEpgEvent(const char* channel, char* day_string, char* event_string)
{
	std::list<dvb_channel*>::iterator it;
	time_t cur_t;
	time(&cur_t);

//	printf("%s( %s, %s )\n", __func__, channel, day_string);

	for(it=channel_list.begin(); it!=channel_list.end(); ++it)
	{
		if (strcmp((*it)->channelID, channel) == 0)
		{
			print_epg((*it));
			epg_iter *e_iter = new epg_iter;
			e_iter->program_list = &(*it)->program_list;
			for(e_iter->it=(*it)->program_list.begin(); e_iter->it!=(*it)->program_list.end();)
			{
//				printf("%s() - %s : %s\n", __func__, (*e_iter->it)->title, ctime(&(*e_iter->it)->start));

				struct tm tm = { 0 };
				strptime(day_string, "%m-%d-%Y", &tm);
				tm.tm_hour = 0;
				tm.tm_min = 0;
				tm.tm_sec = 0;
				time_t t_time = mktime(&tm);
				t_time += 86400;

	return NULL;
}

extern "C" void* nextEpgEvent(void* handle, const char* channel, char* day_string, char* event_string)
{
	return NULL;
}

class server_parse_iface : public parse_iface
{
public:
	server_parse_iface() {}

	virtual void chandump(parsed_channel_info_t *c)
	{
		char channelno[16]; /* XXX.XXX */
		if (c->major + c->minor > 1)
			sprintf(channelno, "%02d.%02d", c->major, c->minor);
		else if (c->lcn)
			sprintf(channelno, "%d", c->lcn);
		else
			sprintf(channelno, "%02d.%02d", c->physical_channel, c->program_number);

		struct dvb_channel* tmp;
		tmp = new dvb_channel;
		if (tmp == NULL)
			return;

//		sprintf(tmp->channelID, "%d%02d", c->physical_channel, c->program_number);
		sprintf(tmp->channelID, "%d%02d", c->major, c->minor);
		sprintf(tmp->callSign, "%s - %s", channelno, c->service_name);

		insert_sorted(channel_list, tmp);

		return;
	}
};

bool list_channels(serve *server)
{
	if (!server)
		return false;

	server_parse_iface iface;

	return server->get_channels(&iface);
}

bool start_async_channel_scan(serve *server, unsigned int flags = 0)
{
	server->scan(flags);
}

bool channel_scan_and_dump(serve *server, unsigned int flags = 0)
{
	server_parse_iface iface;

	server->scan(flags, &iface);
}

class server_decode_report : public decode_report
{
public:
	virtual void epg_event(decoded_event_t &e)
	{
		printf("received event id: %d on channel name: %s, major: %d, minor: %d, physical: %d, service id: %d, title: %s, desc: %s, start time (time_t) %ld, duration (sec) %d\n",
		        e.event_id, e.channel_name.c_str(), e.chan_major, e.chan_minor, e.chan_physical, e.chan_svc_id, e.name.c_str(), e.text.c_str(), e.start_time, e.length_sec);

		char channelno[16];
//		sprintf(channelno, "%d%02d", e.chan_physical, e.chan_svc_id);
		sprintf(channelno, "%d%02d", e.chan_major, e.chan_minor);

		std::list<dvb_channel*>::iterator it;
		for(it=channel_list.begin(); it!=channel_list.end(); ++it)
		{
			if (strcmp((*it)->channelID, channelno) == 0) {
				struct program_info* tmp;
				tmp = new program_info;
				if (tmp == NULL)
					return;

				tmp->start = e.start_time;
				tmp->duration = e.length_sec;
				snprintf(tmp->title, sizeof(tmp->title), "%s", e.name.c_str());
				snprintf(tmp->description, sizeof(tmp->description), "%s", e.text.c_str());

				insert_sorted_epg((*it)->program_list, tmp);
			}
		}
	}
	virtual void epg_header_footer(bool header, bool channel) {}
	virtual void print(const char *, ...) {}
};
extern "C" void dvbtee_start(void* nothing)
{
	int opt;

	dvbtee_context tmpContext;
	context = &tmpContext;
	context->server = NULL;

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
	context->tuner.feeder.parser.limit_eit(-1);

	start_server(context, scan_flags, 62080, 62081);
#if 0
	channel_scan_and_dump(context->server, scan_flags);
#else
	list_channels(context->server);
#endif

	server_decode_report reporter;
	context->server->get_epg(&reporter);

	if (context->server) {
		while (context->server->is_running() && killServer != 1) sleep(1);
		stop_server(context);
	}
//	cleanup(&context);
#if 1 /* FIXME */
	ATSCMultipleStringsDeInit();
#endif

}
