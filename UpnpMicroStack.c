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
#include <semaphore.h>

#include "ILibParsers.h"
#include "UpnpMicroStack.h"
#include "ILibWebServer.h"
#include "ILibWebClient.h"
#include "ILibAsyncSocket.h"
#include "PortingFunctions.h"
#include "version.h"

#define UPNP_HTTP_MAXSOCKETS 5


#define UPNP_PORT 1900
#define UPNP_GROUP "239.255.255.250"
#define UpnpMIN(a,b) (((a)<(b))?(a):(b))

#define LVL3DEBUG(x)

struct UpnpDataObject;

struct SubscriberInfo
{
	char* SID;
	int SIDLength;
	int SEQ;

	int Address;
	unsigned short Port;
	char* Path;
	int PathLength;
	int RefCount;
	int Disposing;

	struct timeval RenewByTime;
	struct SubscriberInfo *Next;
	struct SubscriberInfo *Previous;
};
struct UpnpDataObject
{
	void (*PreSelect)(void* object,fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);
	void (*PostSelect)(void* object,int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
	void (*Destroy)(void* object);

	void *EventClient;
	void *Chain;
	int UpdateFlag;

	/* Network Poll */
	unsigned int NetworkPollTime;

	int ForceExit;
	char *UUID;
	char *UDN;
	char *Serial;

	void *WebServerTimer;
	void *HTTPServer;

	char *DeviceDescription;
	int DeviceDescriptionLength;
	int InitialNotify;
	char* ConnectionManager_SourceProtocolInfo;
	char* ConnectionManager_SinkProtocolInfo;
	char* ConnectionManager_CurrentConnectionIDs;
	char* ContentDirectory_SystemUpdateID;
	struct sockaddr_in addr;
	int addrlen;
	int MSEARCH_sock;
	struct ip_mreq mreq;
	char message[4096];
	int *AddressList;
	int AddressListLength;

	int _NumEmbeddedDevices;
	int WebSocketPortNumber;
	int *NOTIFY_SEND_socks;
	int NOTIFY_RECEIVE_sock;

	int SID;

	struct timeval CurrentTime;
	int NotifyCycleTime;
	struct timeval NotifyTime;

	sem_t EventLock;
	struct SubscriberInfo *HeadSubscriberPtr_ConnectionManager;
	int NumberOfSubscribers_ConnectionManager;
	struct SubscriberInfo *HeadSubscriberPtr_ContentDirectory;
	int NumberOfSubscribers_ContentDirectory;
};

struct MSEARCH_state
{
	char *ST;
	int STLength;
	void *upnp;
	struct sockaddr_in dest_addr;
};

#define UPNP_XML_LOCATION "./%s"
#define XML_GET_TEMPLATE "HTTP/1.1 200  OK\r\nCONTENT-TYPE:  text/xml; charset=\"utf-8\"\r\nServer: POSIX, UPnP/1.0, NDi TV-Now/"TV_NOW_VERSION"\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"

void SendXML(struct ILibWebServer_Session *session, char* location)
{
	char file_location[128];
	char *file_contents, *buffer;
	sprintf(file_location, UPNP_XML_LOCATION, location);
	int len = file_get_contents(file_location, &file_contents);
	if (len >= 0) {
		buffer = (char*)malloc(strlen(XML_GET_TEMPLATE) + len + 3);
		len = snprintf(buffer, strlen(XML_GET_TEMPLATE) + len + 3, XML_GET_TEMPLATE, len, file_contents);
		ILibWebServer_Send_Raw(session, buffer, len, 0, 1);
		free(file_contents);
	}
}

/* Pre-declarations */
void UpnpSendNotify(const struct UpnpDataObject *upnp);
void UpnpSendByeBye();
void UpnpMainInvokeSwitch();
void UpnpSendDataXmlEscaped(const void* UPnPToken, const char* Data, const int DataLength, const int Terminate);
void UpnpSendData(const void* UPnPToken, const char* Data, const int DataLength, const int Terminate);
int UpnpPeriodicNotify(struct UpnpDataObject *upnp);
void UpnpSendEvent_Body(void *upnptoken, char *body, int bodylength, struct SubscriberInfo *info);

void* UpnpGetWebServerToken(const void *MicroStackToken)
{
	return(((struct UpnpDataObject*)MicroStackToken)->HTTPServer);
}

#define UpnpBuildSsdpResponsePacket(outpacket,outlenght,ipaddr,port,EmbeddedDeviceNumber,USN,USNex,ST,NTex,NotifyTime)\
{\
	*outlenght = sprintf(outpacket,"HTTP/1.1 200 OK\r\nLOCATION: http://%d.%d.%d.%d:%d/\r\nEXT:\r\nSERVER: POSIX, UPnP/1.0, NDi TV-Now/"TV_NOW_VERSION"\r\nUSN: uuid:%s%s\r\nCACHE-CONTROL: max-age=%d\r\nST: %s%s\r\n\r\n" ,(ipaddr&0xFF),((ipaddr>>8)&0xFF),((ipaddr>>16)&0xFF),((ipaddr>>24)&0xFF),port,USN,USNex,NotifyTime,ST,NTex);\
}

#define UpnpBuildSsdpNotifyPacket(outpacket,outlenght,ipaddr,port,EmbeddedDeviceNumber,USN,USNex,NT,NTex,NotifyTime)\
{\
	*outlenght = sprintf(outpacket,"NOTIFY * HTTP/1.1\r\nLOCATION: http://%d.%d.%d.%d:%d/\r\nHOST: 239.255.255.250:1900\r\nSERVER: POSIX, UPnP/1.0, NDi TV-Now/"TV_NOW_VERSION"\r\nNTS: ssdp:alive\r\nUSN: uuid:%s%s\r\nCACHE-CONTROL: max-age=%d\r\nNT: %s%s\r\n\r\n",(ipaddr&0xFF),((ipaddr>>8)&0xFF),((ipaddr>>16)&0xFF),((ipaddr>>24)&0xFF),port,USN,USNex,NotifyTime,NT,NTex);\
}

void UpnpAsyncResponse_START(const void* UPnPToken, const char* actionName, const char* serviceUrnWithVersion)
{
	char* RESPONSE_HEADER = "\r\nEXT:\r\nCONTENT-TYPE: text/xml\r\nSERVER: POSIX, UPnP/1.0, NDi TV-Now/"TV_NOW_VERSION;
	char* RESPONSE_BODY = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">\r\n<s:Body>\r\n<u:%sResponse xmlns:u=\"%s\">";
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)UPnPToken;

	int headSize = (int)strlen(RESPONSE_BODY) + (int)strlen(actionName) + (int)strlen(serviceUrnWithVersion) + 1;
	char* head = (char*) malloc (headSize);

	int headLength = sprintf(head, RESPONSE_BODY, actionName, serviceUrnWithVersion);
	ILibWebServer_StreamHeader_Raw(session,200,"OK",RESPONSE_HEADER,1);
	ILibWebServer_StreamBody(session,head,headLength,0,0);
}

void UpnpAsyncResponse_DONE(const void* UPnPToken, const char* actionName)
{
	char* RESPONSE_FOOTER = "</u:%sResponse>\r\n   </s:Body>\r\n</s:Envelope>";

	int footSize = (int)strlen(RESPONSE_FOOTER) + (int)strlen(actionName);
	char* footer = (char*) malloc(footSize);
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)UPnPToken;

	int footLength = sprintf(footer, RESPONSE_FOOTER, actionName);
	ILibWebServer_StreamBody(session,footer,footLength,0,1);
}

void UpnpAsyncResponse_OUT(const void* UPnPToken, const char* outArgName, const char* bytes, const int byteLength, const int startArg, const int endArg)
{
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)UPnPToken;

	if (startArg != 0)
	{
		ILibWebServer_StreamBody(session,"<",1,1,0);
		ILibWebServer_StreamBody(session,(char*)outArgName,(int)strlen(outArgName),1,0);
		ILibWebServer_StreamBody(session,">",1,1,0);
	}

	if(byteLength>0)
	{
		ILibWebServer_StreamBody(session,(char*)bytes,byteLength,1,0);
	}

	if (endArg != 0)
	{
		ILibWebServer_StreamBody(session,"</",2,1,0);
		ILibWebServer_StreamBody(session,(char*)outArgName,(int)strlen(outArgName),1,0);
		ILibWebServer_StreamBody(session,">\r\n",3,1,0);
	}
}

