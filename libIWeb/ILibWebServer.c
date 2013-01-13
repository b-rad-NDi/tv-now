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

#ifdef WIN32
	#define sem_t HANDLE
	#define sem_init(p_semaphore,y,initialValue) *p_semaphore=CreateSemaphore(NULL,initialValue,FD_SETSIZE,NULL)
	#define sem_destroy(x) (CloseHandle(*x)==0?1:0)
	#define sem_wait(x) WaitForSingleObject(*x,INFINITE)
	#define sem_trywait(x) ((WaitForSingleObject(*x,0)==WAIT_OBJECT_0)?0:1)
	#define sem_post(x) ReleaseSemaphore(*x,1,NULL)
	#define strncasecmp(x,y,z) _strnicmp(x,y,z)
#elif _WIN32_WCE
	#define sem_t HANDLE
	#define sem_init(x,y,z) *x=CreateSemaphore(NULL,z,FD_SETSIZE,NULL)
	#define sem_destroy(x) (CloseHandle(*x)==0?1:0)
	#define sem_wait(x) WaitForSingleObject(*x,INFINITE)
	#define sem_trywait(x) ((WaitForSingleObject(*x,0)==WAIT_OBJECT_0)?0:1)
	#define sem_post(x) ReleaseSemaphore(*x,1,NULL)
	#define strncasecmp(x,y,z) _strnicmp(x,y,z)
#else
	#include <semaphore.h>
#endif

#include <memory.h>
#include <math.h>
#include "ILibParsers.h"
#include "ILibWebServer.h"
#include "ILibAsyncServerSocket.h"
#include "ILibAsyncSocket.h"
#include "ILibWebClient.h"


#define HTTP_SESSION_IDLE_TIMEOUT 3
#define INITIAL_BUFFER_SIZE 2048

struct ILibWebServer_VirDir_Data
{
	void *callback;
	void *user;
};

struct ILibWebServer_StateModule
{
	void (*PreSelect)(void* object,fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);
	void (*PostSelect)(void* object,int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
	void (*Destroy)(void* object);

	void *Chain;
	void *ServerSocket;
	void *LifeTime;
	void *User;
	void *Tag;

	void *VirtualDirectoryTable;

	void (*OnSession)(struct ILibWebServer_Session *SessionToken, void *User);

};
void ILibWebServer_SetTag(void *object, void *Tag)
{
	struct ILibWebServer_StateModule *s = (struct ILibWebServer_StateModule*)object;
	s->Tag = Tag;
}
void *ILibWebServer_GetTag(void *object)
{
	struct ILibWebServer_StateModule *s = (struct ILibWebServer_StateModule*)object;
	return(s->Tag);
}
void ILibWebServer_IdleSink(void *object)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)object;
	if(ILibAsyncSocket_IsFree(session->Reserved2)==0)
	{
		ILibAsyncServerSocket_Disconnect(session->Reserved1,session->Reserved2);
	}
}

