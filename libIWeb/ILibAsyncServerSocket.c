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

#ifdef _WIN32_WCE
	#define _CRTDBG_MAP_ALLOC
	#include <math.h>
	#include <winerror.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <stddef.h>
	#include <string.h>
	#include <winsock.h>
	#include <wininet.h>
	#include <windows.h>
	#include <winioctl.h>
	#include <winbase.h>
#elif WIN32
	#define _CRTDBG_MAP_ALLOC
	#include <math.h>
	#include <winerror.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <stddef.h>
	#include <string.h>
	#ifdef WINSOCK2
		#include <winsock2.h>
		#include <ws2tcpip.h>
	#else
		#include <winsock.h>
		#include <wininet.h>
	#endif
	#include <windows.h>
	#include <winioctl.h>
	#include <winbase.h>
	#include <crtdbg.h>
#elif _POSIX
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/time.h>
	#include <netdb.h>
	#include <string.h>
	#include <sys/ioctl.h>
	#include <net/if.h>
	#include <sys/utsname.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <errno.h>
#endif

#include "ILibAsyncServerSocket.h"
#include "ILibAsyncSocket.h"
#include "ILibParsers.h"

#ifdef _WIN32_WCE
#define strncasecmp(x,y,z) _strnicmp(x,y,z)
#define gettimeofday(x,y) (x)->tv_sec = GetTickCount()/1000
#define sem_t HANDLE
#define sem_init(x,pshared,pvalue) *x=CreateSemaphore(NULL,pvalue,FD_SETSIZE,NULL)
#define sem_destroy(x) (CloseHandle(*x)==0?1:0)
#define sem_wait(x) WaitForSingleObject(*x,INFINITE)
#define sem_trywait(x) ((WaitForSingleObject(*x,0)==WAIT_OBJECT_0)?0:1)
#define sem_post(x) ReleaseSemaphore(*x,1,NULL)

#elif WIN32
#include <errno.h>
#define strncasecmp(x,y,z) _strnicmp(x,y,z)
#define gettimeofday(x,y) (x)->tv_sec = GetTickCount()/1000
#define sem_t HANDLE
#define sem_init(x,y,z) *x=CreateSemaphore(NULL,z,FD_SETSIZE,NULL)
#define sem_destroy(x) (CloseHandle(*x)==0?1:0)
#define sem_wait(x) WaitForSingleObject(*x,INFINITE)
#define sem_trywait(x) ((WaitForSingleObject(*x,0)==WAIT_OBJECT_0)?0:1)
#define sem_post(x) ReleaseSemaphore(*x,1,NULL)

#elif _POSIX
#include <errno.h>
#include <semaphore.h>
#endif


#define DEBUGSTATEMENT(x)