void UpnpIPAddressListChanged(void *MicroStackToken)
{
	((struct UpnpDataObject*)MicroStackToken)->UpdateFlag = 1;
	ILibForceUnBlockChain(((struct UpnpDataObject*)MicroStackToken)->Chain);
}
void UpnpInit(struct UpnpDataObject *state,const int NotifyCycleSeconds,const unsigned short PortNumber)
{
	int ra = 1;
	int i;
	struct sockaddr_in addr;
	struct ip_mreq mreq;
	unsigned char TTL = 4;

	/* Complete State Reset */
	memset(state,0,sizeof(struct UpnpDataObject));

	/* Setup Notification Timer */
	state->NotifyCycleTime = NotifyCycleSeconds;
	gettimeofday(&(state->CurrentTime),NULL);
	(state->NotifyTime).tv_sec = (state->CurrentTime).tv_sec  + (state->NotifyCycleTime/2);
	memset((char *)&(state->addr), 0, sizeof(state->addr));
	state->addr.sin_family = AF_INET;
	state->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	state->addr.sin_port = (unsigned short)htons(UPNP_PORT);
	state->addrlen = sizeof(state->addr);
	/* Set up socket */
	state->AddressListLength = ILibGetLocalIPAddressList(&(state->AddressList));
	state->NOTIFY_SEND_socks = (int*)MALLOC(sizeof(int)*(state->AddressListLength));
	state->NOTIFY_RECEIVE_sock = socket(AF_INET, SOCK_DGRAM, 0);
	memset((char *)&(addr), 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = (unsigned short)htons(UPNP_PORT);
	if (setsockopt(state->NOTIFY_RECEIVE_sock, SOL_SOCKET, SO_REUSEADDR,(char*)&ra, sizeof(ra)) < 0)
	{
		printf("Setting SockOpt SO_REUSEADDR failed\r\n");
		exit(1);
	}
	if (bind(state->NOTIFY_RECEIVE_sock, (struct sockaddr *) &(addr), sizeof(addr)) < 0)
	{
		printf("Could not bind to UPnP Listen Port\r\n");
		exit(1);
	}
	for(i=0;i<state->AddressListLength;++i)
	{
		state->NOTIFY_SEND_socks[i] = socket(AF_INET, SOCK_DGRAM, 0);
		memset((char *)&(addr), 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = state->AddressList[i];
		addr.sin_port = (unsigned short)htons(UPNP_PORT);
		if (setsockopt(state->NOTIFY_SEND_socks[i], SOL_SOCKET, SO_REUSEADDR,(char*)&ra, sizeof(ra)) == 0)
		{
			if (setsockopt(state->NOTIFY_SEND_socks[i], IPPROTO_IP, IP_MULTICAST_TTL,(char*)&TTL, sizeof(TTL)) < 0)
			{
				/* Ignore this case */
			}
			if (bind(state->NOTIFY_SEND_socks[i], (struct sockaddr *) &(addr), sizeof(addr)) == 0)
			{
				mreq.imr_multiaddr.s_addr = inet_addr(UPNP_GROUP);
				mreq.imr_interface.s_addr = state->AddressList[i];
				if (setsockopt(state->NOTIFY_RECEIVE_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char*)&mreq, sizeof(mreq)) < 0)
				{
					/* Does not matter */
				}
			}
		}
	}
}
void UpnpPostMX_Destroy(void *object)
{
	struct MSEARCH_state *mss = (struct MSEARCH_state*)object;
	FREE(mss->ST);
	FREE(mss);
}
void UpnpPostMX_MSEARCH(void *object)
{
	struct MSEARCH_state *mss = (struct MSEARCH_state*)object;

	char *b = (char*)MALLOC(sizeof(char)*5000);
	int packetlength;
	struct sockaddr_in response_addr;
	int response_addrlen;
	int *response_socket;
	int cnt;
	int i;
	struct sockaddr_in dest_addr = mss->dest_addr;
	char *ST = mss->ST;
	int STLength = mss->STLength;
	struct UpnpDataObject *upnp = (struct UpnpDataObject*)mss->upnp;

	response_socket = (int*)MALLOC(upnp->AddressListLength*sizeof(int));
	for(i=0;i<upnp->AddressListLength;++i)
	{
		response_socket[i] = socket(AF_INET, SOCK_DGRAM, 0);
		if (response_socket[i]< 0)
		{
			printf("response socket");
			exit(1);
		}
		memset((char *)&(response_addr), 0, sizeof(response_addr));
		response_addr.sin_family = AF_INET;
		response_addr.sin_addr.s_addr = upnp->AddressList[i];
		response_addr.sin_port = (unsigned short)htons(0);
		response_addrlen = sizeof(response_addr);	
		if (bind(response_socket[i], (struct sockaddr *) &(response_addr), sizeof(response_addr)) < 0)
		{
			/* Ignore if this happens */
		}
	}
	if(STLength==15 && memcmp(ST,"upnp:rootdevice",15)==0)
	{
		for(i=0;i<upnp->AddressListLength;++i)
		{
			UpnpBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::upnp:rootdevice","upnp:rootdevice","",upnp->NotifyCycleTime);
			cnt = sendto(response_socket[i], b, packetlength, 0,(struct sockaddr *) &dest_addr, sizeof(dest_addr));
		}
	}
	else if(STLength==8 && memcmp(ST,"ssdp:all",8)==0)
	{
		for(i=0;i<upnp->AddressListLength;++i)
		{
			UpnpBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::upnp:rootdevice","upnp:rootdevice","",upnp->NotifyCycleTime);
			cnt = sendto(response_socket[i], b, packetlength, 0,
			(struct sockaddr *) &dest_addr, sizeof(dest_addr));
			UpnpBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"",upnp->UUID,"",upnp->NotifyCycleTime);
			cnt = sendto(response_socket[i], b, packetlength, 0,
			(struct sockaddr *) &dest_addr, sizeof(dest_addr));
			UpnpBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-upnp-org:device:MediaServer:1","urn:schemas-upnp-org:device:MediaServer:1","",upnp->NotifyCycleTime);
			cnt = sendto(response_socket[i], b, packetlength, 0,
			(struct sockaddr *) &dest_addr, sizeof(dest_addr));
			UpnpBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-upnp-org:service:ContentDirectory:1","urn:schemas-upnp-org:service:ContentDirectory:1","",upnp->NotifyCycleTime);
			cnt = sendto(response_socket[i], b, packetlength, 0,
			(struct sockaddr *) &dest_addr, sizeof(dest_addr));
			UpnpBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-upnp-org:service:ConnectionManager:1","urn:schemas-upnp-org:service:ConnectionManager:1","",upnp->NotifyCycleTime);
			cnt = sendto(response_socket[i], b, packetlength, 0,
			(struct sockaddr *) &dest_addr, sizeof(dest_addr));
		}
	}
	if(STLength==(int)strlen(upnp->UUID) && memcmp(ST,upnp->UUID,(int)strlen(upnp->UUID))==0)
	{
		for(i=0;i<upnp->AddressListLength;++i)
		{
			UpnpBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"",upnp->UUID,"",upnp->NotifyCycleTime);
			cnt = sendto(response_socket[i], b, packetlength, 0,
			(struct sockaddr *) &dest_addr, sizeof(dest_addr));
		}
	}
	if(STLength>=41 && memcmp(ST,"urn:schemas-upnp-org:device:MediaServer:1",41)==0)
	{
		for(i=0;i<upnp->AddressListLength;++i)
		{
			UpnpBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-upnp-org:device:MediaServer:1","urn:schemas-upnp-org:device:MediaServer:1","",upnp->NotifyCycleTime);
			cnt = sendto(response_socket[i], b, packetlength, 0,
			(struct sockaddr *) &dest_addr, sizeof(dest_addr));
		}
	}
	if(STLength>=47 && memcmp(ST,"urn:schemas-upnp-org:service:ContentDirectory:1",47)==0)
	{
		for(i=0;i<upnp->AddressListLength;++i)
		{
			UpnpBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-upnp-org:service:ContentDirectory:1","urn:schemas-upnp-org:service:ContentDirectory:1","",upnp->NotifyCycleTime);
			cnt = sendto(response_socket[i], b, packetlength, 0,
			(struct sockaddr *) &dest_addr, sizeof(dest_addr));
		}
	}
	if(STLength>=48 && memcmp(ST,"urn:schemas-upnp-org:service:ConnectionManager:1",48)==0)
	{
		for(i=0;i<upnp->AddressListLength;++i)
		{
			UpnpBuildSsdpResponsePacket(b,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-upnp-org:service:ConnectionManager:1","urn:schemas-upnp-org:service:ConnectionManager:1","",upnp->NotifyCycleTime);
			cnt = sendto(response_socket[i], b, packetlength, 0,
			(struct sockaddr *) &dest_addr, sizeof(dest_addr));
		}
	}
	for(i=0;i<upnp->AddressListLength;++i)
	{
		close(response_socket[i]);
	}
	FREE(response_socket);
	FREE(mss->ST);
	FREE(mss);
	FREE(b);
}
void UpnpProcessMSEARCH(struct UpnpDataObject *upnp, struct packetheader *packet)
{
	char* ST = NULL;
	int STLength = 0;
	struct packetheader_field_node *node;
	int MANOK = 0;
	unsigned long MXVal;
	int MXOK = 0;
	int MX;
	struct MSEARCH_state *mss = NULL;

	if(memcmp(packet->DirectiveObj,"*",1)==0)
	{
		if(memcmp(packet->Version,"1.1",3)==0)
		{
			node = packet->FirstField;
			while(node!=NULL)
			{
				if(strncasecmp(node->Field,"ST",2)==0)
				{
					ST = (char*)MALLOC(1+node->FieldDataLength);
					memcpy(ST,node->FieldData,node->FieldDataLength);
					ST[node->FieldDataLength] = 0;
					STLength = node->FieldDataLength;
				}
				else if(strncasecmp(node->Field,"MAN",3)==0 && memcmp(node->FieldData,"\"ssdp:discover\"",15)==0)
				{
					MANOK = 1;
				}
				else if(strncasecmp(node->Field,"MX",2)==0 && ILibGetULong(node->FieldData,node->FieldDataLength,&MXVal)==0)
				{
					MXOK = 1;
					MXVal = MXVal>10?10:MXVal;
				}
				node = node->NextField;
			}
			if(MANOK!=0 && MXOK!=0)
			{
				MX = (int)(0 + ((unsigned short)rand() % MXVal));
				mss = (struct MSEARCH_state*)MALLOC(sizeof(struct MSEARCH_state));
				mss->ST = ST;
				mss->STLength = STLength;
				mss->upnp = upnp;
				memset((char *)&(mss->dest_addr), 0, sizeof(mss->dest_addr));
				mss->dest_addr.sin_family = AF_INET;
				mss->dest_addr.sin_addr = packet->Source->sin_addr;
				mss->dest_addr.sin_port = packet->Source->sin_port;

				ILibLifeTime_Add(upnp->WebServerTimer,mss,MX,&UpnpPostMX_MSEARCH,&UpnpPostMX_Destroy);
			}
			else
			{
				FREE(ST);
			}
		}
	}
}
void UpnpDispatch_ConnectionManager_GetCurrentConnectionInfo(struct parser_result *xml, struct ILibWebServer_Session *ReaderObject)
{
	struct parser_result *temp;
	struct parser_result *temp2;
	struct parser_result *temp3;
	struct parser_result_field *field;
	char *VarName;
	int VarNameLength;
	int i;
	long TempLong;
	int OK = 0;
	char *p_ConnectionID = NULL;
	int p_ConnectionIDLength = 0;
	int _ConnectionID = 0;
	field = xml->FirstResult;
	while(field!=NULL)
	{
		if((memcmp(field->data,"?",1)!=0) && (memcmp(field->data,"/",1)!=0))
		{
			temp = ILibParseString(field->data,0,field->datalength," ",1);
			temp2 = ILibParseString(temp->FirstResult->data,0,temp->FirstResult->datalength,":",1);
			if(temp2->NumResults==1)
			{
				VarName = temp2->FirstResult->data;
				VarNameLength = temp2->FirstResult->datalength;
			}
			else
			{
				temp3 = ILibParseString(temp2->FirstResult->data,0,temp2->FirstResult->datalength,">",1);
				if(temp3->NumResults==1)
				{
					VarName = temp2->FirstResult->NextResult->data;
					VarNameLength = temp2->FirstResult->NextResult->datalength;
				}
				else
				{
					VarName = temp2->FirstResult->data;
					VarNameLength = temp2->FirstResult->datalength;
				}
				ILibDestructParserResults(temp3);
			}
			for(i=0;i<VarNameLength;++i)
			{
				if( i!=0 && ((VarName[i]==' ')||(VarName[i]=='/')||(VarName[i]=='>')) )
				{
					VarNameLength = i;
					break;
				}
			}
			if(VarNameLength==12 && memcmp(VarName,"ConnectionID",12) == 0)
			{
				temp3 = ILibParseString(field->data,0,field->datalength,">",1);
				if(memcmp(temp3->FirstResult->data+temp3->FirstResult->datalength-1,"/",1) != 0)
				{
					p_ConnectionID = temp3->LastResult->data;
					p_ConnectionIDLength = temp3->LastResult->datalength;
				}
				OK |= 1;
				ILibDestructParserResults(temp3);
			}
			ILibDestructParserResults(temp2);
			ILibDestructParserResults(temp);
		}
		field = field->NextResult;
	}

	if (OK != 1)
	{
		UpnpResponse_Error(ReaderObject,402,"Incorrect Arguments");
		return;
	}

	/* Type Checking */
	OK = ILibGetLong(p_ConnectionID,p_ConnectionIDLength, &TempLong);
	if(OK!=0)
	{
		UpnpResponse_Error(ReaderObject,402,"Argument[ConnectionID] illegal value");
		return;
	}
	_ConnectionID = (int)TempLong;
	UpnpConnectionManager_GetCurrentConnectionInfo((void*)ReaderObject,_ConnectionID);
}

#define UpnpDispatch_ConnectionManager_GetProtocolInfo(xml, session)\
{\
	UpnpConnectionManager_GetProtocolInfo((void*)session);\
}

#define UpnpDispatch_ConnectionManager_GetCurrentConnectionIDs(xml, session)\
{\
	UpnpConnectionManager_GetCurrentConnectionIDs((void*)session);\
}