void ILibWebServer_Destroy(void *object)
{
	struct ILibWebServer_StateModule *s = (struct ILibWebServer_StateModule*)object;
	void *en;
	void *data;
	char *key;
	int keyLength;

	if(s->VirtualDirectoryTable!=NULL)
	{
		en = ILibHashTree_GetEnumerator(s->VirtualDirectoryTable);
		while(ILibHashTree_MoveNext(en)==0)
		{
			ILibHashTree_GetValue(en,&key,&keyLength,&data);
			free(data);
		}
		ILibHashTree_DestroyEnumerator(en);
		ILibDestroyHashTree(s->VirtualDirectoryTable);
	}
}
void ILibWebServer_OnResponse(void *WebReaderToken,
								int InterruptFlag,
								struct packetheader *header,
								char *bodyBuffer,
								int *beginPointer,
								int endPointer,
								int done,
								void *user1,
								void *user2,
								int *PAUSE)
{
	struct ILibWebServer_Session *ws = (struct ILibWebServer_Session*)user2;
	struct ILibWebServer_StateModule *wsm = (struct ILibWebServer_StateModule*)ws->Parent;
	
	char *tmp;
	int tmpLength;
	struct parser_result *pr;
	int PreSlash=0;

	if(ws->Reserved4!=0 || ws->Reserved5==0)
	{
		// Not Idle anymore
		ws->Reserved4 = 0;
		ws->Reserved5 = 1;
		ws->Reserved8 = 0;
		ILibLifeTime_Remove(((struct ILibWebServer_StateModule*)ws->Parent)->LifeTime,ws);
	}


	//Check Virtual Directory
	if(wsm->VirtualDirectoryTable!=NULL)
	{
		if(ws->Reserved7==NULL)
		{
			pr = ILibParseString(header->DirectiveObj,0,header->DirectiveObjLength,"/",1);
			if(pr->FirstResult->datalength==0)
			{
				// Does not start with '/'
				tmp = pr->FirstResult->NextResult->data;
				tmpLength = pr->FirstResult->NextResult->datalength;
			}
			else
			{
				// Starts with '/'
				tmp = pr->FirstResult->data;
				tmpLength = pr->FirstResult->datalength;
				PreSlash=1;
			}
			ILibDestructParserResults(pr);
			if(ILibHasEntry(wsm->VirtualDirectoryTable,tmp,tmpLength)!=0)
			{
				// Virtual Directory Defined
				header->DirectiveObj = tmp+tmpLength;
				header->DirectiveObjLength -= (tmpLength+PreSlash);
				ws->Reserved7 = ILibGetEntry(wsm->VirtualDirectoryTable,tmp,tmpLength);

				((ILibWebServer_VirtualDirectory)((struct ILibWebServer_VirDir_Data*)ws->Reserved7)->callback)(ws,header,bodyBuffer,beginPointer,endPointer,done,((struct ILibWebServer_VirDir_Data*)ws->Reserved7)->user);
			}
			else if(ws->OnReceive!=NULL)
			{
				ws->OnReceive(ws,InterruptFlag,header,bodyBuffer,beginPointer,endPointer,done);
			}
		}
		else
		{
			((ILibWebServer_VirtualDirectory)((struct ILibWebServer_VirDir_Data*)ws->Reserved7)->callback)(ws,header,bodyBuffer,beginPointer,endPointer,done,((struct ILibWebServer_VirDir_Data*)ws->Reserved7)->user);
		}
	}
	else if(ws->OnReceive!=NULL)
	{
		ws->OnReceive(ws,InterruptFlag,header,bodyBuffer,beginPointer,endPointer,done);
	}


	if(done!=0 && InterruptFlag==0 && header!=NULL && ws->Reserved8==0)
	{
		*PAUSE=1;
	}
}
void ILibWebServer_OnConnect(void *AsyncServerSocketModule, void *ConnectionToken,void **user)
{
	struct ILibWebServer_StateModule *wsm = (struct ILibWebServer_StateModule*)ILibAsyncServerSocket_GetTag(AsyncServerSocketModule);
	struct ILibWebServer_Session *ws = (struct ILibWebServer_Session*)MALLOC(sizeof(struct ILibWebServer_Session));
	
	memset(ws,0,sizeof(struct ILibWebServer_Session));

	ws->Parent = wsm;
	ws->Reserved1 = AsyncServerSocketModule;
	ws->Reserved2 = ConnectionToken;
	ws->Reserved3 = ILibCreateWebClientEx(&ILibWebServer_OnResponse,ConnectionToken,wsm,ws);
	ws->User = wsm->User;
	*user = ws;

	ILibLifeTime_Add(wsm->LifeTime,ws,HTTP_SESSION_IDLE_TIMEOUT,&ILibWebServer_IdleSink,NULL);

	if(wsm->OnSession!=NULL)
	{
		wsm->OnSession(ws,wsm->User);
	}
}
void ILibWebServer_OnDisconnect(void *AsyncServerSocketModule, void *ConnectionToken, void *user)
{
	struct ILibWebServer_Session *ws = (struct ILibWebServer_Session*)user;

	if(ws->Reserved4!=0 || ws->Reserved5==0)
	{
		ILibLifeTime_Remove(((struct ILibWebServer_StateModule*)ws->Parent)->LifeTime,ws);
		ws->Reserved4=0;
	}

	if(ws->OnDisconnect!=NULL)
	{
		ws->OnDisconnect(ws);
	}
	ILibWebClient_DestroyWebClientDataObject(ws->Reserved3);
	FREE(user);
}
void ILibWebServer_OnReceive(void *AsyncServerSocketModule, void *ConnectionToken,char* buffer,int *p_beginPointer, int endPointer,void (**OnInterrupt)(void *AsyncServerSocketMoudle, void *ConnectionToken, void *user), void **user, int *PAUSE)
{
	struct ILibWebServer_Session *ws = (struct ILibWebServer_Session*)(*user);
	ILibWebClient_OnData(ConnectionToken,buffer,p_beginPointer,endPointer,NULL,&(ws->Reserved3),PAUSE);
}
void ILibWebServer_OnInterrupt(void *AsyncServerSocketModule, void *ConnectionToken, void *user)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)user;
	
	ILibWebClient_DestroyWebClientDataObject(session->Reserved3);
}