struct AsyncServerSocketModule
{
	void (*PreSelect)(void* object,fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);
	void (*PostSelect)(void* object,int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
	void (*Destroy)(void* object);
	void *Chain;

	int MaxConnection;
	void **AsyncSockets;
	
	#ifdef _WIN32_WCE
		SOCKET ListenSocket;
	#elif WIN32
		SOCKET ListenSocket;
	#elif _POSIX
		int ListenSocket;
	#endif
	unsigned short portNumber;
	int listening;

	void (*OnReceive)(void *AsyncServerSocketModule, void *ConnectionToken,char* buffer,int *p_beginPointer, int endPointer, void (**OnInterrupt)(void *AsyncServerSocketMoudle, void *ConnectionToken, void *user),void **user, int *PAUSE);
	void (*OnConnect)(void *AsyncServerSocketModule, void *ConnectionToken,void **user);
	void (*OnDisconnect)(void *AsyncServerSocketModule, void *ConnectionToken, void *user);
	void (*OnInterrupt)(void *AsyncServerSocketModule, void *ConnectionToken, void *user);
	void (*OnSendOK)(void *AsyncServerSocketModule, void *ConnectionToken, void *user);

	void *Tag;
};
struct AsyncServerSocket_Data
{
	struct AsyncServerSocketModule *module;
	void *user;
};

void *ILibAsyncServerSocket_GetTag(void *AsyncSocketModule)
{
	struct AsyncServerSocketModule *module = (struct AsyncServerSocketModule*)AsyncSocketModule;
	return(module->Tag);
}
void ILibAsyncServerSocket_SetTag(void *AsyncSocketModule, void *tag)
{
	struct AsyncServerSocketModule *module = (struct AsyncServerSocketModule*)AsyncSocketModule;
	module->Tag = tag;
}


void ILibAsyncServerSocket_OnInterrupt(void *socketModule, void *user)
{
	struct AsyncServerSocket_Data *data = (struct AsyncServerSocket_Data*)user;
	if(data->module->OnInterrupt!=NULL)
	{
		data->module->OnInterrupt(data->module,socketModule,data->user);
	}
	FREE(user);
}
void ILibAsyncServerSocket_PreSelect(void* socketModule,fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime)
{
	struct AsyncServerSocketModule *module = (struct AsyncServerSocketModule*)socketModule;
	int flags,i;

	if(module->listening==0)
	{
		#ifdef _WIN32_WCE
			flags = 1;
			ioctlsocket(module->ListenSocket,FIONBIO,&flags);
		#elif WIN32
			flags = 1;
			ioctlsocket(module->ListenSocket,FIONBIO,&flags);
		#elif _POSIX
			flags = fcntl(module->ListenSocket,F_GETFL,0);
			fcntl(module->ListenSocket,F_SETFL,O_NONBLOCK|flags);
		#endif
		
		module->listening=1;
		listen(module->ListenSocket,4);
		FD_SET(module->ListenSocket,readset);
	}
	else
	{
		// Only put the ListenSocket in the readset, if we are able to handle a new socket
		for(i=0;i<module->MaxConnection;++i)
		{
			if(ILibAsyncSocket_IsFree(module->AsyncSockets[i])!=0)
			{
				FD_SET(module->ListenSocket,readset);
				break;
			}
		}
	}
}
void ILibAsyncServerSocket_PostSelect(void* socketModule,int slct, fd_set *readset, fd_set *writeset, fd_set *errorset)
{
	struct AsyncServerSocket_Data *data;
	struct sockaddr_in addr;
	int addrlen;
	struct AsyncServerSocketModule *module = (struct AsyncServerSocketModule*)socketModule;
	int i,flags;
	#ifdef _WIN32_WCE
		SOCKET NewSocket;
	#elif WIN32
		SOCKET NewSocket;
	#elif _POSIX
		int NewSocket;
	#endif

	if(FD_ISSET(module->ListenSocket,readset)!=0)
	{
		for(i=0;i<module->MaxConnection;++i)
		{
			if(ILibAsyncSocket_IsFree(module->AsyncSockets[i])!=0)
			{
				addrlen = sizeof(addr);
				NewSocket = accept(module->ListenSocket,(struct sockaddr*)&addr,&addrlen);
				if (NewSocket != ~0)
				{
					#ifdef _WIN32_WCE
						flags = 1;
						ioctlsocket(NewSocket,FIONBIO,&flags);
					#elif WIN32
						flags = 1;
						ioctlsocket(NewSocket,FIONBIO,&flags);
					#elif _POSIX
						flags = fcntl(NewSocket,F_GETFL,0);
						fcntl(NewSocket,F_SETFL,O_NONBLOCK|flags);
					#endif
					data = (struct AsyncServerSocket_Data*)MALLOC(sizeof(struct AsyncServerSocket_Data));
					data->module = socketModule;
					data->user = NULL;

					ILibAsyncSocket_UseThisSocket(module->AsyncSockets[i],&NewSocket,&ILibAsyncServerSocket_OnInterrupt,data);
					ILibAsyncSocket_SetRemoteAddress(module->AsyncSockets[i],addr.sin_addr.s_addr);

					if(module->OnConnect!=NULL)
					{
						module->OnConnect(module,module->AsyncSockets[i],&(data->user));
					}
				}
				else {break;}
			}
		}
	}
}
void ILibAsyncServerSocket_Destroy(void *socketModule)
{
	struct AsyncServerSocketModule *module =(struct AsyncServerSocketModule*)socketModule;

	FREE(module->AsyncSockets);
	#ifdef _WIN32_WCE
		closesocket(module->ListenSocket);
	#elif WIN32
		closesocket(module->ListenSocket);
	#elif _POSIX
		close(module->ListenSocket);
	#endif	
}
void ILibAsyncServerSocket_OnData(void* socketModule,char* buffer,int *p_beginPointer, int endPointer,void (**OnInterrupt)(void *AsyncSocketMoudle, void *user),void **user, int *PAUSE)
{
	struct AsyncServerSocket_Data *data = (struct AsyncServerSocket_Data*)(*user);
	int bpointer = *p_beginPointer;

	if(data->module->OnReceive!=NULL)
	{
		data->module->OnReceive(data->module,socketModule,buffer,&bpointer,endPointer,&(data->module->OnInterrupt),&(data->user),PAUSE);
		*p_beginPointer = bpointer;
	}
}

void ILibAsyncServerSocket_OnDisconnect(void* socketModule, void *user)
{
	struct AsyncServerSocket_Data *data = (struct AsyncServerSocket_Data*)user;

	if(data->module->OnDisconnect!=NULL)
	{
		data->module->OnDisconnect(data->module,socketModule,data->user);
	}
	if(ILibIsChainBeingDestroyed(data->module->Chain)==0)
	{
		FREE(data);
	}
}
void ILibAsyncServerSocket_OnSendOK(void* socketModule, void *user)
{
	struct AsyncServerSocket_Data *data = (struct AsyncServerSocket_Data*)user;

	if(data->module->OnSendOK!=NULL)
	{
		data->module->OnSendOK(data->module,socketModule,data->user);
	}
}

void *ILibCreateAsyncServerSocketModule(void *Chain, int MaxConnections, int PortNumber, int initialBufferSize, void (*OnConnect)(void *AsyncServerSocketModule, void *ConnectionToken,void **user),void (*OnDisconnect)(void *AsyncServerSocketModule, void *ConnectionToken, void *user),void (*OnReceive)(void *AsyncServerSocketModule, void *ConnectionToken,char* buffer,int *p_beginPointer, int endPointer,void (**OnInterrupt)(void *AsyncServerSocketMoudle, void *ConnectionToken, void *user), void **user, int *PAUSE),void (*OnInterrupt)(void *AsyncServerSocketModule, void *ConnectionToken, void *user), void (*OnSendOK)(void *AsyncServerSocketModule, void *ConnectionToken, void *user))
{
	struct AsyncServerSocketModule *RetVal;
	int i;

	RetVal = (struct AsyncServerSocketModule*)MALLOC(sizeof(struct AsyncServerSocketModule));
	RetVal->PreSelect = &ILibAsyncServerSocket_PreSelect;
	RetVal->PostSelect = &ILibAsyncServerSocket_PostSelect;
	RetVal->Destroy = &ILibAsyncServerSocket_Destroy;
	RetVal->Chain = Chain;
	RetVal->OnConnect = OnConnect;
	RetVal->OnDisconnect = OnDisconnect;
	RetVal->OnInterrupt = OnInterrupt;
	RetVal->OnSendOK = OnSendOK;
	RetVal->OnReceive = OnReceive;
	RetVal->MaxConnection = MaxConnections;
	RetVal->AsyncSockets = (void**)MALLOC(MaxConnections*sizeof(void*));
	RetVal->portNumber = PortNumber;
	RetVal->Tag = NULL;
	RetVal->listening = 0;
	
	for(i=0;i<MaxConnections;++i)
	{
		RetVal->AsyncSockets[i] = ILibCreateAsyncSocketModule(Chain,initialBufferSize,&ILibAsyncServerSocket_OnData,NULL,&ILibAsyncServerSocket_OnDisconnect, &ILibAsyncServerSocket_OnSendOK);
		//RetVal->AsyncSockets[i] = ILibCreateAsyncSocketModule(Chain,initialBufferSize,&ILibAsyncServerSocket_OnData,NULL,&ILibAsyncServerSocket_OnDisconnect, &ILibAsyncServerSocket_OnSendOK);
	}
	ILibAddToChain(Chain,RetVal);

	#ifdef WINSOCK2
		RetVal->portNumber = ILibGetStreamSocket(htonl(INADDR_ANY),RetVal->portNumber,(HANDLE*)&(RetVal->ListenSocket));
	#else
		RetVal->portNumber = ILibGetStreamSocket(htonl(INADDR_ANY),RetVal->portNumber,&(RetVal->ListenSocket));
	#endif

	return(RetVal);
}

unsigned short ILibAsyncServerSocket_GetPortNumber(void *ServerSocketModule)
{
	struct AsyncServerSocketModule *ASSM = (struct AsyncServerSocketModule*)ServerSocketModule;
	return(ASSM->portNumber);
}