void UpnpDispatch_ContentDirectory_Browse(struct parser_result *xml, struct ILibWebServer_Session *ReaderObject)
{
	struct parser_result *temp;
	struct parser_result *temp2;
	struct parser_result *temp3;
	struct parser_result_field *field;
	char *VarName;
	int VarNameLength;
	int i;
	unsigned long TempULong;
	int OK = 0;
	char *p_ObjectID = NULL;
	int p_ObjectIDLength = 0;
	char* _ObjectID = "";
	int _ObjectIDLength;
	char *p_BrowseFlag = NULL;
	int p_BrowseFlagLength = 0;
	char* _BrowseFlag = "";
	int _BrowseFlagLength;
	char *p_Filter = NULL;
	int p_FilterLength = 0;
	char* _Filter = "";
	int _FilterLength;
	char *p_StartingIndex = NULL;
	int p_StartingIndexLength = 0;
	unsigned int _StartingIndex = 0;
	char *p_RequestedCount = NULL;
	int p_RequestedCountLength = 0;
	unsigned int _RequestedCount = 0;
	char *p_SortCriteria = NULL;
	int p_SortCriteriaLength = 0;
	char* _SortCriteria = "";
	int _SortCriteriaLength;
	field = xml->FirstResult;
	while(field!=NULL)
	{
		if((memcmp(field->data,"?",1)!=0) && (memcmp(field->data,"/",1)!=0))
		{
			temp = ILibParseString(field->data,0,field->datalength," ",1);
			temp2 = ILibParseString(temp->FirstResult->data,0,temp->FirstResult->datalength,":",1);
			if(temp2->NumResults==1)
			{
				VarName = temp2->FirstResult->data;
				VarNameLength = temp2->FirstResult->datalength;
			}
			else
			{
				temp3 = ILibParseString(temp2->FirstResult->data,0,temp2->FirstResult->datalength,">",1);
				if(temp3->NumResults==1)
				{
					VarName = temp2->FirstResult->NextResult->data;
					VarNameLength = temp2->FirstResult->NextResult->datalength;
				}
				else
				{
					VarName = temp2->FirstResult->data;
					VarNameLength = temp2->FirstResult->datalength;
				}
				ILibDestructParserResults(temp3);
			}
			for(i=0;i<VarNameLength;++i)
			{
				if( i!=0 && ((VarName[i]==' ')||(VarName[i]=='/')||(VarName[i]=='>')) )
				{
					VarNameLength = i;
					break;
				}
			}
			if(VarNameLength==8 && memcmp(VarName,"ObjectID",8) == 0)
			{
				temp3 = ILibParseString(field->data,0,field->datalength,">",1);
				if(memcmp(temp3->FirstResult->data+temp3->FirstResult->datalength-1,"/",1) != 0)
				{
					p_ObjectID = temp3->LastResult->data;
					p_ObjectIDLength = temp3->LastResult->datalength;
					p_ObjectID[p_ObjectIDLength] = 0;
				}
				else
				{
					p_ObjectID = temp3->LastResult->data;
					p_ObjectIDLength = 0;
					p_ObjectID[p_ObjectIDLength] = 0;
				}
				OK |= 1;
				ILibDestructParserResults(temp3);
			}
			else if(VarNameLength==10 && memcmp(VarName,"BrowseFlag",10) == 0)
			{
				temp3 = ILibParseString(field->data,0,field->datalength,">",1);
				if(memcmp(temp3->FirstResult->data+temp3->FirstResult->datalength-1,"/",1) != 0)
				{
					p_BrowseFlag = temp3->LastResult->data;
					p_BrowseFlagLength = temp3->LastResult->datalength;
					p_BrowseFlag[p_BrowseFlagLength] = 0;
				}
				else
				{
					p_BrowseFlag = temp3->LastResult->data;
					p_BrowseFlagLength = 0;
					p_BrowseFlag[p_BrowseFlagLength] = 0;
				}
				OK |= 2;
				ILibDestructParserResults(temp3);
			}
			else if(VarNameLength==6 && memcmp(VarName,"Filter",6) == 0)
			{
				temp3 = ILibParseString(field->data,0,field->datalength,">",1);
				if(memcmp(temp3->FirstResult->data+temp3->FirstResult->datalength-1,"/",1) != 0)
				{
					p_Filter = temp3->LastResult->data;
					p_FilterLength = temp3->LastResult->datalength;
					p_Filter[p_FilterLength] = 0;
				}
				else
				{
					p_Filter = temp3->LastResult->data;
					p_FilterLength = 0;
					p_Filter[p_FilterLength] = 0;
				}
				OK |= 4;
				ILibDestructParserResults(temp3);
			}
			else if(VarNameLength==13 && memcmp(VarName,"StartingIndex",13) == 0)
			{
				temp3 = ILibParseString(field->data,0,field->datalength,">",1);
				if(memcmp(temp3->FirstResult->data+temp3->FirstResult->datalength-1,"/",1) != 0)
				{
					p_StartingIndex = temp3->LastResult->data;
					p_StartingIndexLength = temp3->LastResult->datalength;
				}
				OK |= 8;
				ILibDestructParserResults(temp3);
			}
			else if(VarNameLength==14 && memcmp(VarName,"RequestedCount",14) == 0)
			{
				temp3 = ILibParseString(field->data,0,field->datalength,">",1);
				if(memcmp(temp3->FirstResult->data+temp3->FirstResult->datalength-1,"/",1) != 0)
				{
					p_RequestedCount = temp3->LastResult->data;
					p_RequestedCountLength = temp3->LastResult->datalength;
				}
				OK |= 16;
				ILibDestructParserResults(temp3);
			}
			else if(VarNameLength==12 && memcmp(VarName,"SortCriteria",12) == 0)
			{
				temp3 = ILibParseString(field->data,0,field->datalength,">",1);
				if(memcmp(temp3->FirstResult->data+temp3->FirstResult->datalength-1,"/",1) != 0)
				{
					p_SortCriteria = temp3->LastResult->data;
					p_SortCriteriaLength = temp3->LastResult->datalength;
					p_SortCriteria[p_SortCriteriaLength] = 0;
				}
				else
				{
					p_SortCriteria = temp3->LastResult->data;
					p_SortCriteriaLength = 0;
					p_SortCriteria[p_SortCriteriaLength] = 0;
				}
				OK |= 32;
				ILibDestructParserResults(temp3);
			}
			ILibDestructParserResults(temp2);
			ILibDestructParserResults(temp);
		}
		field = field->NextResult;
	}

	if (OK != 63)
	{
		UpnpResponse_Error(ReaderObject,402,"Incorrect Arguments");
		return;
	}

	/* Type Checking */
	_ObjectIDLength = ILibInPlaceXmlUnEscape(p_ObjectID);
	_ObjectID = p_ObjectID;
	_BrowseFlagLength = ILibInPlaceXmlUnEscape(p_BrowseFlag);
	_BrowseFlag = p_BrowseFlag;
	if(memcmp(_BrowseFlag, "BrowseMetadata\0",15) != 0
	&& memcmp(_BrowseFlag, "BrowseDirectChildren\0",21) != 0
	)
	{
		UpnpResponse_Error(ReaderObject,402,"Argument[BrowseFlag] contains a value that is not in AllowedValueList");
		return;
	}
	_FilterLength = ILibInPlaceXmlUnEscape(p_Filter);
	_Filter = p_Filter;
	OK = ILibGetULong(p_StartingIndex,p_StartingIndexLength, &TempULong);
	if(OK!=0)
	{
		UpnpResponse_Error(ReaderObject,402,"Argument[StartingIndex] illegal value");
		return;
	}
	_StartingIndex = (unsigned int)TempULong;
	OK = ILibGetULong(p_RequestedCount,p_RequestedCountLength, &TempULong);
	if(OK!=0)
	{
		UpnpResponse_Error(ReaderObject,402,"Argument[RequestedCount] illegal value");
		return;
	}
	_RequestedCount = (unsigned int)TempULong;
	_SortCriteriaLength = ILibInPlaceXmlUnEscape(p_SortCriteria);
	_SortCriteria = p_SortCriteria;
	UpnpContentDirectory_Browse((void*)ReaderObject,_ObjectID,_BrowseFlag,_Filter,_StartingIndex,_RequestedCount,_SortCriteria);
}

#define UpnpDispatch_ContentDirectory_GetSortCapabilities(xml, session)\
{\
	UpnpContentDirectory_GetSortCapabilities((void*)session);\
}

#define UpnpDispatch_ContentDirectory_GetSystemUpdateID(xml, session)\
{\
	UpnpContentDirectory_GetSystemUpdateID((void*)session);\
}

#define UpnpDispatch_ContentDirectory_GetSearchCapabilities(xml, session)\
{\
	UpnpContentDirectory_GetSearchCapabilities((void*)session);\
}

void UpnpProcessPOST(struct ILibWebServer_Session *session, struct packetheader* header, char *bodyBuffer, int offset, int bodyBufferLength)

{
	struct packetheader_field_node *f = header->FirstField;
	char* HOST;
	char* SOAPACTION = NULL;
	int SOAPACTIONLength = 0;
	struct parser_result *r;
	struct parser_result *xml;

	xml = ILibParseString(bodyBuffer,offset,bodyBufferLength,"<",1);
	while(f!=NULL)
	{
		if(f->FieldLength==4 && strncasecmp(f->Field,"HOST",4)==0)
		{
			HOST = f->FieldData;
		}
		else if(f->FieldLength==10 && strncasecmp(f->Field,"SOAPACTION",10)==0)
		{
			r = ILibParseString(f->FieldData,0,f->FieldDataLength,"#",1);
			SOAPACTION = r->LastResult->data;
			SOAPACTIONLength = r->LastResult->datalength-1;
			ILibDestructParserResults(r);
		}
		f = f->NextField;
	}
	if(header->DirectiveObjLength==25 && memcmp((header->DirectiveObj)+1,"ContentDirectory/control",24)==0)
	{
		if(SOAPACTIONLength==6 && memcmp(SOAPACTION,"Browse",6)==0)
		{
			UpnpDispatch_ContentDirectory_Browse(xml, session);
		}
		else if(SOAPACTIONLength==19 && memcmp(SOAPACTION,"GetSortCapabilities",19)==0)
		{
			UpnpDispatch_ContentDirectory_GetSortCapabilities(xml, session);
		}
		else if(SOAPACTIONLength==17 && memcmp(SOAPACTION,"GetSystemUpdateID",17)==0)
		{
			UpnpDispatch_ContentDirectory_GetSystemUpdateID(xml, session);
		}
		else if(SOAPACTIONLength==21 && memcmp(SOAPACTION,"GetSearchCapabilities",21)==0)
		{
			UpnpDispatch_ContentDirectory_GetSearchCapabilities(xml, session);
		}
	}
	else if(header->DirectiveObjLength==26 && memcmp((header->DirectiveObj)+1,"ConnectionManager/control",25)==0)
	{
		if(SOAPACTIONLength==24 && memcmp(SOAPACTION,"GetCurrentConnectionInfo",24)==0)
		{
			UpnpDispatch_ConnectionManager_GetCurrentConnectionInfo(xml, session);
		}
		else if(SOAPACTIONLength==15 && memcmp(SOAPACTION,"GetProtocolInfo",15)==0)
		{
			UpnpDispatch_ConnectionManager_GetProtocolInfo(xml, session);
		}
		else if(SOAPACTIONLength==23 && memcmp(SOAPACTION,"GetCurrentConnectionIDs",23)==0)
		{
			UpnpDispatch_ConnectionManager_GetCurrentConnectionIDs(xml, session);
		}
	}
	ILibDestructParserResults(xml);
}
struct SubscriberInfo* UpnpRemoveSubscriberInfo(struct SubscriberInfo **Head, int *TotalSubscribers,char* SID, int SIDLength)
{
	struct SubscriberInfo *info = *Head;
	struct SubscriberInfo **ptr = Head;
	while(info!=NULL)
	{
		if(info->SIDLength==SIDLength && memcmp(info->SID,SID,SIDLength)==0)
		{
			*ptr = info->Next;
			if(info->Next!=NULL) 
			{
				(*ptr)->Previous = info->Previous;
				if((*ptr)->Previous!=NULL) 
				{
					(*ptr)->Previous->Next = info->Next;
					if((*ptr)->Previous->Next!=NULL)
					{
						(*ptr)->Previous->Next->Previous = (*ptr)->Previous;
					}
				}
			}
			break;
		}
		ptr = &(info->Next);
		info = info->Next;
	}
	if(info!=NULL)
	{
		info->Previous = NULL;
		info->Next = NULL;
		--(*TotalSubscribers);
	}
	return(info);
}