int ILibWebServer_RequestAnswered(struct ILibWebServer_Session *session)
{
	struct packetheader *hdr = ILibWebClient_GetHeaderFromDataObject(session->Reserved3);
	struct packetheader_field_node *f;
	int PersistentConnection = 0;

	session->Reserved7 = NULL;
	if(session->Reserved8!=0)
	{
		return(0);
	}
	else
	{
		session->Reserved8 = 1;
		f = hdr->FirstField;
	}

	if(session->Reserved6==0)
	{
		if(hdr->VersionLength==3 && memcmp(hdr->Version,"1.0",3)==0)
		{
			// HTTP 1.0 , Check for Keep-Alive token
			while(f!=NULL)
			{
				if(f->FieldLength==9 && strncasecmp(f->Field,"CONNECTION",9)==0)
				{
					if(f->FieldDataLength==10 && strncasecmp(f->FieldData,"KEEP-ALIVE",10)==0)
					{
						PersistentConnection = 1;
						break;
					}
				}
				f = f->NextField;
			}
		}
		else
		{
			// HTTP 1.1+ , Check for CLOSE token
			PersistentConnection = 1;
			while(f!=NULL)
			{
				if(f->FieldLength==9 && strncasecmp(f->Field,"CONNECTION",9)==0)
				{
					if(f->FieldDataLength==5 && strncasecmp(f->FieldData,"CLOSE",5)==0)
					{
						PersistentConnection = 0;
						break;
					}
				}
				f = f->NextField;
			}
		}
	}

	if(PersistentConnection==0)
	{
		ILibAsyncServerSocket_Disconnect(session->Reserved1,session->Reserved2);
	}
	else
	{
		// Add timeout to close socket
		ILibLifeTime_Add(((struct ILibWebServer_StateModule*)session->Parent)->LifeTime,session,HTTP_SESSION_IDLE_TIMEOUT,&ILibWebServer_IdleSink,NULL);
		ILibWebClient_FinishedResponse_Server(session->Reserved3);
		ILibWebClient_Resume(session->Reserved3);
	}
	return(PersistentConnection==0?ILibWebServer_SEND_RESULTED_IN_DISCONNECT:0);
}

void ILibWebServer_OnSendOK(void *AsyncServerSocketModule,void *ConnectionToken, void *user)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)user;
	int flag = 0;

	if(session->Reserved4!=0)
	{
		flag = ILibWebServer_RequestAnswered(session);
	}
	if(session->OnSendOK!=NULL && flag != ILibWebServer_SEND_RESULTED_IN_DISCONNECT)
	{
		session->OnSendOK(session);
	}
}

void *ILibWebServer_Create(void *Chain, int MaxConnections, int PortNumber,ILibWebServer_Session_OnSession OnSession, void *User)
{
	struct ILibWebServer_StateModule *RetVal = (struct ILibWebServer_StateModule*)MALLOC(sizeof(struct ILibWebServer_StateModule));
	
	memset(RetVal,0,sizeof(struct ILibWebServer_StateModule));

	RetVal->Destroy = &ILibWebServer_Destroy;
	RetVal->Chain = Chain;
	RetVal->OnSession = OnSession;
	RetVal->ServerSocket = ILibCreateAsyncServerSocketModule(
		Chain,
		MaxConnections,
		PortNumber,
		INITIAL_BUFFER_SIZE,
		&ILibWebServer_OnConnect,			// OnConnect
		&ILibWebServer_OnDisconnect,		// OnDisconnect
		&ILibWebServer_OnReceive,			// OnReceive
		&ILibWebServer_OnInterrupt,			// OnInterrupt
		&ILibWebServer_OnSendOK				// OnSendOK
		);
	ILibAsyncServerSocket_SetTag(RetVal->ServerSocket,RetVal);
	RetVal->LifeTime = ILibCreateLifeTime(Chain);
	RetVal->User = User;
	ILibAddToChain(Chain,RetVal);

	return(RetVal);
}

