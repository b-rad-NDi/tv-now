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

#ifndef __ILibWebServer__
#define __ILibWebServer__

#define ILibWebServer_SEND_RESULTED_IN_DISCONNECT -2

struct ILibWebServer_Session;
typedef void (*ILibWebServer_Session_OnReceive)\
				(struct ILibWebServer_Session *sender,\
					int InterruptFlag,\
					struct packetheader *header,\
					char *bodyBuffer,\
					int *beginPointer,\
					int endPointer,\
					int done);

struct ILibWebServer_Session
{
	ILibWebServer_Session_OnReceive OnReceive;
	void (*OnDisconnect)(struct ILibWebServer_Session *sender);
	void (*OnSendOK)(struct ILibWebServer_Session *sender);
	void *Parent;
	void *User;
	void *User2;

	void *Reserved1;	// AsyncServerSocket
	void *Reserved2;	// ConnectionToken
	void *Reserved3;	// WebClientDataObject
	void *Reserved7;	// VirtualDirectory
	int Reserved4;	// Request Answered Flag (set by send)
	int Reserved8;	// RequestAnswered Method Called
	int Reserved5;	// Request Made Flag
	int Reserved6;	// Close Override Flag
};

typedef void (*ILibWebServer_Session_OnSession)(struct ILibWebServer_Session *SessionToken, void *User);
typedef void (*ILibWebServer_VirtualDirectory)(struct ILibWebServer_Session *session, struct packetheader *header, char *bodyBuffer, int *beginPointer, int endPointer, int done, void *user);

void ILibWebServer_SetTag(void *WebServerToken, void *Tag);
void *ILibWebServer_GetTag(void *WebServerToken);

void *ILibWebServer_Create(void *Chain, int MaxConnections, int PortNumber,ILibWebServer_Session_OnSession OnSession, void *User);
int ILibWebServer_RegisterVirtualDirectory(void *WebServerToken, char *vd, int vdLength, ILibWebServer_VirtualDirectory OnVirtualDirectory, void *user);
int ILibWebServer_UnRegisterVirtualDirectory(void *WebServerToken, char *vd, int vdLength);

int ILibWebServer_Send(struct ILibWebServer_Session *session, struct packetheader *packet);
int ILibWebServer_Send_Raw(struct ILibWebServer_Session *session, char *buffer, int bufferSize, int userFree, int done);

#define ILibWebServer_Session_GetPendingBytesToSend(session) ILibAsyncServerSocket_GetPendingBytesToSend(session->Reserved1,session->Reserved2)
#define ILibWebServer_Session_GetTotalBytesSent(session) ILibAsyncServerSocket_GetTotalBytesSent(session->Reserved1,session->Reserved2)
#define ILibWebServer_Session_ResetTotalBytesSent(session) ILibAsyncServerSocket_ResetTotalBytesSent(session->Reserved1,session->Reserved2)

unsigned short ILibWebServer_GetPortNumber(void *WebServerToken);
int ILibWebServer_GetLocalInterface(struct ILibWebServer_Session *session);
int ILibWebServer_GetRemoteInterface(struct ILibWebServer_Session *session);

int ILibWebServer_StreamHeader(struct ILibWebServer_Session *session, struct packetheader *header);
int ILibWebServer_StreamBody(struct ILibWebServer_Session *session, char *buffer, int bufferSize, int userFree, int done);

int ILibWebServer_StreamHeader_Raw(struct ILibWebServer_Session *session, int StatusCode,char *StatusData,char *ResponseHeaders, int ResponseHeaders_FREE);

#endif