#define UpnpDestructSubscriberInfo(info)\
{\
	FREE(info->Path);\
	FREE(info->SID);\
	FREE(info);\
}

#define UpnpDestructEventObject(EvObject)\
{\
	FREE(EvObject->PacketBody);\
	FREE(EvObject);\
}

#define UpnpDestructEventDataObject(EvData)\
{\
	FREE(EvData);\
}
void UpnpExpireSubscriberInfo(struct UpnpDataObject *d, struct SubscriberInfo *info)
{
	struct SubscriberInfo *t = info;
	while(t->Previous!=NULL)
	{
		t = t->Previous;
	}
	if(d->HeadSubscriberPtr_ConnectionManager==t)
	{
		--(d->NumberOfSubscribers_ConnectionManager);
	}
	else if(d->HeadSubscriberPtr_ContentDirectory==t)
	{
		--(d->NumberOfSubscribers_ContentDirectory);
	}
	if(info->Previous!=NULL)
	{
		// This is not the Head
		info->Previous->Next = info->Next;
		if(info->Next!=NULL)
		{
			info->Previous->Next->Previous = info->Previous;
		}
	}
	else
	{
		// This is the Head
		if(d->HeadSubscriberPtr_ConnectionManager==info)
		{
			d->HeadSubscriberPtr_ConnectionManager = info->Next;
			if(info->Next!=NULL)
			{
				info->Next->Previous = d->HeadSubscriberPtr_ConnectionManager;
			}
		}
		else if(d->HeadSubscriberPtr_ContentDirectory==info)
		{
			d->HeadSubscriberPtr_ContentDirectory = info->Next;
			if(info->Next!=NULL)
			{
				info->Next->Previous = d->HeadSubscriberPtr_ContentDirectory;
			}
		}
		else
		{
			// Error
			return;
		}
	}
	--info->RefCount;
	if(info->RefCount==0)
	{
		UpnpDestructSubscriberInfo(info);
	}
}