unsigned short ILibWebServer_GetPortNumber(void *WebServerToken)
{
	struct ILibWebServer_StateModule *WSM = (struct ILibWebServer_StateModule*) WebServerToken;
	return(ILibAsyncServerSocket_GetPortNumber(WSM->ServerSocket));
}

int ILibWebServer_Send(struct ILibWebServer_Session *session, struct packetheader *packet)
{
	char *buffer;
	int bufferSize;
	int RetVal = 0;

	session->Reserved4=1;
	bufferSize = ILibGetRawPacket(packet,&buffer);

	RetVal = ILibAsyncServerSocket_Send(session->Reserved1,session->Reserved2,buffer,bufferSize,0);
	if(RetVal==0)
	{
		// Completed Send
		RetVal = ILibWebServer_RequestAnswered(session);
	}
	ILibDestructPacket(packet);
	return(RetVal);
}
int ILibWebServer_Send_Raw(struct ILibWebServer_Session *session, char *buffer, int bufferSize, int userFree, int done)
{
	int RetVal=0;
	session->Reserved4 = done;

	RetVal = ILibAsyncServerSocket_Send(session->Reserved1,session->Reserved2,buffer,bufferSize,userFree);
	if(RetVal==0 && done!=0)
	{
		// Completed Send
		RetVal = ILibWebServer_RequestAnswered(session);
	}
	return(RetVal);
}
int ILibWebServer_StreamHeader_Raw(struct ILibWebServer_Session *session, int StatusCode,char *StatusData,char *ResponseHeaders, int ResponseHeaders_FREE)
{
	struct packetheader *hdr = ILibWebClient_GetHeaderFromDataObject(session->Reserved3);
	
	char *buffer;
	int bufferLength;
	int RetVal;

	buffer = (char*)MALLOC(20+strlen(StatusData));
	bufferLength = sprintf(buffer,"HTTP/1.1 %d %s",StatusCode,StatusData);



	RetVal = ILibWebServer_Send_Raw(session,buffer,bufferLength,0,0);
	if(RetVal != ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RetVal != ILibWebServer_SEND_RESULTED_IN_DISCONNECT)
	{
		if(!(hdr->VersionLength==3 && memcmp(hdr->Version,"1.0",3)==0))
		{
			RetVal = ILibWebServer_Send_Raw(session,"\r\nTransfer-Encoding: chunked",28,1,0);
		}
		else
		{
			// Must Close Socket when done
			session->Reserved6=1;
		}
		if(ResponseHeaders!=NULL && RetVal != ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RetVal != ILibWebServer_SEND_RESULTED_IN_DISCONNECT)
		{
			RetVal = ILibWebServer_Send_Raw(session,ResponseHeaders,(int)strlen(ResponseHeaders),ResponseHeaders_FREE,0);
		}
		if(RetVal != ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RetVal != ILibWebServer_SEND_RESULTED_IN_DISCONNECT)
		{
			return(ILibWebServer_Send_Raw(session,"\r\n\r\n",4,1,0));
		}
		else
		{
			return(RetVal);
		}
	}
	return(RetVal);
}
int ILibWebServer_StreamHeader(struct ILibWebServer_Session *session, struct packetheader *header)
{
	struct packetheader *hdr = ILibWebClient_GetHeaderFromDataObject(session->Reserved3);
	struct packetheader_field_node *n = header->FirstField;
	char *buffer;
	int bufferLength;
	int RetVal;
	while(n!=NULL)
	{
		if(n->FieldLength==14 && strncasecmp(n->Field,"Content-Length",14)==0)
		{
			break;
		}
		n = n->NextField;
	}

	if(!(hdr->VersionLength==3 && memcmp(hdr->Version,"1.0",3)==0))
	{
		// Remove Content-Length and Chunk it!
		if(n!=NULL)
		{
			n->Field = "Transfer-Encoding";
			n->FieldLength = 17;
			n->FieldData = "chunked";
			n->FieldDataLength = 7;
		}
		else
		{
			ILibAddHeaderLine(header,"Transfer-Encoding",17,"chunked",7);
		}
	}
	else
	{
		// Check to see if they gave us a Content-Length
		if(n==NULL)
		{
			// Must Close Socket when done
			session->Reserved6=1;
		}
	}
	bufferLength = ILibGetRawPacket(header,&buffer);
	RetVal = ILibWebServer_Send_Raw(session,buffer,bufferLength,0,0);
	ILibDestructPacket(header);
	return(RetVal);
}
int ILibWebServer_StreamBody(struct ILibWebServer_Session *session, char *buffer, int bufferSize, int userFree, int done)
{
	struct packetheader *hdr = ILibWebClient_GetHeaderFromDataObject(session->Reserved3);
	char *hex;
	int hexLen;
	int RetVal=0;

	if(hdr->VersionLength==3 && memcmp(hdr->Version,"1.0",3)==0)
	{
		// 1.0 No Chunk
		if(bufferSize>0)
		{
			RetVal = ILibWebServer_Send_Raw(session,buffer,bufferSize,userFree,done);
		}
		else if(done!=0)
		{
			RetVal = ILibWebServer_RequestAnswered(session);
		}
	}
	else
	{
		// 1.1+ , Chunk
		if(bufferSize>0)
		{
			hex = (char*)MALLOC(16);
			hexLen = sprintf(hex,"%X\r\n",bufferSize);
			RetVal = ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR;

			if(ILibWebServer_Send_Raw(session,hex,hexLen,0,0)!=ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR)
			{
				if(ILibWebServer_Send_Raw(session,buffer,bufferSize,userFree,0)!=ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR)
				{
					RetVal = ILibWebServer_Send_Raw(session,"\r\n",2,1,0);
				}
			}
			
		}
		if(done!=0 && RetVal != ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RetVal != ILibWebServer_SEND_RESULTED_IN_DISCONNECT)
		{
			RetVal = ILibWebServer_Send_Raw(session,"0\r\n\r\n",5,1,1);
		}
	}

	return(RetVal);
}
int ILibWebServer_GetRemoteInterface(struct ILibWebServer_Session *session)
{
	return(ILibAsyncSocket_GetRemoteInterface(session->Reserved2));
}

