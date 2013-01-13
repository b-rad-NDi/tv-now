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

#ifndef ___ILibAsyncSocket___
#define ___ILibAsyncSocket___

#define ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR 2

enum ILibAsyncSocket_MemoryOwnership
{
	ILibAsyncSocket_MemoryOwnership_CHAIN=0,
	ILibAsyncSocket_MemoryOwnership_STATIC=1,
	ILibAsyncSocket_MemoryOwnership_USER=2
};



void* ILibCreateAsyncSocketModule(void *Chain, int initialBufferSize, void(*OnData)(void* socketModule,char* buffer,int *p_beginPointer, int endPointer,void (**InterruptPtr)(void *socketModule, void *user), void **user, int *PAUSE), void(*OnConnect)(void* socketModule, int Connected, void *user),void(*OnDisconnect)(void* socketModule, void *user),void(*OnSendOK)(void *socketModule, void *user));
unsigned int ILibAsyncSocket_GetPendingBytesToSend(void *socketModule);
unsigned int ILibAsyncSocket_GetTotalBytesSent(void *socketModule);
void ILibAsyncSocket_ResetTotalBytesSent(void *socketModule);

void ILibAsyncSocket_ConnectTo(void* socketModule, int localInterface, int remoteInterface, int remotePortNumber,void (*InterruptPtr)(void *socketModule, void *user),void *user);
int ILibAsyncSocket_Send(void* socketModule, char* buffer, int length, enum ILibAsyncSocket_MemoryOwnership UserFree);
void ILibAsyncSocket_Disconnect(void* socketModule);
void ILibAsyncSocket_GetBuffer(void *socketModule, char **buffer, int *BeginPointer, int *EndPointer);

void ILibAsyncSocket_UseThisSocket(void *socketModule,void* TheSocket,void (*InterruptPtr)(void *socketModule, void *user),void *user);
void ILibAsyncSocket_SetRemoteAddress(void *socketModule,int RemoteAddress);

int ILibAsyncSocket_IsFree(void *socketModule);
int ILibAsyncSocket_GetLocalInterface(void *socketModule);
int ILibAsyncSocket_GetRemoteInterface(void *socketModule);


char* ILibGetReceivingInterface(void* ReaderObject);
void ILibAsyncSocket_Resume(void *socketModule);
#endif