int UpnpSubscriptionExpired(struct SubscriberInfo *info)
{
	int RetVal = 0;
	struct timeval tv;
	gettimeofday(&tv,NULL);
	if((info->RenewByTime).tv_sec < tv.tv_sec) {RetVal = -1;}
	return(RetVal);
}
void UpnpGetInitialEventBody_ConnectionManager(struct UpnpDataObject *UPnPObject,char ** body, int *bodylength)
{
	int TempLength;
	TempLength = (int)(177+(int)strlen(UPnPObject->ConnectionManager_SourceProtocolInfo)+(int)strlen(UPnPObject->ConnectionManager_SinkProtocolInfo)+(int)strlen(UPnPObject->ConnectionManager_CurrentConnectionIDs));
	*body = (char*)MALLOC(sizeof(char)*TempLength);
	*bodylength = sprintf(*body,"SourceProtocolInfo>%s</SourceProtocolInfo></e:property><e:property><SinkProtocolInfo>%s</SinkProtocolInfo></e:property><e:property><CurrentConnectionIDs>%s</CurrentConnectionIDs",UPnPObject->ConnectionManager_SourceProtocolInfo,UPnPObject->ConnectionManager_SinkProtocolInfo,UPnPObject->ConnectionManager_CurrentConnectionIDs);
}
void UpnpGetInitialEventBody_ContentDirectory(struct UpnpDataObject *UPnPObject,char ** body, int *bodylength)
{
	int TempLength;
	TempLength = (int)(33+(int)strlen(UPnPObject->ContentDirectory_SystemUpdateID));
	*body = (char*)MALLOC(sizeof(char)*TempLength);
	*bodylength = sprintf(*body,"SystemUpdateID>%s</SystemUpdateID",UPnPObject->ContentDirectory_SystemUpdateID);
}
void UpnpProcessUNSUBSCRIBE(struct packetheader *header, struct ILibWebServer_Session *session)
{
	char* SID = NULL;
	int SIDLength = 0;
	struct SubscriberInfo *Info;
	struct packetheader_field_node *f;
	char* packet = (char*)MALLOC(sizeof(char)*50);
	int packetlength;

	f = header->FirstField;
	while(f!=NULL)
	{
		if(f->FieldLength==3)
		{
			if(strncasecmp(f->Field,"SID",3)==0)
			{
				SID = f->FieldData;
				SIDLength = f->FieldDataLength;
			}
		}
		f = f->NextField;
	}
	sem_wait(&(((struct UpnpDataObject*)session->User)->EventLock));
	if(header->DirectiveObjLength==24 && memcmp(header->DirectiveObj + 1,"ConnectionManager/event",23)==0)
	{
		Info = UpnpRemoveSubscriberInfo(&(((struct UpnpDataObject*)session->User)->HeadSubscriberPtr_ConnectionManager),&(((struct UpnpDataObject*)session->User)->NumberOfSubscribers_ConnectionManager),SID,SIDLength);
		if(Info!=NULL)
		{
			--Info->RefCount;
			if(Info->RefCount==0)
			{
				UpnpDestructSubscriberInfo(Info);
			}
			packetlength = sprintf(packet,"HTTP/1.1 %d %s\r\nContent-Length: 0\r\n\r\n",200,"OK");
			ILibWebServer_Send_Raw(session,packet,packetlength,0,1);
		}
		else
		{
			packetlength = sprintf(packet,"HTTP/1.1 %d %s\r\nContent-Length: 0\r\n\r\n",412,"Invalid SID");
			ILibWebServer_Send_Raw(session,packet,packetlength,0,1);
		}
	}
	else if(header->DirectiveObjLength==23 && memcmp(header->DirectiveObj + 1,"ContentDirectory/event",22)==0)
	{
		Info = UpnpRemoveSubscriberInfo(&(((struct UpnpDataObject*)session->User)->HeadSubscriberPtr_ContentDirectory),&(((struct UpnpDataObject*)session->User)->NumberOfSubscribers_ContentDirectory),SID,SIDLength);
		if(Info!=NULL)
		{
			--Info->RefCount;
			if(Info->RefCount==0)
			{
				UpnpDestructSubscriberInfo(Info);
			}
			packetlength = sprintf(packet,"HTTP/1.1 %d %s\r\nContent-Length: 0\r\n\r\n",200,"OK");
			ILibWebServer_Send_Raw(session,packet,packetlength,0,1);
		}
		else
		{
			packetlength = sprintf(packet,"HTTP/1.1 %d %s\r\nContent-Length: 0\r\n\r\n",412,"Invalid SID");
			ILibWebServer_Send_Raw(session,packet,packetlength,0,1);
		}
	}
	sem_post(&(((struct UpnpDataObject*)session->User)->EventLock));
}
void UpnpTryToSubscribe(char* ServiceName, long Timeout, char* URL, int URLLength,struct ILibWebServer_Session *session)
{
	int *TotalSubscribers = NULL;
	struct SubscriberInfo **HeadPtr = NULL;
	struct SubscriberInfo *NewSubscriber,*TempSubscriber;
	int SIDNumber;
	char *SID;
	char *TempString;
	int TempStringLength;
	char *TempString2;
	long TempLong;
	char *packet;
	int packetlength;
	char* path;

	char *packetbody = NULL;
	int packetbodyLength;

	struct parser_result *p;
	struct parser_result *p2;

	struct UpnpDataObject *dataObject = (struct UpnpDataObject*)session->User;

	if(strncmp(ServiceName,"ConnectionManager",17)==0)
	{
		TotalSubscribers = &(dataObject->NumberOfSubscribers_ConnectionManager);
		HeadPtr = &(dataObject->HeadSubscriberPtr_ConnectionManager);
	}
	if(strncmp(ServiceName,"ContentDirectory",16)==0)
	{
		TotalSubscribers = &(dataObject->NumberOfSubscribers_ContentDirectory);
		HeadPtr = &(dataObject->HeadSubscriberPtr_ContentDirectory);
	}
	if(*HeadPtr!=NULL)
	{
		NewSubscriber = *HeadPtr;
		while(NewSubscriber!=NULL)
		{
			if(UpnpSubscriptionExpired(NewSubscriber)!=0)
			{
				TempSubscriber = NewSubscriber->Next;
				NewSubscriber = UpnpRemoveSubscriberInfo(HeadPtr,TotalSubscribers,NewSubscriber->SID,NewSubscriber->SIDLength);
				UpnpDestructSubscriberInfo(NewSubscriber);
				NewSubscriber = TempSubscriber;
			}
			else
			{
				NewSubscriber = NewSubscriber->Next;
			}
		}
	}
	if(*TotalSubscribers<10)
	{
		NewSubscriber = (struct SubscriberInfo*)MALLOC(sizeof(struct SubscriberInfo));
		SIDNumber = ++dataObject->SID;
		SID = (char*)MALLOC(10 + 6);
		sprintf(SID,"uuid:%d",SIDNumber);
		p = ILibParseString(URL,0,URLLength,"://",3);
		if(p->NumResults==1)
		{
			ILibWebServer_Send_Raw(session,"HTTP/1.1 412 Precondition Failed\r\nContent-Length: 0\r\n\r\n",55,1,1);
			ILibDestructParserResults(p);
			return;
		}
		TempString = p->LastResult->data;
		TempStringLength = p->LastResult->datalength;
		ILibDestructParserResults(p);
		p = ILibParseString(TempString,0,TempStringLength,"/",1);
		p2 = ILibParseString(p->FirstResult->data,0,p->FirstResult->datalength,":",1);
		TempString2 = (char*)MALLOC(1+sizeof(char)*p2->FirstResult->datalength);
		memcpy(TempString2,p2->FirstResult->data,p2->FirstResult->datalength);
		TempString2[p2->FirstResult->datalength] = '\0';
		NewSubscriber->Address = inet_addr(TempString2);
		if(p2->NumResults==1)
		{
			NewSubscriber->Port = 80;
			path = (char*)MALLOC(1+TempStringLength - p2->FirstResult->datalength -1);
			memcpy(path,TempString + p2->FirstResult->datalength,TempStringLength - p2->FirstResult->datalength -1);
			path[TempStringLength - p2->FirstResult->datalength - 1] = '\0';
			NewSubscriber->Path = path;
			NewSubscriber->PathLength = (int)strlen(path);
		}
		else
		{
			ILibGetLong(p2->LastResult->data,p2->LastResult->datalength,&TempLong);
			NewSubscriber->Port = (unsigned short)TempLong;
			if(TempStringLength==p->FirstResult->datalength)
			{
				path = (char*)MALLOC(2);
				memcpy(path,"/",1);
				path[1] = '\0';
			}
			else
			{
				path = (char*)MALLOC(1+TempStringLength - p->FirstResult->datalength -1);
				memcpy(path,TempString + p->FirstResult->datalength,TempStringLength - p->FirstResult->datalength -1);
				path[TempStringLength - p->FirstResult->datalength -1] = '\0';
			}
			NewSubscriber->Path = path;
			NewSubscriber->PathLength = (int)strlen(path);
		}
		ILibDestructParserResults(p);
		ILibDestructParserResults(p2);
		FREE(TempString2);
		NewSubscriber->RefCount = 1;
		NewSubscriber->Disposing = 0;
		NewSubscriber->Previous = NULL;
		NewSubscriber->SID = SID;
		NewSubscriber->SIDLength = (int)strlen(SID);
		NewSubscriber->SEQ = 0;
		gettimeofday(&(NewSubscriber->RenewByTime),NULL);
		(NewSubscriber->RenewByTime).tv_sec += (int)Timeout;
		NewSubscriber->Next = *HeadPtr;
		if(*HeadPtr!=NULL) {(*HeadPtr)->Previous = NewSubscriber;}
		*HeadPtr = NewSubscriber;
		++(*TotalSubscribers);
		LVL3DEBUG(printf("\r\n\r\nSubscribed [%s] %d.%d.%d.%d:%d FOR %d Duration\r\n",
		                 NewSubscriber->SID,(NewSubscriber->Address)&0xFF,(NewSubscriber->Address>>8)&0xFF,
		                 (NewSubscriber->Address>>16)&0xFF,(NewSubscriber->Address>>24)&0xFF,NewSubscriber->Port,Timeout);)
		LVL3DEBUG(printf("TIMESTAMP: %d <%d>\r\n\r\n",(NewSubscriber->RenewByTime).tv_sec-Timeout,NewSubscriber);)
		packet = (char*)MALLOC(132 + (int)strlen(SID) + 4);
		packetlength = sprintf(packet,"HTTP/1.1 200 OK\r\nSERVER: POSIX, UPnP/1.0, NDi TV-Now/%s\r\nSID: %s\r\nTIMEOUT: Second-%ld\r\nContent-Length: 0\r\n\r\n",
		                       TV_NOW_VERSION, SID, Timeout);
		if(strcmp(ServiceName,"ConnectionManager")==0)
		{
			UpnpGetInitialEventBody_ConnectionManager(dataObject,&packetbody,&packetbodyLength);
		}
		else if(strcmp(ServiceName,"ContentDirectory")==0)
		{
			UpnpGetInitialEventBody_ContentDirectory(dataObject,&packetbody,&packetbodyLength);
		}
		if (packetbody != NULL)	    {
			ILibWebServer_Send_Raw(session,packet,packetlength,0,1);

			UpnpSendEvent_Body(dataObject,packetbody,packetbodyLength,NewSubscriber);
			FREE(packetbody);
		} 
	}
	else
	{
		/* Too many subscribers */
		ILibWebServer_Send_Raw(session,"HTTP/1.1 412 Too Many Subscribers\r\nContent-Length: 0\r\n\r\n",56,1,1);
	}
}
void UpnpSubscribeEvents(char* path,int pathlength,char* Timeout,int TimeoutLength,char* URL,int URLLength,struct ILibWebServer_Session* session)
{
	long TimeoutVal;
	char* buffer = (char*)MALLOC(1+sizeof(char)*pathlength);

	ILibGetLong(Timeout,TimeoutLength,&TimeoutVal);
	memcpy(buffer,path,pathlength);
	buffer[pathlength] = '\0';
	FREE(buffer);
	if(TimeoutVal>7200) {TimeoutVal=7200;}

	if(pathlength==23 && memcmp(path+1,"ContentDirectory/event",22)==0)
	{
		UpnpTryToSubscribe("ContentDirectory",TimeoutVal,URL,URLLength,session);
	}
	else if(pathlength==24 && memcmp(path+1,"ConnectionManager/event",23)==0)
	{
		UpnpTryToSubscribe("ConnectionManager",TimeoutVal,URL,URLLength,session);
	}
	else
	{
		ILibWebServer_Send_Raw(session,"HTTP/1.1 412 Invalid Service Name\r\nContent-Length: 0\r\n\r\n",56,1,1);
	}
}
void UpnpRenewEvents(char* path,int pathlength,char *_SID,int SIDLength, char* Timeout, int TimeoutLength, struct ILibWebServer_Session *ReaderObject)
{
	struct SubscriberInfo *info = NULL;
	long TimeoutVal;
	struct timeval tv;
	char* packet;
	int packetlength;
	char* SID = (char*)MALLOC(SIDLength+1);
	memcpy(SID,_SID,SIDLength);
	SID[SIDLength] ='\0';
	LVL3DEBUG(gettimeofday(&tv,NULL);)
	LVL3DEBUG(printf("\r\n\r\nTIMESTAMP: %d\r\n",tv.tv_sec);)
	LVL3DEBUG(printf("SUBSCRIBER [%s] attempting to Renew Events for %s Duration [",SID,Timeout);)
	if(pathlength==24 && memcmp(path+1,"ConnectionManager/event",23)==0)
	{
		info = ((struct UpnpDataObject*)ReaderObject->User)->HeadSubscriberPtr_ConnectionManager;
	}
	else if(pathlength==23 && memcmp(path+1,"ContentDirectory/event",22)==0)
	{
		info = ((struct UpnpDataObject*)ReaderObject->User)->HeadSubscriberPtr_ContentDirectory;
	}
	while(info!=NULL && strcmp(info->SID,SID)!=0)
	{
		info = info->Next;
	}
	if(info!=NULL)
	{
		ILibGetLong(Timeout,TimeoutLength,&TimeoutVal);
		gettimeofday(&tv,NULL);
		(info->RenewByTime).tv_sec = tv.tv_sec + TimeoutVal;
		packet = (char*)MALLOC(132 + (int)strlen(SID) + 4);
		packetlength = sprintf(packet,"HTTP/1.1 200 OK\r\nSERVER: POSIX, UPnP/1.0, NDi TV-Now/%s\r\nSID: %s\r\nTIMEOUT: Second-%ld\r\nContent-Length: 0\r\n\r\n",
		                       TV_NOW_VERSION, SID, TimeoutVal);
		ILibWebServer_Send_Raw(ReaderObject,packet,packetlength,0,1);
		LVL3DEBUG(printf("OK] {%d} <%d>\r\n\r\n",TimeoutVal,info);)
	}
	else
	{
		LVL3DEBUG(printf("FAILED]\r\n\r\n");)
		ILibWebServer_Send_Raw(ReaderObject,"HTTP/1.1 412 Precondition Failed\r\nContent-Length: 0\r\n\r\n",55,1,1);
	}
	FREE(SID);
}
void UpnpProcessSUBSCRIBE(struct packetheader *header, struct ILibWebServer_Session *session)
{
	char* SID = NULL;
	int SIDLength = 0;
	char* Timeout = NULL;
	int TimeoutLength = 0;
	char* URL = NULL;
	int URLLength = 0;
	struct parser_result *p;
	struct packetheader_field_node *f;

	f = header->FirstField;
	while(f!=NULL)
	{
		if(f->FieldLength==3 && strncasecmp(f->Field,"SID",3)==0)
		{
			SID = f->FieldData;
			SIDLength = f->FieldDataLength;
		}
		else if(f->FieldLength==8 && strncasecmp(f->Field,"Callback",8)==0)
		{
			URL = f->FieldData;
			URLLength = f->FieldDataLength;
		}
		else if(f->FieldLength==7 && strncasecmp(f->Field,"Timeout",7)==0)
		{
			Timeout = f->FieldData;
			TimeoutLength = f->FieldDataLength;
		}
		f = f->NextField;
	}
	if(Timeout==NULL)
	{
		Timeout = "7200";
		TimeoutLength = 4;
	}
	else
	{
		p = ILibParseString(Timeout,0,TimeoutLength,"-",1);
		if(p->NumResults==2)
		{
			Timeout = p->LastResult->data;
			TimeoutLength = p->LastResult->datalength;
			if(TimeoutLength==8 && strncasecmp(Timeout,"INFINITE",8)==0)
			{
				Timeout = "7200";
				TimeoutLength = 4;
			}
		}
		else
		{
			Timeout = "7200";
			TimeoutLength = 4;
		}
		ILibDestructParserResults(p);
	}
	if(SID==NULL)
	{
		/* Subscribe */
		UpnpSubscribeEvents(header->DirectiveObj,header->DirectiveObjLength,Timeout,TimeoutLength,URL,URLLength,session);
	}
	else
	{
		/* Renew */
		UpnpRenewEvents(header->DirectiveObj,header->DirectiveObjLength,SID,SIDLength,Timeout,TimeoutLength,session);
	}
}
void UpnpProcessHTTPPacket(struct ILibWebServer_Session *session, struct packetheader* header, char *bodyBuffer, int offset, int bodyBufferLength)

