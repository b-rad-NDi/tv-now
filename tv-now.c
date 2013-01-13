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

/* UPnP MicroStack, Main Module */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "ILibParsers.h"
#include "MicroMediaServer.h"
#include "PortingFunctions.h"
#include "UpnpMicroStack.h"

#include "dvbteeserver.h"

void *TheChain;
void *TheStack;

void *UpnpMonitor;
int UpnpIPAddressLength;
int *UpnpIPAddressList;

/*
 *	Method gets periodically executed on the microstack
 *	thread to update the list of known IP addresses.
 *	This allows the upnp layer to adjust to changes
 *	in the IP address list for the platform.
 *	This applies only if winsock1 is used.
 */
void UpnpIPAddressMonitor(void *data)
{
	int length;
	int *list;

	length = ILibGetLocalIPAddressList(&list);
	if(length!=UpnpIPAddressLength || memcmp((void*)list,(void*)UpnpIPAddressList,sizeof(int)*length)!=0)
	{
		UpnpIPAddressListChanged(TheStack);

		free(UpnpIPAddressList);
		UpnpIPAddressList = list;
		UpnpIPAddressLength = length;
		UpdateIPAddresses(UpnpIPAddressList, UpnpIPAddressLength);
	}
	else
	{
		free(list);
	}

	ILibLifeTime_Add(UpnpMonitor,NULL,4,&UpnpIPAddressMonitor,NULL);
}

void BreakSink(int s)
{
	ILibStopChain(TheChain);
#if NDi_LiveTV
	dvbtee_stop();
	sleep(5);
#endif
}

int main(int argv, char** argc)
{
	char udn[20];
	char friendlyname[100];
	int i;

	/* Randomized udn generation */
	srand((unsigned int)time(NULL));
	for (i=0;i<19;i++)
	{
		udn[i] = (rand() % 25) + 66;
	}
	udn[19] = 0;

	/* get friendly name with hostname */
	memcpy(friendlyname,"TV-Now (",8);
	gethostname(friendlyname+8,68);
	memcpy(friendlyname+strlen(friendlyname),")\0",2);

	/* command line arg processing */
	TheChain = ILibCreateChain();

	TheStack = UpnpCreateMicroStack(TheChain, friendlyname, udn,"1A4001A4",1800,0);

	InitMms(TheChain, TheStack, "./");

	/*
	 *	Set up the app to periodically monitor the available list
	 *	of IP addresses.
	 */
	UpnpMonitor = ILibCreateLifeTime(TheChain);
	UpnpIPAddressLength = ILibGetLocalIPAddressList(&UpnpIPAddressList);
	UpdateIPAddresses(UpnpIPAddressList, UpnpIPAddressLength);
	ILibLifeTime_Add(UpnpMonitor,NULL,4,&UpnpIPAddressMonitor,NULL);

#if NDi_LiveTV
	printf("Starting Transport Stream Engine...\n\t");
	SpawnNormalThread(&dvbtee_start, NULL);
#endif

	/* start UPnP - blocking call*/
	signal(SIGINT,BreakSink);
	ILibStartChain(TheChain);

	return 0;
}