int ILibWebServer_GetLocalInterface(struct ILibWebServer_Session *session)
{
	return(ILibAsyncSocket_GetLocalInterface(session->Reserved2));
}
int ILibWebServer_RegisterVirtualDirectory(void *WebServerToken, char *vd, int vdLength, ILibWebServer_VirtualDirectory OnVirtualDirectory, void *user)
{
	struct ILibWebServer_VirDir_Data *data;
	struct ILibWebServer_StateModule *s = (struct ILibWebServer_StateModule*)WebServerToken;
	if(s->VirtualDirectoryTable==NULL)
	{
		s->VirtualDirectoryTable = ILibInitHashTree();
	}

	if(ILibHasEntry(s->VirtualDirectoryTable,vd,vdLength)!=0)
	{
		return(1);
	}
	else
	{
		data = (struct ILibWebServer_VirDir_Data*)malloc(sizeof(struct ILibWebServer_VirDir_Data));
		data->callback = (void*)OnVirtualDirectory;
		data->user = user;
		ILibAddEntry(s->VirtualDirectoryTable,vd,vdLength,data);
	}
	return(0);
}
int ILibWebServer_UnRegisterVirtualDirectory(void *WebServerToken, char *vd, int vdLength)
{
	struct ILibWebServer_StateModule *s = (struct ILibWebServer_StateModule*)WebServerToken;
	if(ILibHasEntry(s->VirtualDirectoryTable,vd,vdLength)!=0)
	{
		free(ILibGetEntry(s->VirtualDirectoryTable,vd,vdLength));
		ILibDeleteEntry(s->VirtualDirectoryTable,vd,vdLength);
		return(0);
	}
	else
	{
		return(1);
	}
}