{
	struct UpnpDataObject *dataObject = (struct UpnpDataObject*)session->User;
	char *responseHeader = 	"\r\nCONTENT-TYPE:  text/xml\r\nServer: POSIX, UPnP/1.0, NDi TV-Now/"TV_NOW_VERSION;
	char *errorTemplate = "HTTP/1.1 %d %s\r\nServer: %s\r\nContent-Length: 0\r\n\r\n";
	char *errorPacket;
	int errorPacketLength;
	char *buffer;
	/* Virtual Directory Support */
	if(header->DirectiveObjLength>=4 && memcmp(header->DirectiveObj,"/web",4)==0)
	{
		UpnpPresentationRequest((void*)session,header);
	}
	else if(header->DirectiveLength==3 && memcmp(header->Directive,"GET",3)==0)
	{
		if(header->DirectiveObjLength==1 && memcmp(header->DirectiveObj,"/",1)==0)
		{
                       // A GET Request for the device description document, so lets stream it back to the client
                        static char* hostAddress = NULL;
                        static char* deviceDescription = NULL;
                        static int bufferLen = 0;
                        if (hostAddress == NULL) {
                                hostAddress = (char*)malloc(40);
                                deviceDescription = (char*)malloc(dataObject->DeviceDescriptionLength + (40 * 3));
                                if (hostAddress == NULL || deviceDescription == NULL) exit(254);
                        }
                        char* tmpHostAddress = NULL;
                        tmpHostAddress = ILibGetHeaderLine(header, "Host", 4);
                        if (tmpHostAddress != NULL) {
                                char* colonSpot = NULL;
                                colonSpot = rindex(tmpHostAddress, ':');
                                if (colonSpot != NULL) *colonSpot = '\0';

                                if (strcmp(hostAddress,tmpHostAddress) != 0) {
                                        strncpy(hostAddress, tmpHostAddress, 40);
                                        //printf("HOST IP IS : %s\n", hostAddress);
                                }
                                bufferLen = snprintf(deviceDescription, dataObject->DeviceDescriptionLength, dataObject->DeviceDescription);
                                free(tmpHostAddress);
                        }

                        buffer = (char*)malloc(strlen(XML_GET_TEMPLATE) + bufferLen + 3);
                        if (buffer == NULL) exit(254);

                        bufferLen = snprintf(buffer, strlen(XML_GET_TEMPLATE) + bufferLen + 3, XML_GET_TEMPLATE, bufferLen, deviceDescription);
                        ILibWebServer_Send_Raw(session, buffer, bufferLen, 0, 1);
		}
		else if(header->DirectiveObjLength==26 && memcmp((header->DirectiveObj)+1,"ContentDirectory/scpd.xml",25)==0)
		{
			SendXML(session, "ContentDirectory.xml");
		}
		else if(header->DirectiveObjLength==27 && memcmp((header->DirectiveObj)+1,"ConnectionManager/scpd.xml",26)==0)
		{
			SendXML(session, "ConnectionManager.xml");
		}
		else
		{
			errorPacket = (char*)MALLOC(128);
			errorPacketLength = sprintf(errorPacket,errorTemplate,404,"File Not Found","POSIX, UPnP/1.0, NDi TV-Now/%s", TV_NOW_VERSION);
			ILibWebServer_Send_Raw(session,errorPacket,errorPacketLength,0,1);
			return;
		}
	}
	else if(header->DirectiveLength==4 && memcmp(header->Directive,"POST",4)==0)
	{
		UpnpProcessPOST(session,header,bodyBuffer,offset,bodyBufferLength);
	}
	else if(header->DirectiveLength==9 && memcmp(header->Directive,"SUBSCRIBE",9)==0)
	{
		UpnpProcessSUBSCRIBE(header,session);
	}
	else if(header->DirectiveLength==11 && memcmp(header->Directive,"UNSUBSCRIBE",11)==0)
	{
		UpnpProcessUNSUBSCRIBE(header,session);
	}
	else
	{
		errorPacket = (char*)MALLOC(128);
		errorPacketLength = sprintf(errorPacket,errorTemplate,400,"Bad Request","POSIX, UPnP/1.0, NDi TV-Now/%s", TV_NOW_VERSION);
		ILibWebServer_Send_Raw(session,errorPacket,errorPacketLength,1,1);
		return;
	}
}
void UpnpMasterPreSelect(void* object,fd_set *socketset, fd_set *writeset, fd_set *errorset, int* blocktime)
{
	int i;
	struct UpnpDataObject *UPnPObject = (struct UpnpDataObject*)object;
	int notifytime;

	int ra = 1;
	struct sockaddr_in addr;
	struct ip_mreq mreq;
	unsigned char TTL = 4;

	if(UPnPObject->InitialNotify==0)
	{
		UPnPObject->InitialNotify = -1;
		UpnpSendByeBye(UPnPObject);
		UpnpSendNotify(UPnPObject);
	}
	if(UPnPObject->UpdateFlag!=0)
	{
		UPnPObject->UpdateFlag = 0;

		/* Clear Sockets */
		for(i=0;i<UPnPObject->AddressListLength;++i)
		{
			close(UPnPObject->NOTIFY_SEND_socks[i]);
		}
		FREE(UPnPObject->NOTIFY_SEND_socks);

		/* Set up socket */
		FREE(UPnPObject->AddressList);
		UPnPObject->AddressListLength = ILibGetLocalIPAddressList(&(UPnPObject->AddressList));
		UPnPObject->NOTIFY_SEND_socks = (int*)MALLOC(sizeof(int)*(UPnPObject->AddressListLength));

		for(i=0;i<UPnPObject->AddressListLength;++i)
		{
			UPnPObject->NOTIFY_SEND_socks[i] = socket(AF_INET, SOCK_DGRAM, 0);
			memset((char *)&(addr), 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = UPnPObject->AddressList[i];
			addr.sin_port = (unsigned short)htons(UPNP_PORT);
			if (setsockopt(UPnPObject->NOTIFY_SEND_socks[i], SOL_SOCKET, SO_REUSEADDR,(char*)&ra, sizeof(ra)) == 0)
			{
				if (setsockopt(UPnPObject->NOTIFY_SEND_socks[i], IPPROTO_IP, IP_MULTICAST_TTL,(char*)&TTL, sizeof(TTL)) < 0)
				{
					// Ignore the case if setting the Multicast-TTL fails
				}
				if (bind(UPnPObject->NOTIFY_SEND_socks[i], (struct sockaddr *) &(addr), sizeof(addr)) == 0)
				{
					mreq.imr_multiaddr.s_addr = inet_addr(UPNP_GROUP);
					mreq.imr_interface.s_addr = UPnPObject->AddressList[i];
					if (setsockopt(UPnPObject->NOTIFY_RECEIVE_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char*)&mreq, sizeof(mreq)) < 0)
					{
						// Does not matter if it fails, just ignore
					}
				}
			}
		}
		UpnpSendNotify(UPnPObject);
	}
	FD_SET(UPnPObject->NOTIFY_RECEIVE_sock,socketset);
	notifytime = UpnpPeriodicNotify(UPnPObject);
	if(*blocktime>notifytime){*blocktime=notifytime;}
}

