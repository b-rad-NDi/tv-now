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
	char event_id[128];
	char title[128];
	char description[1024];
	time_t  start;
	int  duration;
};

extern "C" struct dvb_channel
{
	char channelID[32];
	char channelNr[16];
	char callSign[64];
	std::list<struct program_info*> program_list;
};

struct combo_iter
{
	std::list<struct dvb_channel*> *channel_list;
	std::list<struct dvb_channel*>::iterator c_iter;

	std::list<struct program_info*> *program_list;
	std::list<struct program_info*>::iterator p_iter;
};

static std::list<struct dvb_channel*> channel_list;
static std::list<struct dvb_channel*>::iterator channel_iterator;
static int killServer = 0;
static dvbtee_context *context;

void stop_server(struct dvbtee_context* context);

void insert_sorted(std::list<struct dvb_channel*> &channels, dvb_channel *channel)
{
	std::list<dvb_channel*>::iterator it;
	for(it=channels.begin(); it!=channels.end(); ++it)
	{
		if (strcasecmp((*it)->channelNr,channel->channelNr) > 0)
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

void destroy_lists()
{
	std::list<dvb_channel*>::iterator it;
	std::list<program_info*>::iterator it2;

	for(it=channel_list.begin(); it!=channel_list.end(); )
	{
		for(it2=(*it)->program_list.begin(); it2!=(*it)->program_list.end(); )
		{
			it2 = (*it)->program_list.erase(it2);
		}
		it = channel_list.erase(it);
	}
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

	destroy_lists();
}

extern "C" void destroy_iterator(void* iter)
{
	combo_iter* c_iter = (combo_iter*)iter;

	if (c_iter != NULL)
		delete c_iter;
}

extern "C" void* channel_token()
{
	combo_iter *it = new combo_iter;
	return it;
}

extern "C" void* firstchannel(char* chan_name)
{
//	printf("%s()\n", __func__);
	if (channel_list.empty())
		return NULL;

	combo_iter *it = new combo_iter;
	it->c_iter = channel_list.begin();
	it->channel_list = &channel_list;
	it->program_list = NULL;

	if (chan_name != NULL)
	{
		sprintf(chan_name, "%s", (*it->c_iter)->channelID);
	}
	it->c_iter++;
	return it;
}

extern "C" void* nextchannel(void* c_iter, char* chan_name)
{
//	printf("%s()\n", __func__);
	combo_iter *it = (combo_iter*)c_iter;
	if (it == NULL || it->c_iter == it->channel_list->end())
	{
		return NULL;
	}

	if (chan_name != NULL)
	{
		sprintf(chan_name, "%s", (*it->c_iter)->channelID);
	}

	it->c_iter++;
	return it;
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
	sprintf(chanName,"%s","");
	return 0;
}

extern "C" const int channel_number(char* channelID, char* chan_nr) {
	std::list<dvb_channel*>::iterator it;
	for(it=channel_list.begin(); it!=channel_list.end(); ++it)
	{
		if (strcmp((*it)->channelID, channelID) == 0)
		{
			sprintf(chan_nr, "%s", (*it)->channelNr);
			return 1;
		}
	}
	sprintf(chan_nr,"%s","");
	return 0;
}

struct epg_iter
{
	std::list<struct program_info*> *program_list;
	std::list<struct program_info*>::iterator it;
};

extern "C" void* firstEpgDay(const char* channel, char* day_string)
{
	std::list<dvb_channel*>::iterator it;

//	printf("%s( %s )\n", __func__, channel);
	for(it=channel_list.begin(); it!=channel_list.end(); ++it)
	{
		if (strcmp((*it)->channelID, channel) == 0)
		{
			combo_iter *e_iter = new combo_iter;
			e_iter->program_list = &(*it)->program_list;
			for(e_iter->p_iter=e_iter->program_list->begin(); e_iter->p_iter!=e_iter->program_list->end(); e_iter->p_iter++)
			{
//				printf("%s() - %s : %s\n", __func__, (*e_iter->it)->title, ctime(&(*e_iter->it)->start));

				time_t cur_t;
				time(&cur_t);

				if ((*e_iter->p_iter)->start+(*e_iter->p_iter)->duration <= cur_t)
				{
					e_iter->p_iter = e_iter->program_list->erase(e_iter->p_iter);
					continue;
				}
				if (cur_t >= (*e_iter->p_iter)->start)
				{
					struct tm* tmDate = localtime(&(*e_iter->p_iter)->start);
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
	combo_iter *e_iter = (combo_iter*)handle;
	time_t next_day;

	if (e_iter->p_iter == e_iter->program_list->end())
	{
		return NULL;
	}

	time_t cur_t;
	time(&cur_t);
	time_t cur_day = (*e_iter->p_iter)->start;
	struct tm* tm = localtime(&cur_day);
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;
	next_day = mktime(tm);
	next_day += ( 24 * 60 * 60 );

	e_iter->p_iter++;
	while (e_iter->p_iter != e_iter->program_list->end())
	{
//		printf("%s() - %s : %s - %d - %s\n", __func__, (*e_iter->p_iter)->title, ctime(&(*e_iter->p_iter)->start), mktime(tm), ctime(&next_day));
		if ((*e_iter->p_iter)->start+(*e_iter->p_iter)->duration <= cur_t)
		{
			e_iter->p_iter = e_iter->program_list->erase(e_iter->p_iter);
			continue;
		}
		if ((*e_iter->p_iter)->start >= next_day) /* TODO: plus duration? */
		{
			struct tm* tmDate = localtime(&(*e_iter->p_iter)->start);
			if (tmDate != NULL) strftime(&day_string[0], 128, "%m-%d-%Y", tmDate);

			return (void*)e_iter;
		}
		e_iter->p_iter++;
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
			combo_iter *e_iter = new combo_iter;
			e_iter->program_list = &(*it)->program_list;
			for(e_iter->p_iter=(*it)->program_list.begin(); e_iter->p_iter!=(*it)->program_list.end();)
			{
//				printf("%s() - %s : %s\n", __func__, (*e_iter->p_iter)->title, ctime(&(*e_iter->p_iter)->start));

				struct tm tm = { 0 };
				strptime(day_string, "%m-%d-%Y", &tm);
				tm.tm_hour = 0;
				tm.tm_min = 0;
				tm.tm_sec = 0;
				time_t t_time = mktime(&tm);
				time_t t_time2 = t_time + ( 24 * 60 * 60 );

				if ((*e_iter->p_iter)->start+(*e_iter->p_iter)->duration <= cur_t)
				{
					e_iter->p_iter = e_iter->program_list->erase(e_iter->p_iter);
					continue;
				}
				if ((*e_iter->p_iter)->start >= t_time && (*e_iter->p_iter)->start < t_time2)
				{
					sprintf(event_string, "%s", (*e_iter->p_iter)->event_id);
					return (void*)e_iter;
				}
				e_iter->p_iter++;
			}
		}
	}
	return NULL;
}

extern "C" void* nextEpgEvent(void* handle, const char* channel, char* day_string, char* event_string)
{
	combo_iter *e_iter = (combo_iter*)handle;

	char* endPtr;
	time_t next_day;

	time_t cur_t;
	time(&cur_t);

//	printf("%s( %p, %s, %s )\n", __func__, handle, channel, day_string);

	struct tm tm = { 0 };
	strptime(day_string, "%m-%d-%Y", &tm);
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	next_day = mktime(&tm);
	next_day += ( 24 * 60 * 60 );

	e_iter->p_iter++;
	while (e_iter->p_iter != e_iter->program_list->end())
	{
//		printf("%s() - %s : %s - %s\n", __func__, (*e_iter->p_iter)->title, ctime(&(*e_iter->p_iter)->start), ctime(&next_day));

		if ((*e_iter->p_iter)->start+(*e_iter->p_iter)->duration <= cur_t)
		{
			e_iter->p_iter = e_iter->program_list->erase(e_iter->p_iter);
			continue;
		}
		if ((*e_iter->p_iter)->start < next_day)
		{
			sprintf(event_string, "%s", (*e_iter->p_iter)->event_id);
			return (void*)e_iter;
		}
		else
		{
			return NULL;
		}
		e_iter->p_iter++;
	}

	return NULL;
}

/* if *epg_id is NULL return first current item */
/* epg_id is in/out */
extern "C" int get_epg_data_simple(const char* channel, char** epg_id, char **title, char **description, time_t *start_t, time_t *duration_t, time_t *end_t)
{
	std::list<dvb_channel*>::iterator it;
	time_t cur_t;
	time(&cur_t);

//	printf("%s( %s )\n", __func__, channel);

	for(it=channel_list.begin(); it!=channel_list.end(); ++it)
	{
		if (strcmp((*it)->channelID, channel) == 0)
		{
//			printf("%s() - %s\n", __func__, (*it)->channelID);

			std::list<struct program_info*>::iterator it2;

			for(it2=(*it)->program_list.begin(); it2!=(*it)->program_list.end();)
			{
//				printf("%s() - %s : %s\n", __func__, (*it2)->title, ctime(&(*it2)->start));
				if ((*it2)->start+(*it2)->duration <= cur_t)
				{
					it2 = (*it)->program_list.erase(it2);
					continue;
				}
				if ( *epg_id == NULL || strcmp(*epg_id, (*it2)->event_id) == 0 )
				{
//					printf("%s() !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d\n", __func__, __LINE__);
					if (strlen((*it2)->description) >= 1)
					{
						*description = (char*)malloc(strlen((*it2)->description) + 1);
						sprintf(*description, "%s", (*it2)->description);

					}
					if (strlen((*it2)->title) == 0)
					{
						*title = (char*)malloc(6);
						sprintf(*title, "%s", "_____");
					}
					else
					{
						*title = (char*)malloc(strlen((*it2)->title) + 1);
						sprintf(*title, "%s", (*it2)->title);
					}
					*start_t = (*it2)->start;
					*duration_t = (*it2)->duration;
					*end_t = (*it2)->start + (*it2)->duration;

					if (*epg_id == NULL)
					{
						*epg_id = (char*)malloc(strlen((*it2)->event_id) + 1);
						sprintf(*epg_id, "%s", (*it2)->event_id);
					}
					return 0;
				}
				it2++;
			}
		}
	}

	return 1;
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

		if (c->major + c->minor > 1)
		{
			sprintf(tmp->channelNr, "%02d.%02d", c->major, c->minor);
			sprintf(tmp->channelID, "%d.%02d", c->physical_channel, c->program_number);
		}
		else if (c->lcn)
		{
			sprintf(tmp->channelNr, "%d", c->lcn);
			sprintf(tmp->channelID, "%d.%02d", c->physical_channel, c->program_number);
		}
		else
		{
			sprintf(tmp->channelNr, "%02d.%02d", c->physical_channel, c->program_number);
			sprintf(tmp->channelID, "%d.%02d", c->physical_channel, c->program_number);
		}

		sprintf(tmp->callSign, "%s", c->service_name);
		/* right trim the call sign */
		char* is_space = tmp->callSign + strlen(tmp->callSign);
		while (is_space >= tmp->callSign && (*is_space == ' ' || *is_space == '\0'))
		{
			*is_space = '\0';
			is_space--;
		};

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

#if TVNOW_TESTDATA
static time_t testing_offset = 0;
#endif

class server_decode_report : public decode_report
{
public:
	virtual void epg_event(decoded_event_t &e)
	{
//		printf("received event id: %d on channel name: %s, major: %d, minor: %d, physical: %d, service id: %d, title: %s, desc: %s, start time (time_t) %ld, duration (sec) %d\n",
//		        e.event_id, e.channel_name.c_str(), e.chan_major, e.chan_minor, e.chan_physical, e.chan_svc_id, e.name.c_str(), e.text.c_str(), e.start_time, e.length_sec);

		char channelID[16];

		sprintf(channelID, "%d.%02d", e.chan_physical, e.chan_svc_id);

		std::list<dvb_channel*>::iterator it;
		for(it=channel_list.begin(); it!=channel_list.end(); ++it)
		{
			if (strcmp((*it)->channelID, channelID) == 0) {
				struct program_info* tmp;
				tmp = new program_info;
				if (tmp == NULL)
					return;

#if TVNOW_TESTDATA
				if (testing_offset == 0)
				{
					time(&testing_offset);
					testing_offset = testing_offset - e.start_time - (60 * 60);
				}
				tmp->start = testing_offset + e.start_time;
#else
				tmp->start = e.start_time;
#endif
				tmp->title[0] = '\0';
				tmp->description[0] = '\0';
				tmp->duration = e.length_sec;
				snprintf(tmp->title, sizeof(tmp->title), "%s", e.name.c_str());
				snprintf(tmp->description, sizeof(tmp->description), "%s", e.text.c_str());
				snprintf(tmp->event_id, sizeof(tmp->event_id), "%d", e.event_id);
				insert_sorted_epg((*it)->program_list, tmp);
			}
		}
	}
	virtual void epg_header_footer(bool header, bool channel) {}
	virtual void print(const char *, ...) {}
};

#if TVNOW_TESTDATA
#include "testdata.cpp"
#endif

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
#if TVNOW_TESTDATA
	load_test_data();
#else

#if 1 /* FIXME */
	ATSCMultipleStringsInit();
#endif
	context->tuner.feeder.parser.limit_eit(-1);
//	enum output_options oopt = OUTPUT_PSIP;
//	context->tuner.feeder.parser.out.set_options(oopt);

	start_server(context, scan_flags, 62080, 62081);
	context->tuner.scan_for_services(scan_flags, 0, 0, 1);

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
#endif

}