void UpnpMasterPostSelect(void* object,int slct, fd_set *socketset, fd_set *writeset, fd_set *errorset)
{
	int cnt = 0;
	struct packetheader *packet;
	struct UpnpDataObject *UPnPObject = (struct UpnpDataObject*)object;

	if(slct>0)
	{
		if(FD_ISSET(UPnPObject->NOTIFY_RECEIVE_sock,socketset)!=0)
		{
			cnt = recvfrom(UPnPObject->NOTIFY_RECEIVE_sock, UPnPObject->message, sizeof(UPnPObject->message), 0,
			(struct sockaddr *) &(UPnPObject->addr), &(UPnPObject->addrlen));
			if (cnt < 0)
			{
				printf("recvfrom");
				exit(1);
			}
			else if (cnt == 0)
			{
				/* Socket Closed? */
			}
			packet = ILibParsePacketHeader(UPnPObject->message,0,cnt);
			packet->Source = (struct sockaddr_in*)&(UPnPObject->addr);
			packet->ReceivingAddress = 0;
			if(packet->StatusCode==-1 && memcmp(packet->Directive,"M-SEARCH",8)==0)
			{
				UpnpProcessMSEARCH(UPnPObject, packet);
			}
			ILibDestructPacket(packet);
		}

	}
}
int UpnpPeriodicNotify(struct UpnpDataObject *upnp)
{
	gettimeofday(&(upnp->CurrentTime),NULL);
	if((upnp->CurrentTime).tv_sec >= (upnp->NotifyTime).tv_sec)
	{
		(upnp->NotifyTime).tv_sec = (upnp->CurrentTime).tv_sec + (upnp->NotifyCycleTime/3);
		UpnpSendNotify(upnp);
	}
	return((upnp->NotifyTime).tv_sec-(upnp->CurrentTime).tv_sec);
}
void UpnpSendNotify(const struct UpnpDataObject *upnp)
{
	int packetlength;
	char* packet = (char*)MALLOC(5000);
	int i,i2;
	struct sockaddr_in addr;
	int addrlen;
	struct in_addr interface_addr;

	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(UPNP_GROUP);
	addr.sin_port = (unsigned short)htons(UPNP_PORT);
	addrlen = sizeof(addr);

	memset((char *)&interface_addr, 0, sizeof(interface_addr));

	for(i=0;i<upnp->AddressListLength;++i)
	{
		interface_addr.s_addr = upnp->AddressList[i];
		if (setsockopt(upnp->NOTIFY_SEND_socks[i], IPPROTO_IP, IP_MULTICAST_IF,(char*)&interface_addr, sizeof(interface_addr)) == 0)
		{
			for (i2=0;i2<2;i2++)
			{
				UpnpBuildSsdpNotifyPacket(packet,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::upnp:rootdevice","upnp:rootdevice","",upnp->NotifyCycleTime);
				if (sendto(upnp->NOTIFY_SEND_socks[i], packet, packetlength, 0, (struct sockaddr *) &addr, addrlen) < 0) {exit(1);}
				UpnpBuildSsdpNotifyPacket(packet,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"","uuid:",upnp->UDN,upnp->NotifyCycleTime);
				if (sendto(upnp->NOTIFY_SEND_socks[i], packet, packetlength, 0, (struct sockaddr *) &addr, addrlen) < 0) {exit(1);}
				UpnpBuildSsdpNotifyPacket(packet,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-upnp-org:device:MediaServer:1","urn:schemas-upnp-org:device:MediaServer:1","",upnp->NotifyCycleTime);
				if (sendto(upnp->NOTIFY_SEND_socks[i], packet, packetlength, 0, (struct sockaddr *) &addr, addrlen) < 0) {exit(1);}
				UpnpBuildSsdpNotifyPacket(packet,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-upnp-org:service:ContentDirectory:1","urn:schemas-upnp-org:service:ContentDirectory:1","",upnp->NotifyCycleTime);
				if (sendto(upnp->NOTIFY_SEND_socks[i], packet, packetlength, 0, (struct sockaddr *) &addr, addrlen) < 0) {exit(1);}
				UpnpBuildSsdpNotifyPacket(packet,&packetlength,upnp->AddressList[i],(unsigned short)upnp->WebSocketPortNumber,0,upnp->UDN,"::urn:schemas-upnp-org:service:ConnectionManager:1","urn:schemas-upnp-org:service:ConnectionManager:1","",upnp->NotifyCycleTime);
				if (sendto(upnp->NOTIFY_SEND_socks[i], packet, packetlength, 0, (struct sockaddr *) &addr, addrlen) < 0) {exit(1);}
			}
		}
	}
	FREE(packet);
}

#define UpnpBuildSsdpByeByePacket(outpacket,outlenght,USN,USNex,NT,NTex)\
{\
	*outlenght = sprintf(outpacket,"NOTIFY * HTTP/1.0\r\nHOST: 239.255.255.250:1900\r\nNTS: ssdp:byebye\r\nUSN: uuid:%s%s\r\nNT: %s%s\r\nContent-Length: 0\r\n\r\n",USN,USNex,NT,NTex);\
}

void UpnpSendByeBye(const struct UpnpDataObject *upnp)
{
	int packetlength;
	char* packet = (char*)MALLOC(5000);
	int i, i2;
	struct sockaddr_in addr;
	int addrlen;
	struct in_addr interface_addr;

	memset((char *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(UPNP_GROUP);
	addr.sin_port = (unsigned short)htons(UPNP_PORT);
	addrlen = sizeof(addr);

	memset((char *)&interface_addr, 0, sizeof(interface_addr));

	for(i=0;i<upnp->AddressListLength;++i)
	{

		interface_addr.s_addr = upnp->AddressList[i];
		if (setsockopt(upnp->NOTIFY_SEND_socks[i], IPPROTO_IP, IP_MULTICAST_IF,(char*)&interface_addr, sizeof(interface_addr)) == 0)
		{

			for (i2=0;i2<2;i2++)
			{
				UpnpBuildSsdpByeByePacket(packet,&packetlength,upnp->UDN,"::upnp:rootdevice","upnp:rootdevice","");
				if (sendto(upnp->NOTIFY_SEND_socks[i], packet, packetlength, 0, (struct sockaddr *) &addr, addrlen) < 0) exit(1);
				UpnpBuildSsdpByeByePacket(packet,&packetlength,upnp->UDN,"","uuid:",upnp->UDN);
				if (sendto(upnp->NOTIFY_SEND_socks[i], packet, packetlength, 0, (struct sockaddr *) &addr, addrlen) < 0) exit(1);
				UpnpBuildSsdpByeByePacket(packet,&packetlength,upnp->UDN,"::urn:schemas-upnp-org:device:MediaServer:1","urn:schemas-upnp-org:device:MediaServer:1","");
				if (sendto(upnp->NOTIFY_SEND_socks[i], packet, packetlength, 0, (struct sockaddr *) &addr, addrlen) < 0) exit(1);
				UpnpBuildSsdpByeByePacket(packet,&packetlength,upnp->UDN,"::urn:schemas-upnp-org:service:ContentDirectory:1","urn:schemas-upnp-org:service:ContentDirectory:1","");
				if (sendto(upnp->NOTIFY_SEND_socks[i], packet, packetlength, 0, (struct sockaddr *) &addr, addrlen) < 0) exit(1);
				UpnpBuildSsdpByeByePacket(packet,&packetlength,upnp->UDN,"::urn:schemas-upnp-org:service:ConnectionManager:1","urn:schemas-upnp-org:service:ConnectionManager:1","");
				if (sendto(upnp->NOTIFY_SEND_socks[i], packet, packetlength, 0, (struct sockaddr *) &addr, addrlen) < 0) exit(1);
			}
		}
	}
	FREE(packet);
}

void UpnpResponse_Error(const void* UPnPToken, const int ErrorCode, const char* ErrorMsg)
{
	char* body;
	int bodylength;
	char* head;
	int headlength;
	body = (char*)MALLOC(395 + (int)strlen(ErrorMsg));
	bodylength = sprintf(body,"<s:Envelope\r\n xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><s:Fault><faultcode>s:Client</faultcode><faultstring>UPnPError</faultstring><detail><UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\"><errorCode>%d</errorCode><errorDescription>%s</errorDescription></UPnPError></detail></s:Fault></s:Body></s:Envelope>",ErrorCode,ErrorMsg);
	head = (char*)MALLOC(59);
	headlength = sprintf(head,"HTTP/1.1 500 Internal\r\nContent-Length: %d\r\n\r\n",bodylength);
	ILibWebServer_Send_Raw((struct ILibWebServer_Session*)UPnPToken,head,headlength,0,0);
	ILibWebServer_Send_Raw((struct ILibWebServer_Session*)UPnPToken,body,bodylength,0,1);
}

int UpnpGetLocalInterfaceToHost(const void* UPnPToken)
{
	return(ILibWebServer_GetLocalInterface((struct ILibWebServer_Session*)UPnPToken));
}

void UpnpResponseGeneric(const void* UPnPToken,const char* ServiceURI,const char* MethodName,const char* Params)
{
	char* packet;
	int packetlength;
	struct ILibWebServer_Session *session = (struct ILibWebServer_Session*)UPnPToken;
	int RVAL=0;

	packet = (char*)MALLOC(239+strlen(ServiceURI)+strlen(Params)+(strlen(MethodName)*2));
	packetlength = sprintf(packet,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><u:%sResponse xmlns:u=\"%s\">%s</u:%sResponse></s:Body></s:Envelope>",MethodName,ServiceURI,Params,MethodName);
	RVAL=ILibWebServer_StreamHeader_Raw(session,200,"OK","\r\nEXT:\r\nCONTENT-TYPE: text/xml\r\nSERVER: POSIX, UPnP/1.0, NDi TV-Now/"TV_NOW_VERSION, 1);
	if(RVAL!=ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RVAL != ILibWebServer_SEND_RESULTED_IN_DISCONNECT)
	{
		RVAL=ILibWebServer_StreamBody(session,packet,packetlength,0,1);
	}
}

void UpnpResponse_ContentDirectory_Browse(const void* UPnPToken, const char* unescaped_Result, const unsigned int NumberReturned, const unsigned int TotalMatches, const unsigned int UpdateID)
{
	char* body;
	char *Result = (char*)MALLOC(1+ILibXmlEscapeLength(unescaped_Result));

	ILibXmlEscape(Result,unescaped_Result);
	body = (char*)MALLOC(134+strlen(Result));
	sprintf(body,"<Result>%s</Result><NumberReturned>%u</NumberReturned><TotalMatches>%u</TotalMatches><UpdateID>%u</UpdateID>",Result,NumberReturned,TotalMatches,UpdateID);
	UpnpResponseGeneric(UPnPToken,"urn:schemas-upnp-org:service:ContentDirectory:1","Browse",body);
	FREE(body);
	FREE(Result);
}

void UpnpResponse_ContentDirectory_GetSortCapabilities(const void* UPnPToken, const char* unescaped_SortCaps)
{
	char* body;
	char *SortCaps = (char*)MALLOC(1+ILibXmlEscapeLength(unescaped_SortCaps));

	ILibXmlEscape(SortCaps,unescaped_SortCaps);
	body = (char*)MALLOC(22+strlen(SortCaps));
	sprintf(body,"<SortCaps>%s</SortCaps>",SortCaps);
	UpnpResponseGeneric(UPnPToken,"urn:schemas-upnp-org:service:ContentDirectory:1","GetSortCapabilities",body);
	FREE(body);
	FREE(SortCaps);
}

void UpnpResponse_ContentDirectory_GetSystemUpdateID(const void* UPnPToken, const unsigned int Id)
{
	char* body;

	body = (char*)MALLOC(21);
	sprintf(body,"<Id>%u</Id>",Id);
	UpnpResponseGeneric(UPnPToken,"urn:schemas-upnp-org:service:ContentDirectory:1","GetSystemUpdateID",body);
	FREE(body);
}

void UpnpResponse_ContentDirectory_GetSearchCapabilities(const void* UPnPToken, const char* unescaped_SearchCaps)
{
	char* body;
	char *SearchCaps = (char*)MALLOC(1+ILibXmlEscapeLength(unescaped_SearchCaps));

	ILibXmlEscape(SearchCaps,unescaped_SearchCaps);
	body = (char*)MALLOC(26+strlen(SearchCaps));
	sprintf(body,"<SearchCaps>%s</SearchCaps>",SearchCaps);
	UpnpResponseGeneric(UPnPToken,"urn:schemas-upnp-org:service:ContentDirectory:1","GetSearchCapabilities",body);
	FREE(body);
	FREE(SearchCaps);
}

void UpnpResponse_ConnectionManager_GetCurrentConnectionInfo(const void* UPnPToken, const int RcsID, const int AVTransportID, const char* unescaped_ProtocolInfo, const char* unescaped_PeerConnectionManager, const int PeerConnectionID, const char* unescaped_Direction, const char* unescaped_Status)
{
	char* body;
	char *ProtocolInfo = (char*)MALLOC(1+ILibXmlEscapeLength(unescaped_ProtocolInfo));
	char *PeerConnectionManager = (char*)MALLOC(1+ILibXmlEscapeLength(unescaped_PeerConnectionManager));
	char *Direction = (char*)MALLOC(1+ILibXmlEscapeLength(unescaped_Direction));
	char *Status = (char*)MALLOC(1+ILibXmlEscapeLength(unescaped_Status));

	ILibXmlEscape(ProtocolInfo,unescaped_ProtocolInfo);
	ILibXmlEscape(PeerConnectionManager,unescaped_PeerConnectionManager);
	ILibXmlEscape(Direction,unescaped_Direction);
	ILibXmlEscape(Status,unescaped_Status);
	body = (char*)MALLOC(233+strlen(ProtocolInfo)+strlen(PeerConnectionManager)+strlen(Direction)+strlen(Status));
	sprintf(body,"<RcsID>%d</RcsID><AVTransportID>%d</AVTransportID><ProtocolInfo>%s</ProtocolInfo><PeerConnectionManager>%s</PeerConnectionManager><PeerConnectionID>%d</PeerConnectionID><Direction>%s</Direction><Status>%s</Status>",RcsID,AVTransportID,ProtocolInfo,PeerConnectionManager,PeerConnectionID,Direction,Status);
	UpnpResponseGeneric(UPnPToken,"urn:schemas-upnp-org:service:ConnectionManager:1","GetCurrentConnectionInfo",body);
	FREE(body);
	FREE(ProtocolInfo);
	FREE(PeerConnectionManager);
	FREE(Direction);
	FREE(Status);
}

void UpnpResponse_ConnectionManager_GetProtocolInfo(const void* UPnPToken, const char* unescaped_Source, const char* unescaped_Sink)
{
	char* body;
	char *Source = (char*)MALLOC(1+ILibXmlEscapeLength(unescaped_Source));
	char *Sink = (char*)MALLOC(1+ILibXmlEscapeLength(unescaped_Sink));

	ILibXmlEscape(Source,unescaped_Source);
	ILibXmlEscape(Sink,unescaped_Sink);
	body = (char*)MALLOC(31+strlen(Source)+strlen(Sink));
	sprintf(body,"<Source>%s</Source><Sink>%s</Sink>",Source,Sink);
	UpnpResponseGeneric(UPnPToken,"urn:schemas-upnp-org:service:ConnectionManager:1","GetProtocolInfo",body);
	FREE(body);
	FREE(Source);
	FREE(Sink);
}

void UpnpResponse_ConnectionManager_GetCurrentConnectionIDs(const void* UPnPToken, const char* unescaped_ConnectionIDs)
{
	char* body;
	char *ConnectionIDs = (char*)MALLOC(1+ILibXmlEscapeLength(unescaped_ConnectionIDs));

	ILibXmlEscape(ConnectionIDs,unescaped_ConnectionIDs);
	body = (char*)MALLOC(32+strlen(ConnectionIDs));
	sprintf(body,"<ConnectionIDs>%s</ConnectionIDs>",ConnectionIDs);
	UpnpResponseGeneric(UPnPToken,"urn:schemas-upnp-org:service:ConnectionManager:1","GetCurrentConnectionIDs",body);
	FREE(body);
	FREE(ConnectionIDs);
}

void UpnpSendEventSink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *subscriber,
void *upnp,
int *PAUSE)
{
	if(done!=0 && ((struct SubscriberInfo*)subscriber)->Disposing==0)
	{
		sem_wait(&(((struct UpnpDataObject*)upnp)->EventLock));
		--((struct SubscriberInfo*)subscriber)->RefCount;
		if(((struct SubscriberInfo*)subscriber)->RefCount==0)
		{
			LVL3DEBUG(printf("\r\n\r\nSubscriber at [%s] %d.%d.%d.%d:%d was/did UNSUBSCRIBE while trying to send event\r\n\r\n",((struct SubscriberInfo*)subscriber)->SID,(((struct SubscriberInfo*)subscriber)->Address&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>8)&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>16)&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>24)&0xFF),((struct SubscriberInfo*)subscriber)->Port);)
			UpnpDestructSubscriberInfo(((struct SubscriberInfo*)subscriber));
		}
		else if(header==NULL)
		{
			LVL3DEBUG(printf("\r\n\r\nCould not deliver event for [%s] %d.%d.%d.%d:%d UNSUBSCRIBING\r\n\r\n",((struct SubscriberInfo*)subscriber)->SID,(((struct SubscriberInfo*)subscriber)->Address&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>8)&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>16)&0xFF),((((struct SubscriberInfo*)subscriber)->Address>>24)&0xFF),((struct SubscriberInfo*)subscriber)->Port);)
			// Could not send Event, so unsubscribe the subscriber
			((struct SubscriberInfo*)subscriber)->Disposing = 1;
			UpnpExpireSubscriberInfo(upnp,subscriber);
		}
		sem_post(&(((struct UpnpDataObject*)upnp)->EventLock));
	}
}
void UpnpSendEvent_Body(void *upnptoken,char *body,int bodylength,struct SubscriberInfo *info)
{
	struct UpnpDataObject* UPnPObject = (struct UpnpDataObject*)upnptoken;
	struct sockaddr_in dest;
	int packetLength;
	char *packet;
	int ipaddr;

	memset(&dest,0,sizeof(dest));
	dest.sin_addr.s_addr = info->Address;
	dest.sin_port = htons(info->Port);
	dest.sin_family = AF_INET;
	ipaddr = info->Address;

	packet = (char*)MALLOC(info->PathLength + bodylength + 383);
	packetLength = sprintf(packet,"NOTIFY %s HTTP/1.1\r\nHOST: %d.%d.%d.%d:%d\r\nContent-Type: text/xml\r\nNT: upnp:event\r\nNTS: upnp:propchange\r\nSID: %s\r\nSEQ: %d\r\nContent-Length: %d\r\n\r\n<?xml version=\"1.0\" encoding=\"utf-8\"?><e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\"><e:property><%s></e:property></e:propertyset>",info->Path,(ipaddr&0xFF),((ipaddr>>8)&0xFF),((ipaddr>>16)&0xFF),((ipaddr>>24)&0xFF),info->Port,info->SID,info->SEQ,bodylength+137,body);
	++info->SEQ;

	++info->RefCount;
	ILibWebClient_PipelineRequestEx(UPnPObject->EventClient,&dest,packet,packetLength,0,NULL,0,0,&UpnpSendEventSink,info,upnptoken);
}
void UpnpSendEvent(void *upnptoken, char* body, const int bodylength, const char* eventname)
{
	struct SubscriberInfo *info = NULL;
	struct UpnpDataObject* UPnPObject = (struct UpnpDataObject*)upnptoken;
	struct sockaddr_in dest;
	LVL3DEBUG(struct timeval tv;)

	if(UPnPObject==NULL)
	{
		FREE(body);
		return;
	}
	sem_wait(&(UPnPObject->EventLock));
	if(strncmp(eventname,"ConnectionManager",17)==0)
	{
		info = UPnPObject->HeadSubscriberPtr_ConnectionManager;
	}
	if(strncmp(eventname,"ContentDirectory",16)==0)
	{
		info = UPnPObject->HeadSubscriberPtr_ContentDirectory;
	}
	memset(&dest,0,sizeof(dest));
	while(info!=NULL)
	{
		if(!UpnpSubscriptionExpired(info))
		{
			UpnpSendEvent_Body(upnptoken,body,bodylength,info);
		}
		else
		{
			//Remove Subscriber
			LVL3DEBUG(gettimeofday(&tv,NULL);)
			LVL3DEBUG(printf("\r\n\r\nTIMESTAMP: %d\r\n",tv.tv_sec);)
			LVL3DEBUG(printf("Did not renew [%s] %d.%d.%d.%d:%d UNSUBSCRIBING <%d>\r\n\r\n",((struct SubscriberInfo*)info)->SID,(((struct SubscriberInfo*)info)->Address&0xFF),((((struct SubscriberInfo*)info)->Address>>8)&0xFF),((((struct SubscriberInfo*)info)->Address>>16)&0xFF),((((struct SubscriberInfo*)info)->Address>>24)&0xFF),((struct SubscriberInfo*)info)->Port,info);)
		}

		info = info->Next;
	}

	sem_post(&(UPnPObject->EventLock));
}

void UpnpSetState_ContentDirectory_SystemUpdateID(void *upnptoken, unsigned int val)
{
	struct UpnpDataObject *UPnPObject = (struct UpnpDataObject*)upnptoken;
	char* body;
	int bodylength;
	char* valstr;
	valstr = (char*)MALLOC(10);
	sprintf(valstr,"%u",val);
	if (UPnPObject->ContentDirectory_SystemUpdateID != NULL) FREE(UPnPObject->ContentDirectory_SystemUpdateID);
	UPnPObject->ContentDirectory_SystemUpdateID = valstr;
	body = (char*)MALLOC(38 + (int)strlen(valstr));
	bodylength = sprintf(body,"%s>%s</%s","SystemUpdateID",valstr,"SystemUpdateID");
	UpnpSendEvent(upnptoken,body,bodylength,"ContentDirectory");
	FREE(body);
}

void UpnpSetState_ConnectionManager_SourceProtocolInfo(void *upnptoken, char* val)
{
	struct UpnpDataObject *UPnPObject = (struct UpnpDataObject*)upnptoken;
	char* body;
	int bodylength;
	char* valstr;
	valstr = (char*)MALLOC(ILibXmlEscapeLength(val)+1);
	ILibXmlEscape(valstr,val);
	if (UPnPObject->ConnectionManager_SourceProtocolInfo != NULL) FREE(UPnPObject->ConnectionManager_SourceProtocolInfo);
	UPnPObject->ConnectionManager_SourceProtocolInfo = valstr;
	body = (char*)MALLOC(46 + (int)strlen(valstr));
	bodylength = sprintf(body,"%s>%s</%s","SourceProtocolInfo",valstr,"SourceProtocolInfo");
	UpnpSendEvent(upnptoken,body,bodylength,"ConnectionManager");
	FREE(body);
}

void UpnpSetState_ConnectionManager_SinkProtocolInfo(void *upnptoken, char* val)
{
	struct UpnpDataObject *UPnPObject = (struct UpnpDataObject*)upnptoken;
	char* body;
	int bodylength;
	char* valstr;
	valstr = (char*)MALLOC(ILibXmlEscapeLength(val)+1);
	ILibXmlEscape(valstr,val);
	if (UPnPObject->ConnectionManager_SinkProtocolInfo != NULL) FREE(UPnPObject->ConnectionManager_SinkProtocolInfo);
	UPnPObject->ConnectionManager_SinkProtocolInfo = valstr;
	body = (char*)MALLOC(42 + (int)strlen(valstr));
	bodylength = sprintf(body,"%s>%s</%s","SinkProtocolInfo",valstr,"SinkProtocolInfo");
	UpnpSendEvent(upnptoken,body,bodylength,"ConnectionManager");
	FREE(body);
}

void UpnpSetState_ConnectionManager_CurrentConnectionIDs(void *upnptoken, char* val)
{
	struct UpnpDataObject *UPnPObject = (struct UpnpDataObject*)upnptoken;
	char* body;
	int bodylength;
	char* valstr;
	valstr = (char*)MALLOC(ILibXmlEscapeLength(val)+1);
	ILibXmlEscape(valstr,val);
	if (UPnPObject->ConnectionManager_CurrentConnectionIDs != NULL) FREE(UPnPObject->ConnectionManager_CurrentConnectionIDs);
	UPnPObject->ConnectionManager_CurrentConnectionIDs = valstr;
	body = (char*)MALLOC(50 + (int)strlen(valstr));
	bodylength = sprintf(body,"%s>%s</%s","CurrentConnectionIDs",valstr,"CurrentConnectionIDs");
	UpnpSendEvent(upnptoken,body,bodylength,"ConnectionManager");
	FREE(body);
}


void UpnpDestroyMicroStack(void *object)
{
	struct UpnpDataObject *upnp = (struct UpnpDataObject*)object;
	struct SubscriberInfo  *sinfo,*sinfo2;
	UpnpSendByeBye(upnp);

	sem_destroy(&(upnp->EventLock));
	FREE(upnp->ConnectionManager_SourceProtocolInfo);
	FREE(upnp->ConnectionManager_SinkProtocolInfo);
	FREE(upnp->ConnectionManager_CurrentConnectionIDs);
	FREE(upnp->ContentDirectory_SystemUpdateID);

	FREE(upnp->AddressList);
	FREE(upnp->NOTIFY_SEND_socks);
	FREE(upnp->UUID);
	FREE(upnp->Serial);
	FREE(upnp->DeviceDescription);

	sinfo = upnp->HeadSubscriberPtr_ConnectionManager;
	while(sinfo!=NULL)
	{
		sinfo2 = sinfo->Next;
		UpnpDestructSubscriberInfo(sinfo);
		sinfo = sinfo2;
	}
	sinfo = upnp->HeadSubscriberPtr_ContentDirectory;
	while(sinfo!=NULL)
	{
		sinfo2 = sinfo->Next;
		UpnpDestructSubscriberInfo(sinfo);
		sinfo = sinfo2;
	}

}
int UpnpGetLocalPortNumber(void *token)
{
	return(((struct UpnpDataObject*)((struct ILibWebServer_Session*)token)->Parent)->WebSocketPortNumber);
}
void UpnpSessionReceiveSink(
struct ILibWebServer_Session *sender,
int InterruptFlag,
struct packetheader *header,
char *bodyBuffer,
int *beginPointer,
int endPointer,
int done)
{
	if(header!=NULL && done !=0 && InterruptFlag==0)
	{
		UpnpProcessHTTPPacket(sender,header,bodyBuffer,beginPointer==NULL?0:*beginPointer,endPointer);
		if(beginPointer!=NULL) {*beginPointer = endPointer;}
	}
}
void UpnpSessionSink(struct ILibWebServer_Session *SessionToken, void *user)
{
	SessionToken->OnReceive = &UpnpSessionReceiveSink;
	SessionToken->User = user;
}
void *UpnpCreateMicroStack(void *Chain, const char* FriendlyName, const char* UDN, const char* SerialNumber, const int NotifyCycleSeconds, const unsigned short PortNum)
{
	struct UpnpDataObject* RetVal = (struct UpnpDataObject*)MALLOC(sizeof(struct UpnpDataObject));
	char* DDT;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	srand((int)tv.tv_sec);
	UpnpInit(RetVal,NotifyCycleSeconds,PortNum);
	RetVal->ForceExit = 0;
	RetVal->PreSelect = &UpnpMasterPreSelect;
	RetVal->PostSelect = &UpnpMasterPostSelect;
	RetVal->Destroy = &UpnpDestroyMicroStack;
	RetVal->InitialNotify = 0;
	if (UDN != NULL)
	{
		RetVal->UUID = (char*)MALLOC((int)strlen(UDN)+6);
		sprintf(RetVal->UUID,"uuid:%s",UDN);
		RetVal->UDN = RetVal->UUID + 5;
	}
	if (SerialNumber != NULL)
	{
		RetVal->Serial = (char*)MALLOC((int)strlen(SerialNumber)+1);
		strcpy(RetVal->Serial,SerialNumber);
	}

	char* UpnpDeviceDescriptionTemplate;
	int UpnpDeviceDescriptionTemplate_len = file_get_contents("DeviceDescription.xml", &UpnpDeviceDescriptionTemplate);
	UpnpDeviceDescriptionTemplate_len += (int)strlen(FriendlyName) + (int)strlen(RetVal->Serial) + (int)strlen(RetVal->UUID) + 10;
	if ((RetVal->DeviceDescription = (char*)malloc(UpnpDeviceDescriptionTemplate_len)) == NULL) exit(254);

	RetVal->DeviceDescriptionLength = snprintf(RetVal->DeviceDescription, UpnpDeviceDescriptionTemplate_len, UpnpDeviceDescriptionTemplate,
	                                           FriendlyName, RetVal->Serial, RetVal->UDN);

	RetVal->ConnectionManager_SourceProtocolInfo = NULL;
	RetVal->ConnectionManager_SinkProtocolInfo = NULL;
	RetVal->ConnectionManager_CurrentConnectionIDs = NULL;
	RetVal->ContentDirectory_SystemUpdateID = NULL;

	RetVal->WebServerTimer = ILibCreateLifeTime(Chain);

	RetVal->HTTPServer = ILibWebServer_Create(Chain,UPNP_HTTP_MAXSOCKETS,PortNum,&UpnpSessionSink,RetVal);
	RetVal->WebSocketPortNumber=(int)ILibWebServer_GetPortNumber(RetVal->HTTPServer);

	ILibAddToChain(Chain,RetVal);
	RetVal->EventClient = ILibCreateWebClient(5,Chain);
	RetVal->Chain = Chain;
	RetVal->UpdateFlag = 0;

	sem_init(&(RetVal->EventLock),0,1);
	return(RetVal);
}

