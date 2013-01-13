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
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <malloc.h>

#include <time.h>
#include "ILibParsers.h"
#define DEBUGSTATEMENT(x)
#define MINPORTNUMBER 50000
#define PORTNUMBERRANGE 15000
#define UPNP_MAX_WAIT 2000000000
extern int errno;
static sem_t ILibChainLock;
static int ILibChainLock_RefCounter = 0;

static int malloc_counter = 0;
void* dbg_malloc(int sz)
{
	++malloc_counter;
	return((void*)malloc(sz));
}
void dbg_free(void* ptr)
{
	--malloc_counter;
	free(ptr);	
}
int dbg_GetCount()
{
	return(malloc_counter);
}

struct ILibStackNode
{
	void *Data;
	struct ILibStackNode *Next;
};
struct ILibQueueNode
{
	struct ILibStackNode *Head;
	struct ILibStackNode *Tail;
};
struct HashNode_Root
{
	struct HashNode *Root;
	sem_t LOCK;
};
struct HashNode
{
	struct HashNode *Next;
	struct HashNode *Prev;
	int KeyHash;
	char *KeyValue;
	int KeyLength;
	void *Data;
};
struct HashNodeEnumerator
{
	struct HashNode *node;
};
int ILibGetLocalIPAddressList(int** pp_int)
{
	char szBuffer[16*sizeof(struct ifreq)];
	struct ifconf ifConf;
	struct ifreq ifReq;
	int nResult;
	int LocalSock;
	struct sockaddr_in LocalAddr;
	int tempresults[16];
	int ctr=0;
	int i;
	/* Create an unbound datagram socket to do the SIOCGIFADDR ioctl on. */
	if ((LocalSock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		DEBUGSTATEMENT(printf("Can't do that\r\n"));
		exit(1);
	}
	/* Get the interface configuration information... */
	ifConf.ifc_len = sizeof szBuffer;
	ifConf.ifc_ifcu.ifcu_buf = (caddr_t)szBuffer;
	nResult = ioctl(LocalSock, SIOCGIFCONF, &ifConf);
	if (nResult < 0)
	{
		DEBUGSTATEMENT(printf("ioctl error\r\n"));
		exit(1);
	}
	/* Cycle through the list of interfaces looking for IP addresses. */
	for (i = 0;(i < ifConf.ifc_len);)
	{
		struct ifreq *pifReq = (struct ifreq *)((caddr_t)ifConf.ifc_req + i);
		i += sizeof *pifReq;
		/* See if this is the sort of interface we want to deal with. */
		strcpy (ifReq.ifr_name, pifReq -> ifr_name);
		if (ioctl (LocalSock, SIOCGIFFLAGS, &ifReq) < 0)
		{
			DEBUGSTATEMENT(printf("Can't get flags\r\n"));
			exit(1);
		}
		/* Skip loopback, point-to-point and down interfaces, */
		/* except don't skip down interfaces */
		/* if we're trying to get a list of configurable interfaces. */
		if ((ifReq.ifr_flags & IFF_LOOPBACK) || (!(ifReq.ifr_flags & IFF_UP)))
		{
			continue;
		}	
		if (pifReq -> ifr_addr.sa_family == AF_INET)
		{
			/* Get a pointer to the address... */
			memcpy (&LocalAddr, &pifReq -> ifr_addr, sizeof pifReq -> ifr_addr);
			if (LocalAddr.sin_addr.s_addr != htonl (INADDR_LOOPBACK))
			{
				tempresults[ctr] = LocalAddr.sin_addr.s_addr;
				++ctr;
			}
		}
	}
	close(LocalSock);
	*pp_int = (int*)MALLOC(sizeof(int)*(ctr));
	memcpy(*pp_int,tempresults,sizeof(int)*ctr);
	return ctr;
}

struct ILibChain
{
	void (*PreSelect)(void* object,fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);
	void (*PostSelect)(void* object,int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
	void (*Destroy)(void* object);
};

struct LifeTimeMonitorData
{
	long ExpirationTick;
	void *data;
	void (*CallbackPtr)(void *data);
	void (*DestroyPtr)(void *data);
	struct LifeTimeMonitorData *Prev;
	struct LifeTimeMonitorData *Next;
};
struct ILibLifeTime
{
	void (*PreSelect)(void* object,fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);
	void (*PostSelect)(void* object,int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
	void (*Destroy)(void* object);
	struct LifeTimeMonitorData *LM;
	void *Chain;
	sem_t SyncLock;
};
struct ILibBaseChain
{
	int TerminateFlag;
	int Terminate;
	void *Object;
	void *Next;
};

char* ILibDecompressString(unsigned char* CurrentCompressed, const int bufferLength, const int DecompressedLength)
{
	unsigned char *RetVal = (char*)MALLOC(DecompressedLength+1);
	unsigned char *CurrentUnCompressed = RetVal;
	unsigned char *EndPtr = RetVal + DecompressedLength;
	int offset,length;
	
	do
	{
		/* UnCompressed Data Block */
		memcpy(CurrentUnCompressed,CurrentCompressed+1,(int)*CurrentCompressed);
		CurrentUnCompressed += (int)*CurrentCompressed;
		CurrentCompressed += 1+((int)*CurrentCompressed);
		
		/* CompressedBlock */
		length = (unsigned short)((*(CurrentCompressed)) & 63);
		offset = ((unsigned short)(*(CurrentCompressed+1))<<2) + (((unsigned short)(*(CurrentCompressed))) >> 6);
		memcpy(CurrentUnCompressed,CurrentUnCompressed-offset,length);
		CurrentCompressed += 2;
		CurrentUnCompressed += length;
	} while(CurrentUnCompressed < EndPtr);
	RetVal[DecompressedLength] = 0;
	return(RetVal);
}
void *ILibCreateChain()
{
	struct ILibBaseChain *RetVal = (struct ILibBaseChain*)MALLOC(sizeof(struct ILibBaseChain));
	
	srand((unsigned int)time(NULL));
	
	RetVal->Object = NULL;
	RetVal->Next = NULL;
	RetVal->Terminate = socket(AF_INET, SOCK_DGRAM, 0);
	RetVal->TerminateFlag = 0;
	
	if(ILibChainLock_RefCounter==0)
	{
		sem_init(&ILibChainLock,0,1);
	}
	return(RetVal);
}
void ILibAddToChain(void *Chain, void *object)
{
	struct ILibBaseChain *chain = (struct ILibBaseChain*)Chain;
	while(chain->Next!=NULL)
	{
		chain = chain->Next;
	}
	if(chain->Object!=NULL)
	{
		chain->Next = (struct ILibBaseChain*)MALLOC(sizeof(struct ILibBaseChain));
		chain = chain->Next;
	}
	chain->Object = object;
	chain->Next = NULL;
}
void ILibForceUnBlockChain(void *Chain)
{
	struct ILibBaseChain *c = (struct ILibBaseChain*)Chain;
	int temp;
	
	sem_wait(&ILibChainLock);
	
	temp = c->Terminate;
	c->Terminate = ~0;
	close(temp);
	
	sem_post(&ILibChainLock);
	
}
void ILibStartChain(void *Chain)
{
	struct ILibBaseChain *c = (struct ILibBaseChain*)Chain;
	struct ILibBaseChain *temp;
	fd_set readset;
	fd_set errorset;
	fd_set writeset;
	struct timeval tv;
	int slct;
	
	FD_ZERO(&readset);
	FD_ZERO(&errorset);
	FD_ZERO(&writeset);
	while(((struct ILibBaseChain*)Chain)->TerminateFlag==0)
	{
		slct = 0;
		FD_ZERO(&readset);
		FD_ZERO(&errorset);
		FD_ZERO(&writeset);
		tv.tv_sec = UPNP_MAX_WAIT;
		tv.tv_usec = 0;
		
		sem_wait(&ILibChainLock);
		if(((struct ILibBaseChain*)Chain)->Terminate==~0)
		{
			slct = -1;
		}
		else
		{
			FD_SET(((struct ILibBaseChain*)Chain)->Terminate,&errorset);
		}
		sem_post(&ILibChainLock);
		
		c = (struct ILibBaseChain*)Chain;
		while(c!=NULL && c->Object!=NULL)
		{
			if(((struct ILibChain*)c->Object)->PreSelect!=NULL)
			{
				((struct ILibChain*)c->Object)->PreSelect(c->Object,&readset,&writeset,&errorset,(int*)&tv.tv_sec);
			}
			c = c->Next;
		}
		if(slct!=0)
		{
			tv.tv_sec = 0;
		}
		slct = select(FD_SETSIZE,&readset,&writeset,&errorset,&tv);
		if(slct==-1)
		{
			FD_ZERO(&readset);
			FD_ZERO(&writeset);
			FD_ZERO(&errorset);
		}
		
		if(((struct ILibBaseChain*)Chain)->Terminate==~0)
		{
			((struct ILibBaseChain*)Chain)->Terminate = socket(AF_INET,SOCK_DGRAM,0);
		}
		c = (struct ILibBaseChain*)Chain;
		while(c!=NULL && c->Object!=NULL)
		{
			if(((struct ILibChain*)c->Object)->PostSelect!=NULL)
			{
				((struct ILibChain*)c->Object)->PostSelect(c->Object,slct,&readset,&writeset,&errorset);
			}
			c = c->Next;
		}
	}
	c = (struct ILibBaseChain*)Chain;
	while(c!=NULL && c->Object!=NULL)
	{
		if(((struct ILibChain*)c->Object)->Destroy!=NULL)
		{
			((struct ILibChain*)c->Object)->Destroy(c->Object);
		}
		FREE(c->Object);
		c = c->Next;
	}
	
	c = (struct ILibBaseChain*)Chain;
	while(c!=NULL)
	{
		temp = c->Next;
		FREE(c);
		c = temp;
	}
	if(ILibChainLock_RefCounter==1)
	{
		sem_destroy(&ILibChainLock);
	}
	--ILibChainLock_RefCounter;
}
void ILibStopChain(void *Chain)
{
	((struct ILibBaseChain*)Chain)->TerminateFlag = 1;
	ILibForceUnBlockChain(Chain);
}
void ILibDestructXMLNodeList(struct ILibXMLNode *node)
{
	struct ILibXMLNode *temp;
	while(node!=NULL)
	{
		temp = node->Next;
		FREE(node);
		node = temp;
	}
}
void ILibDestructXMLAttributeList(struct ILibXMLAttribute *attribute)
{
	struct ILibXMLAttribute *temp;
	while(attribute!=NULL)
	{
		temp = attribute->Next;
		FREE(attribute);
		attribute = temp;
	}
}
int ILibProcessXMLNodeList(struct ILibXMLNode *nodeList)
{
	int RetVal = 0;
	struct ILibXMLNode *current = nodeList;
	struct ILibXMLNode *temp;
	void *TagStack;
	
	ILibCreateStack(&TagStack);
	
	while(current!=NULL)
	{
		if(current->StartTag!=0)
		{
			// Start Tag
			current->Parent = ILibPeekStack(&TagStack);
			ILibPushStack(&TagStack,current);
		}
		else
		{
			// Close Tag
			temp = (struct ILibXMLNode*)ILibPopStack(&TagStack);
			if(temp!=NULL)
			{
				if(temp->NameLength==current->NameLength && memcmp(temp->Name,current->Name,current->NameLength)==0)
				{
					if(current->Next!=NULL)
					{
						if(current->Next->StartTag!=0)
						{
							temp->Peer = current->Next;
						}
					}
					temp->ClosingTag = current;
				}
				else
				{
					// Illegal Close Tag Order
					RetVal = -2;
					break;
				}
			}
			else
			{
				// Illegal Close Tag
				RetVal = -1;
				break;
			}
		}
		current = current->Next;
	}
	
	if(TagStack!=NULL)
	{
		// Incomplete XML
		RetVal = -3;
		ILibClearStack(&TagStack);
	}
	
	return(RetVal);
}
int ILibReadInnerXML(struct ILibXMLNode *node, char **RetVal)
{
	struct ILibXMLNode *x = node;
	int length = 0;
	void *TagStack;
	
	ILibCreateStack(&TagStack);
	do
	{
		if(x->StartTag!=0) {ILibPushStack(&TagStack,x);}
		x = x->Next;
	}while(!(x->StartTag==0 && ILibPopStack(&TagStack)==node && x->NameLength==node->NameLength && memcmp(x->Name,node->Name,node->NameLength)==0));
	
	length = (int)((char*)x->Reserved - (char*)node->Reserved - 1);
	if(length<0) {length=0;}
	*RetVal = (char*)node->Reserved;
	return(length);
}
struct ILibXMLAttribute *ILibGetXMLAttributes(struct ILibXMLNode *node)
{
	struct ILibXMLAttribute *RetVal = NULL;
	struct ILibXMLAttribute *current = NULL;
	char *c;
	int EndReserved = (node->EmptyTag==0)?1:2;
	
	struct parser_result *xml;
	struct parser_result_field *field;
	struct parser_result *temp2;
	struct parser_result *temp3;
	
	c = (char*)node->Reserved - 1;
	while(*c!='<')
	{
		c = c -1;
	}
	c = c +1;
	
	xml = ILibParseStringAdv(c,0,(int)((char*)node->Reserved - c -EndReserved)," ",1);
	field = xml->FirstResult;
	if(field!=NULL) {field = field->NextResult;}
	while(field!=NULL)
	{
		if(RetVal==NULL)
		{
			RetVal = (struct ILibXMLAttribute*)MALLOC(sizeof(struct ILibXMLAttribute));
			RetVal->Next = NULL;
		}
		else
		{
			current = (struct ILibXMLAttribute*)MALLOC(sizeof(struct ILibXMLAttribute));
			current->Next = RetVal;
			RetVal = current;
		}
		temp2 = ILibParseStringAdv(field->data,0,field->datalength,":",1);
		if(temp2->NumResults==1)
		{
			RetVal->Prefix = NULL;
			RetVal->PrefixLength = 0;
			temp3 = ILibParseStringAdv(field->data,0,field->datalength,"=",1);
		}
		else
		{
			RetVal->Prefix = temp2->FirstResult->data;
			RetVal->PrefixLength = temp2->FirstResult->datalength;
			temp3 = ILibParseStringAdv(field->data,RetVal->PrefixLength+1,field->datalength-RetVal->PrefixLength-1,"=",1);
		}
		ILibDestructParserResults(temp2);
		RetVal->Name = temp3->FirstResult->data;
		RetVal->NameLength = temp3->FirstResult->datalength;
		RetVal->Value = temp3->LastResult->data;
		RetVal->ValueLength = temp3->LastResult->datalength;
		ILibDestructParserResults(temp3);
		field = field->NextResult;
	}
	
	ILibDestructParserResults(xml);
	return(RetVal);
	
}
struct ILibXMLNode *ILibParseXML(char *buffer, int offset, int length)
{
	struct parser_result *xml;
	struct parser_result_field *field;
	struct parser_result *temp;
	struct parser_result *temp2;
	struct parser_result *temp3;
	char* TagName;
	int TagNameLength;
	int StartTag;
	int EmptyTag;
	int i;
	
	struct ILibXMLNode *RetVal = NULL;
	struct ILibXMLNode *current = NULL;
	struct ILibXMLNode *x = NULL;
	
	char *NSTag;
	int NSTagLength;
	
	xml = ILibParseString(buffer,offset,length,"<",1);
	field = xml->FirstResult;
	while(field!=NULL)
	{
		if(memcmp(field->data,"?",1)!=0)
		{
			EmptyTag = 0;
			if(memcmp(field->data,"/",1)==0)
			{
				StartTag = 0;
				field->data = field->data+1;
				field->datalength -= 1;
				temp2 = ILibParseString(field->data,0,field->datalength,">",1);
			}
			else
			{
				StartTag = -1;
				temp2 = ILibParseString(field->data,0,field->datalength,">",1);
				if(temp2->FirstResult->data[temp2->FirstResult->datalength-1]=='/')
				{
					EmptyTag = -1;
				}
			}
			temp = ILibParseString(temp2->FirstResult->data,0,temp2->FirstResult->datalength," ",1);
			temp3 = ILibParseString(temp->FirstResult->data,0,temp->FirstResult->datalength,":",1);
			if(temp3->NumResults==1)
			{
				NSTag = NULL;
				NSTagLength = 0;
				TagName = temp3->FirstResult->data;
				TagNameLength = temp3->FirstResult->datalength;
			}
			else
			{
				NSTag = temp3->FirstResult->data;
				NSTagLength = temp3->FirstResult->datalength;
				TagName = temp3->FirstResult->NextResult->data;
				TagNameLength = temp3->FirstResult->NextResult->datalength;
			}
			ILibDestructParserResults(temp3);
			
			for(i=0;i<TagNameLength;++i)
			{
				if( (TagName[i]==' ')||(TagName[i]=='/')||(TagName[i]=='>')||(TagName[i]=='\r')||(TagName[i]=='\n') )
				{
					if(i!=0)
					{
						if(TagName[i]=='/')
						{
							EmptyTag = -1;
						}
						TagNameLength = i;
						break;
					}
				}
			}
			
			if(TagNameLength!=0)
			{
				x = (struct ILibXMLNode*)MALLOC(sizeof(struct ILibXMLNode));
				x->Next = NULL;
				x->Name = TagName;
				x->NameLength = TagNameLength;
				x->StartTag = StartTag;
				x->NSTag = NSTag;
				x->NSLength = NSTagLength;
				
				x->Parent = NULL;
				x->Peer = NULL;
				x->ClosingTag = NULL;
				x->EmptyTag = 0;
				
				
				if(StartTag==0)
				{
					x->Reserved = field->data;
					do
					{
						x->Reserved -= 1;
					}while(*((char*)x->Reserved)=='<');
				}
				else
				{
					x->Reserved = temp2->LastResult->data;
				}
				
				if(RetVal==NULL)
				{
					RetVal = x;
				}
				else
				{
					current->Next = x;
				}
				current = x;
				if(EmptyTag!=0)
				{
					x = (struct ILibXMLNode*)MALLOC(sizeof(struct ILibXMLNode));
					x->Next = NULL;
					x->Name = TagName;
					x->NameLength = TagNameLength;
					x->StartTag = 0;
					x->NSTag = NSTag;
					x->NSLength = NSTagLength;
					
					x->Parent = NULL;
					x->Peer = NULL;
					x->ClosingTag = NULL;
					
					x->Reserved = current->Reserved;
					current->EmptyTag = -1;
					current->Next = x;
					current = x;
				}
			}
			
			ILibDestructParserResults(temp2);
			ILibDestructParserResults(temp);
		}
		field = field->NextResult;
	}
	
	ILibDestructParserResults(xml);
	return(RetVal);
}
void *ILibQueue_Create()
{
	struct ILibQueueNode *RetVal = (struct ILibQueueNode*)MALLOC(sizeof(struct ILibQueueNode));
	memset(RetVal,0,sizeof(struct ILibQueueNode));
	return(RetVal);
}
void ILibQueue_Destroy(void *q)
{
	struct ILibQueueNode *qn = (struct ILibQueueNode*)q;
	struct ILibStackNode *temp;
	
	temp = qn->Head;
	while(temp!=NULL)
	{
		qn->Head = qn->Head->Next;
		FREE(temp);
		temp = qn->Head;
	}
	FREE(qn);
}
int ILibQueue_IsEmpty(void *q)
{
	struct ILibQueueNode *qn = (struct ILibQueueNode*)q;
	return(qn->Head==NULL?1:0);
}
void ILibQueue_EnQueue(void *q, void *data)
{
	struct ILibQueueNode *qn = (struct ILibQueueNode*)q;
	struct ILibStackNode *sn = (struct ILibStackNode*)MALLOC(sizeof(struct ILibStackNode));
	
	sn->Data = data;
	sn->Next = NULL;
	
	if(qn->Head==NULL)
	{
		qn->Head = sn;
		qn->Tail = sn;
	}
	else
	{
		qn->Tail->Next = sn;
		qn->Tail = sn;
	}
}
void *ILibQueue_DeQueue(void *q)
{
	struct ILibQueueNode *qn = (struct ILibQueueNode*)q;
	struct ILibStackNode *temp;
	
	void *RetVal = NULL;
	
	if(qn->Head==NULL) {return(NULL);}
	temp = qn->Head;
	RetVal = qn->Head->Data;
	qn->Head = qn->Head->Next;
	if(qn->Head==NULL) {qn->Tail = NULL;}
	FREE(temp);
	return(RetVal);
}
void *ILibQueue_PeekQueue(void *q)
{
	struct ILibQueueNode *qn = (struct ILibQueueNode*)q;
	if(qn->Head==NULL)
	{
		return(NULL);
	}
	else
	{
		return(qn->Head->Data);
	}
}
void ILibCreateStack(void **TheStack)
{
	*TheStack = NULL;
}
void ILibPushStack(void **TheStack, void *data)
{
	struct ILibStackNode *RetVal = (struct ILibStackNode*)MALLOC(sizeof(struct ILibStackNode));
	RetVal->Data = data;
	RetVal->Next = *TheStack;
	*TheStack = RetVal;
}
void *ILibPopStack(void **TheStack)
{
	void *RetVal = NULL;
	void *Temp;
	if(*TheStack!=NULL)
	{
		RetVal = ((struct ILibStackNode*)*TheStack)->Data;
		Temp = *TheStack;
		*TheStack = ((struct ILibStackNode*)*TheStack)->Next;
		FREE(Temp);
	}
	return(RetVal);
}
void *ILibPeekStack(void **TheStack)
{
	void *RetVal = NULL;
	if(*TheStack!=NULL)
	{
		RetVal = ((struct ILibStackNode*)*TheStack)->Data;
	}
	return(RetVal);
}
void ILibClearStack(void **TheStack)
{
	void *Temp = *TheStack;
	do
	{
		ILibPopStack(&Temp);
	}while(Temp!=NULL);
	*TheStack = NULL;
}
void ILibHashTree_Lock(void *hashtree)
{
	struct HashNode_Root *r = (struct HashNode_Root*)hashtree;
	sem_wait(&(r->LOCK));
}
void ILibHashTree_UnLock(void *hashtree)
{
	struct HashNode_Root *r = (struct HashNode_Root*)hashtree;
	sem_post(&(r->LOCK));
}
void ILibDestroyHashTree(void *tree)
{
	struct HashNode_Root *r = (struct HashNode_Root*)tree;
	struct HashNode *c = r->Root;
	struct HashNode *n;
	
	sem_destroy(&(r->LOCK));
	while(c!=NULL)
	{
		n = c->Next;
		if(c->KeyValue!=NULL) {FREE(c->KeyValue);}
		FREE(c);
		c = n;
	}
	free(r);
}
void *ILibHashTree_GetEnumerator(void *tree)
{
	struct HashNodeEnumerator *en = (struct HashNodeEnumerator*)MALLOC(sizeof(struct HashNodeEnumerator));
	en->node = ((struct HashNode_Root*)tree)->Root;
	return(en);
}
void ILibHashTree_DestroyEnumerator(void *tree_enumerator)
{
	FREE(tree_enumerator);
}
int ILibHashTree_MoveNext(void *tree_enumerator)
{
	struct HashNodeEnumerator *en = (struct HashNodeEnumerator*)tree_enumerator;
	if(en->node!=NULL)
	{
		en->node = en->node->Next;
		return(en->node!=NULL?0:1);
	}
	else
	{
		return(1);
	}
}
void ILibHashTree_GetValue(void *tree_enumerator, char **key, int *keyLength, void **data)
{
	struct HashNodeEnumerator *en = (struct HashNodeEnumerator*)tree_enumerator;
	
	if(key!=NULL){*key = en->node->KeyValue;}
	if(keyLength!=NULL){*keyLength = en->node->KeyLength;}
	if(data!=NULL){*data = en->node->Data;}
}
void* ILibInitHashTree()
{
	struct HashNode_Root *Root = (struct  HashNode_Root*)malloc(sizeof(struct HashNode_Root));
	struct HashNode *RetVal = (struct HashNode*)MALLOC(sizeof(struct HashNode));
	memset(RetVal,0,sizeof(struct HashNode));
	Root->Root = RetVal;
	sem_init(&(Root->LOCK),0,1);
	return(Root);
}
int ILibGetHashValue(void *key, int keylength)
{
	int HashValue=0;
	char TempValue[4];
	
	if(keylength<=4)
	{
		memset(TempValue,0,4);
		memcpy(TempValue,key,keylength);
		HashValue = *((int*)TempValue);
	}
	else
	{
		memcpy(TempValue,key,4);
		HashValue = *((int*)TempValue);
		memcpy(TempValue,(char*)key+(keylength-4),4);
		HashValue = HashValue^(*((int*)TempValue));
		if(keylength>=10)
		{
			memcpy(TempValue,(char*)key+(keylength/2),4);
			HashValue = HashValue^(*((int*)TempValue));
		}
	}
	return(HashValue);
}
struct HashNode* ILibFindEntry(void *hashtree, void *key, int keylength, int create)
{
	struct HashNode *current = ((struct HashNode_Root*)hashtree)->Root;
	int HashValue = ILibGetHashValue(key,keylength);
	int done = 0;
	
	if(keylength==0){return(NULL);}
	
	while(done==0)
	{
		if(current->KeyHash==HashValue)
		{
			if(current->KeyLength==keylength && memcmp(current->KeyValue,key,keylength)==0)
			{
				return(current);
			}
		}
		
		if(current->Next!=NULL)
		{
			current = current->Next;
		}
		else if(create!=0)
		{
			current->Next = (struct HashNode*)MALLOC(sizeof(struct HashNode));
			memset(current->Next,0,sizeof(struct HashNode));
			current->Next->Prev = current;
			current->Next->KeyHash = HashValue;
			current->Next->KeyValue = (void*)MALLOC(keylength);
			memcpy(current->Next->KeyValue,key,keylength);
			current->Next->KeyLength = keylength;
			return(current->Next);
		}
		else
		{
			return(NULL);
		}
	}
	return(NULL);
}
int ILibHasEntry(void *hashtree, char* key, int keylength)
{
	return(ILibFindEntry(hashtree,key,keylength,0)!=NULL?1:0);
}
void ILibAddEntry(void* hashtree, char* key, int keylength, void *value)
{
	struct HashNode* n = ILibFindEntry(hashtree,key,keylength,1);
	n->Data = value;
}

void* ILibGetEntry(void *hashtree, char* key, int keylength)
{
	struct HashNode* n = ILibFindEntry(hashtree,key,keylength,0);
	if(n==NULL)
	{
		return(NULL);
	}
	else
	{
		return(n->Data);
	}
}
void ILibDeleteEntry(void *hashtree, char* key, int keylength)
{
	struct HashNode* n = ILibFindEntry(hashtree,key,keylength,0);
	if(n!=NULL)
	{
		n->Prev->Next = n->Next;
		if(n->Next!=NULL)
		{
			n->Next->Prev = n->Prev;
		}
		FREE(n->KeyValue);
		FREE(n);
	}
}
int ILibGetLong(char *TestValue, int TestValueLength, long* NumericValue)
{
	char* StopString;
	char* TempBuffer2 = (char*)MALLOC(1+sizeof(char)*19);
	char* TempBuffer = (char*)MALLOC(1+sizeof(char)*TestValueLength);
	memcpy(TempBuffer,TestValue,TestValueLength);
	TempBuffer[TestValueLength] = '\0';
	*NumericValue = strtol(TempBuffer,&StopString,10);
	if(*StopString!='\0')
	{
		FREE(TempBuffer);
		FREE(TempBuffer2);
		return(-1);
	}
	else
	{
		FREE(TempBuffer);
		FREE(TempBuffer2);
		if(errno!=ERANGE)
		{
			return(0);
		}
		else
		{
			return(-1);
		}
	}
}
int ILibGetULong(const char *TestValue, const int TestValueLength, unsigned long* NumericValue){
	char* StopString;
	char* TempBuffer2 = (char*)MALLOC(1+sizeof(char)*19);
	char* TempBuffer = (char*)MALLOC(1+sizeof(char)*TestValueLength);
	memcpy(TempBuffer,TestValue,TestValueLength);
	TempBuffer[TestValueLength] = '\0';
	*NumericValue = strtoul(TempBuffer,&StopString,10);
	if(*StopString!='\0')
	{
		FREE(TempBuffer);
		FREE(TempBuffer2);
		return(-1);
	}
	else
	{
		FREE(TempBuffer);
		FREE(TempBuffer2);
		if(errno!=ERANGE)
		{
			if(memcmp(TestValue,"-",1)==0)
			{
				return(-1);
			}
			return(0);
		}
		else
		{
			return(-1);
		}
	}
}
int ILibIsDelimiter(char* buffer, int offset, int buffersize, char* Delimiter, int DelimiterLength)
{
	int i=0;
	int RetVal = 1;
	if(offset+DelimiterLength>buffersize)
	{
		return(0);
	}
	
	for(i=0;i<DelimiterLength;++i)
	{
		if(buffer[offset+i]!=Delimiter[i])
		{
			RetVal = 0;
			break;
		}
	}
	return(RetVal);
}
struct parser_result* ILibParseStringAdv(char* buffer, int offset, int length, char* Delimiter, int DelimiterLength)
{
	struct parser_result* RetVal = (struct parser_result*)MALLOC(sizeof(struct parser_result));
	int i=0;	
	char* Token = NULL;
	int TokenLength = 0;
	struct parser_result_field *p_resultfield;
	int Ignore = 0;
	char StringDelimiter=0;
	
	RetVal->FirstResult = NULL;
	RetVal->NumResults = 0;
	
	Token = buffer + offset;
	for(i=offset;i<length;++i)
	{
		if(StringDelimiter==0)
		{
			if(buffer[i]=='"') 
			{
				StringDelimiter='"';
				Ignore=1;
			}
			else
			{
				if(buffer[i]=='\'')
				{
					StringDelimiter='\'';
					Ignore=1;
				}
			}
		}
		else
		{
			if(buffer[i]==StringDelimiter)
			{
				Ignore=((Ignore==0)?1:0);
			}
		}
		if(Ignore==0 && ILibIsDelimiter(buffer,i,length,Delimiter,DelimiterLength))
		{
			p_resultfield = (struct parser_result_field*)MALLOC(sizeof(struct parser_result_field));
			p_resultfield->data = Token;
			p_resultfield->datalength = TokenLength;
			p_resultfield->NextResult = NULL;
			if(RetVal->FirstResult != NULL)
			{
				RetVal->LastResult->NextResult = p_resultfield;
				RetVal->LastResult = p_resultfield;
			}
			else
			{
				RetVal->FirstResult = p_resultfield;
				RetVal->LastResult = p_resultfield;
			}
			
			++RetVal->NumResults;
			i = i + DelimiterLength -1;
			Token = Token + TokenLength + DelimiterLength;
			TokenLength = 0;	
		}
		else
		{
			++TokenLength;
		}
	}
	p_resultfield = (struct parser_result_field*)MALLOC(sizeof(struct parser_result_field));
	p_resultfield->data = Token;
	p_resultfield->datalength = TokenLength;
	p_resultfield->NextResult = NULL;
	if(RetVal->FirstResult != NULL)
	{
		RetVal->LastResult->NextResult = p_resultfield;
		RetVal->LastResult = p_resultfield;
	}
	else
	{
		RetVal->FirstResult = p_resultfield;
		RetVal->LastResult = p_resultfield;
	}	
	++RetVal->NumResults;
	
	return(RetVal);
}
struct parser_result* ILibParseString(char* buffer, int offset, int length, char* Delimiter, int DelimiterLength)
{
	struct parser_result* RetVal = (struct parser_result*)MALLOC(sizeof(struct parser_result));
	int i=0;	
	char* Token = NULL;
	int TokenLength = 0;
	struct parser_result_field *p_resultfield;
	
	RetVal->FirstResult = NULL;
	RetVal->NumResults = 0;
	
	Token = buffer + offset;
	for(i=offset;i<length;++i)
	{
		if(ILibIsDelimiter(buffer,i,length,Delimiter,DelimiterLength))
		{
			p_resultfield = (struct parser_result_field*)MALLOC(sizeof(struct parser_result_field));
			p_resultfield->data = Token;
			p_resultfield->datalength = TokenLength;
			p_resultfield->NextResult = NULL;
			if(RetVal->FirstResult != NULL)
			{
				RetVal->LastResult->NextResult = p_resultfield;
				RetVal->LastResult = p_resultfield;
			}
			else
			{
				RetVal->FirstResult = p_resultfield;
				RetVal->LastResult = p_resultfield;
			}
			
			++RetVal->NumResults;
			i = i + DelimiterLength -1;
			Token = Token + TokenLength + DelimiterLength;
			TokenLength = 0;	
		}
		else
		{
			++TokenLength;
		}
	}
	p_resultfield = (struct parser_result_field*)MALLOC(sizeof(struct parser_result_field));
	p_resultfield->data = Token;
	p_resultfield->datalength = TokenLength;
	p_resultfield->NextResult = NULL;
	if(RetVal->FirstResult != NULL)
	{
		RetVal->LastResult->NextResult = p_resultfield;
		RetVal->LastResult = p_resultfield;
	}
	else
	{
		RetVal->FirstResult = p_resultfield;
		RetVal->LastResult = p_resultfield;
	}	
	++RetVal->NumResults;
	
	return(RetVal);
}

void ILibDestructParserResults(struct parser_result *result)
{
	struct parser_result_field *node = result->FirstResult;
	struct parser_result_field *temp;
	
	while(node!=NULL)
	{
		temp = node->NextResult;
		FREE(node);
		node = temp;
	}
	FREE(result);
}

void ILibDestructPacket(struct packetheader *packet)
{
	struct packetheader_field_node *node = packet->FirstField;
	struct packetheader_field_node *nextnode;
	
	while(node!=NULL)
	{
		nextnode = node->NextField;
		if(node->UserAllocStrings!=0)
		{
			FREE(node->Field);
			FREE(node->FieldData);
		}
		FREE(node);
		node = nextnode;
	}
	if(packet->UserAllocStrings!=0)
	{
		if(packet->StatusData!=NULL) {FREE(packet->StatusData);}
		if(packet->Directive!=NULL) {FREE(packet->Directive);}
		if(packet->DirectiveObj!=NULL) {FREE(packet->DirectiveObj);}
		if(packet->Body!=NULL) FREE(packet->Body);
	}
	if(packet->UserAllocVersion!=0)
	{
		FREE(packet->Version);
	}
	FREE(packet);
}

struct packetheader* ILibParsePacketHeader(char* buffer, int offset, int length)
{
	struct packetheader *RetVal = (struct packetheader*)MALLOC(sizeof(struct packetheader));
	struct parser_result *_packet;
	struct parser_result *p;
	struct parser_result *StartLine;
	struct parser_result_field *HeaderLine;
	struct parser_result_field *f;
	char* tempbuffer;
	struct packetheader_field_node *node;
	int i=0;
	int FLNWS = -1;
	int FTNWS = -1;
	
	memset(RetVal,0,sizeof(struct packetheader));
	
	p = (struct parser_result*)ILibParseString(buffer,offset,length,"\r\n",2);
	_packet = p;
	f = p->FirstResult;
	StartLine = (struct parser_result*)ILibParseString(f->data,0,f->datalength," ",1);
	HeaderLine = f->NextResult;
	if(memcmp(StartLine->FirstResult->data,
	"HTTP/",
	5)==0)
	{
		/* Response Packet */
		p = (struct parser_result*)ILibParseString(StartLine->FirstResult->data,
		0,
		StartLine->FirstResult->datalength,
		"/",1);
		RetVal->Version = p->LastResult->data;
		RetVal->VersionLength = p->LastResult->datalength;
		ILibDestructParserResults(p);
		tempbuffer = (char*)MALLOC(1+sizeof(char)*(StartLine->FirstResult->NextResult->datalength));
		memcpy(tempbuffer,StartLine->FirstResult->NextResult->data,
		StartLine->FirstResult->NextResult->datalength);
		tempbuffer[StartLine->FirstResult->NextResult->datalength] = '\0';
		RetVal->StatusCode = (int)atoi(tempbuffer);
		FREE(tempbuffer);
		RetVal->StatusData = StartLine->FirstResult->NextResult->NextResult->data;
		RetVal->StatusDataLength = StartLine->FirstResult->NextResult->NextResult->datalength;
	}
	else
	{
		/* Request Packet */
		RetVal->Directive = StartLine->FirstResult->data;
		RetVal->DirectiveLength = StartLine->FirstResult->datalength;
		RetVal->DirectiveObj = StartLine->FirstResult->NextResult->data;
		RetVal->DirectiveObjLength = StartLine->FirstResult->NextResult->datalength;
		RetVal->StatusCode = -1;
		p = (struct parser_result*)ILibParseString(StartLine->LastResult->data,
		0,
		StartLine->LastResult->datalength,
		"/",1);
		RetVal->Version = p->LastResult->data;
		RetVal->VersionLength = p->LastResult->datalength;
		ILibDestructParserResults(p);
		
		RetVal->Directive[RetVal->DirectiveLength] = '\0';
		RetVal->DirectiveObj[RetVal->DirectiveObjLength] = '\0';
	}
	while(HeaderLine!=NULL)
	{
		if(HeaderLine->datalength==0)
		{
			break;
		}
		node = (struct packetheader_field_node*)MALLOC(sizeof(struct packetheader_field_node));
		memset(node,0,sizeof(struct packetheader_field_node));
		for(i=0;i<HeaderLine->datalength;++i)
		{
			if(*((HeaderLine->data)+i)==':')
			{
				node->Field = HeaderLine->data;
				node->FieldLength = i;
				node->FieldData = HeaderLine->data + i + 1;
				node->FieldDataLength = (HeaderLine->datalength)-i-1;
				break;
			}
		}
		if(node->Field==NULL)
		{
			FREE(RetVal);
			RetVal = NULL;
			break;
		}
		FLNWS = 0;
		FTNWS = node->FieldDataLength-1;
		for(i=0;i<node->FieldDataLength;++i)
		{
			if(*((node->FieldData)+i)!=' ')
			{
				FLNWS = i;
				break;
			}
		}
		for(i=(node->FieldDataLength)-1;i>=0;--i)
		{
			if(*((node->FieldData)+i)!=' ')
			{
				FTNWS = i;
				break;
			}
		}
		node->FieldData = (node->FieldData) + FLNWS;
		node->FieldDataLength = (FTNWS - FLNWS)+1;
		
		node->Field[node->FieldLength] = '\0';
		node->FieldData[node->FieldDataLength] = '\0';
		
		node->UserAllocStrings = 0;
		node->NextField = NULL;
		
		if(RetVal->FirstField==NULL)
		{
			RetVal->FirstField = node;
			RetVal->LastField = node;
		}
		else
		{
			RetVal->LastField->NextField = node;	
		}
		RetVal->LastField = node;
		HeaderLine = HeaderLine->NextResult;
	}
	ILibDestructParserResults(_packet);
	ILibDestructParserResults(StartLine);
	return(RetVal);
}
int ILibGetRawPacket(struct packetheader* packet,char **RetVal)
{
	int i;
	int BufferSize = 0;
	char* Buffer;
	struct packetheader_field_node *node;
	
	if(packet->StatusCode!=-1)
	{
		BufferSize = 12 + packet->VersionLength + packet->StatusDataLength;
		/* HTTP/1.1 200 OK\r\n */
	}
	else
	{
		BufferSize = packet->DirectiveLength + packet->DirectiveObjLength + 12;
		/* GET / HTTP/1.1\r\n */
	}
	
	node = packet->FirstField;
	while(node!=NULL)
	{
		BufferSize += node->FieldLength + node->FieldDataLength + 4;
		node = node->NextField;
	}
	BufferSize += (3+packet->BodyLength);
	
	*RetVal = (char*)MALLOC(BufferSize);
	Buffer = *RetVal;
	if(packet->StatusCode!=-1)
	{
		memcpy(Buffer,"HTTP/",5);
		memcpy(Buffer+5,packet->Version,packet->VersionLength);
		i = 5+packet->VersionLength;
		
		i+=sprintf(Buffer+i," %d ",packet->StatusCode);
		memcpy(Buffer+i,packet->StatusData,packet->StatusDataLength);
		i+=packet->StatusDataLength;
		
		memcpy(Buffer+i,"\r\n",2);
		i+=2;
		/* HTTP/1.1 200 OK\r\n */
	}
	else
	{
		memcpy(Buffer,packet->Directive,packet->DirectiveLength);
		i = packet->DirectiveLength;
		memcpy(Buffer+i," ",1);
		i+=1;
		memcpy(Buffer+i,packet->DirectiveObj,packet->DirectiveObjLength);
		i+=packet->DirectiveObjLength;
		memcpy(Buffer+i," HTTP/",6);
		i+=6;
		memcpy(Buffer+i,packet->Version,packet->VersionLength);
		i+=packet->VersionLength;
		memcpy(Buffer+i,"\r\n",2);
		i+=2;
		/* GET / HTTP/1.1\r\n */
	}
	
	node = packet->FirstField;
	while(node!=NULL)
	{
		memcpy(Buffer+i,node->Field,node->FieldLength);
		i+=node->FieldLength;
		memcpy(Buffer+i,": ",2);
		i+=2;
		memcpy(Buffer+i,node->FieldData,node->FieldDataLength);
		i+=node->FieldDataLength;
		memcpy(Buffer+i,"\r\n",2);
		i+=2;
		BufferSize += node->FieldLength + node->FieldDataLength + 4;
		node = node->NextField;
	}
	memcpy(Buffer+i,"\r\n",2);
	i+=2;
	
	memcpy(Buffer+i,packet->Body,packet->BodyLength);
	i+=packet->BodyLength;
	Buffer[i] = '\0';
	
	return(i);
}

unsigned short ILibGetDGramSocket(int local, int *TheSocket)
{
	unsigned short PortNum = -1;
	struct sockaddr_in addr;	
	memset((char *)&(addr), 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = local;
	
	*TheSocket = (int)socket(AF_INET, SOCK_DGRAM, 0);
	do
	{
		PortNum = (unsigned short)(50000 + ((unsigned short)rand() % 15000));
		addr.sin_port = htons(PortNum);
	}
	while(bind((int)*TheSocket, (struct sockaddr *) &(addr), sizeof(addr)) < 0);
	return(PortNum);
}

unsigned short ILibGetStreamSocket(int local, unsigned short PortNumber, int *TheSocket)
{
	int ra=1;
	unsigned short PortNum = -1;
	struct sockaddr_in addr;
	memset((char *)&(addr), 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = local;
	
	*TheSocket = (int)socket(AF_INET, SOCK_STREAM, 0);
	if(PortNumber==0)
	{
		do
		{
			PortNum = (unsigned short)(MINPORTNUMBER + ((unsigned short)rand() % PORTNUMBERRANGE));
			addr.sin_port = htons(PortNum);
		}
		while(bind((int)*TheSocket, (struct sockaddr *) &(addr), sizeof(addr)) < 0);
	}
	else
	{
		addr.sin_port = htons(PortNumber);
		if (setsockopt((int)*TheSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&ra, sizeof(ra)) < 0)
		{
		}
		PortNum = bind((int)*TheSocket, (struct sockaddr *) &(addr), sizeof(addr))<0?0:PortNumber;
	}
	return(PortNum);
}

void ILibParseUri(char* URI, char** IP, unsigned short* Port, char** Path)
{
	struct parser_result *result,*result2,*result3;
	char *TempString,*TempString2;
	int TempStringLength,TempStringLength2;
	
	result = ILibParseString(URI, 0, (int)strlen(URI), "://", 3);
	TempString = result->LastResult->data;
	TempStringLength = result->LastResult->datalength;
	
	/* Parse Path */
	result2 = ILibParseString(TempString,0,TempStringLength,"/",1);
	TempStringLength2 = TempStringLength-result2->FirstResult->datalength;
	*Path = (char*)MALLOC(TempStringLength2+1);
	memcpy(*Path,TempString+(result2->FirstResult->datalength),TempStringLength2);
	(*Path)[TempStringLength2] = '\0';
	
	/* Parse Port Number */
	result3 = ILibParseString(result2->FirstResult->data,0,result2->FirstResult->datalength,":",1);
	if(result3->NumResults==1)
	{
		*Port = 80;
	}
	else
	{
		TempString2 = (char*)MALLOC(result3->LastResult->datalength+1);
		memcpy(TempString2,result3->LastResult->data,result3->LastResult->datalength);
		TempString2[result3->LastResult->datalength] = '\0';
		*Port = (unsigned short)atoi(TempString2);
		FREE(TempString2);
	}
	/* Parse IP Address */
	TempStringLength2 = result3->FirstResult->datalength;
	*IP = (char*)MALLOC(TempStringLength2+1);
	memcpy(*IP,result3->FirstResult->data,TempStringLength2);
	(*IP)[TempStringLength2] = '\0';
	ILibDestructParserResults(result3);
	ILibDestructParserResults(result2);
	ILibDestructParserResults(result);
}
struct packetheader *ILibCreateEmptyPacket()
{
	struct packetheader *RetVal = (struct packetheader*)MALLOC(sizeof(struct packetheader));
	memset(RetVal,0,sizeof(struct packetheader));
	
	RetVal->UserAllocStrings = -1;
	RetVal->StatusCode = -1;
	RetVal->Version = "1.0";
	RetVal->VersionLength = 3;
	
	return(RetVal);
}
struct packetheader* ILibClonePacket(struct packetheader *packet)
{
	struct packetheader *RetVal = ILibCreateEmptyPacket();
	struct packetheader_field_node *n;
	
	RetVal->ReceivingAddress = packet->ReceivingAddress;
	
	ILibSetDirective(
	RetVal,
	packet->Directive,
	packet->DirectiveLength,
	packet->DirectiveObj,
	packet->DirectiveObjLength);
	ILibSetStatusCode(
	RetVal,
	packet->StatusCode,
	packet->StatusData,
	packet->StatusDataLength);
	ILibSetVersion(RetVal,packet->Version,packet->VersionLength);
	
	n = packet->FirstField;
	while(n!=NULL)
	{
		ILibAddHeaderLine(
		RetVal,
		n->Field,
		n->FieldLength,
		n->FieldData,
		n->FieldDataLength);
		n = n->NextField;
	}
	return(RetVal);
}
void ILibSetVersion(struct packetheader *packet, char* Version, int VersionLength)
{
	if(packet->UserAllocVersion!=0)	{FREE(packet->Version);}
	packet->UserAllocVersion=1;
	packet->Version = (char*)MALLOC(1+VersionLength);
	memcpy(packet->Version,Version,VersionLength);
	packet->Version[VersionLength] = '\0';
}
void ILibSetStatusCode(struct packetheader *packet, int StatusCode, char *StatusData, int StatusDataLength)
{
	packet->StatusCode = StatusCode;
	packet->StatusData = (char*)MALLOC(StatusDataLength+1);
	memcpy(packet->StatusData,StatusData,StatusDataLength);
	packet->StatusData[StatusDataLength] = '\0';
	packet->StatusDataLength = StatusDataLength;
}
void ILibSetDirective(struct packetheader *packet, char* Directive, int DirectiveLength, char* DirectiveObj, int DirectiveObjLength)
{
	packet->Directive = (char*)MALLOC(DirectiveLength+1);
	memcpy(packet->Directive,Directive,DirectiveLength);
	packet->Directive[DirectiveLength] = '\0';
	packet->DirectiveLength = DirectiveLength;
	
	packet->DirectiveObj = (char*)MALLOC(DirectiveObjLength+1);
	memcpy(packet->DirectiveObj,DirectiveObj,DirectiveObjLength);
	packet->DirectiveObj[DirectiveObjLength] = '\0';
	packet->DirectiveObjLength = DirectiveObjLength;
	packet->UserAllocStrings = -1;
}
void ILibAddHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength, char* FieldData, int FieldDataLength)
{
	struct packetheader_field_node *node;
	
	node = (struct packetheader_field_node*)MALLOC(sizeof(struct packetheader_field_node));
	node->UserAllocStrings = -1;
	node->Field = (char*)MALLOC(FieldNameLength+1);
	memcpy(node->Field,FieldName,FieldNameLength);
	node->Field[FieldNameLength] = '\0';
	node->FieldLength = FieldNameLength;
	
	node->FieldData = (char*)MALLOC(FieldDataLength+1);
	memcpy(node->FieldData,FieldData,FieldDataLength);
	node->FieldData[FieldDataLength] = '\0';
	node->FieldDataLength = FieldDataLength;
	
	node->NextField = NULL;
	
	if(packet->LastField!=NULL)
	{
		packet->LastField->NextField = node;
		packet->LastField = node;
	}
	else
	{
		packet->LastField = node;
		packet->FirstField = node;
	}
}
char* ILibGetHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength)
{
	char* RetVal = NULL;
	struct packetheader_field_node *node = packet->FirstField;
	int i;
	
	while(node!=NULL)
	{
		if(strncasecmp(FieldName,node->Field,FieldNameLength)==0)
		{
			RetVal = (char*)MALLOC(node->FieldDataLength+1);
			
			for(i=0;i<node->FieldDataLength;++i)
			{
				if(node->FieldData[i]!=' ') {break;}
			}
			if(i==node->FieldDataLength-1) {i = 0;}
			memcpy(RetVal,node->FieldData+i,node->FieldDataLength-i);
			RetVal[node->FieldDataLength-i] = '\0';
			break;
		}
		node = node->NextField;
	}
	
	return(RetVal);
}

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/* encode 3 8-bit binary bytes as 4 '6-bit' characters */
void ILibencodeblock( unsigned char in[3], unsigned char out[4], int len )
{
	out[0] = cb64[ in[0] >> 2 ];
	out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
	out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
	out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

/* Base64 encode a stream adding padding and line breaks as per spec. */
int ILibBase64Encode(unsigned char* input, const int inputlen, unsigned char** output)
{
	unsigned char* out;
	unsigned char* in;
	
	*output = (unsigned char*)MALLOC(((inputlen * 4) / 3) + 5);
	out = *output;
	in  = input;
	
	if (input == NULL || inputlen == 0)
	{
		*output = NULL;
		return 0;
	}
	
	while ((in+3) <= (input+inputlen))
	{
		ILibencodeblock(in, out, 3);
		in += 3;
		out += 4;
	}
	if ((input+inputlen)-in == 1)
	{
		ILibencodeblock(in, out, 1);
		out += 4;
	}
	else
	if ((input+inputlen)-in == 2)
	{
		ILibencodeblock(in, out, 2);
		out += 4;
	}
	*out = 0;
	
	return (int)(out-*output);
}

/* Decode 4 '6-bit' characters into 3 8-bit binary bytes */
void ILibdecodeblock( unsigned char in[4], unsigned char out[3] )
{
	out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
	out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
	out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

/* decode a base64 encoded stream discarding padding, line breaks and noise */
int ILibBase64Decode(unsigned char* input, const int inputlen, unsigned char** output)
{
	unsigned char* inptr;
	unsigned char* out;
	unsigned char v;
	unsigned char in[4];
	int i, len;
	
	if (input == NULL || inputlen == 0)
	{
		*output = NULL;
		return 0;
	}
	
	*output = (unsigned char*)MALLOC(((inputlen * 3) / 4) + 4);
	out = *output;
	inptr = input;
	
	while( inptr <= (input+inputlen) )
	{
		for( len = 0, i = 0; i < 4 && inptr <= (input+inputlen); i++ )
		{
			v = 0;
			while( inptr <= (input+inputlen) && v == 0 ) {
				v = (unsigned char) *inptr;
				inptr++;
				v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
				if( v ) {
					v = (unsigned char) ((v == '$') ? 0 : v - 61);
				}
			}
			if( inptr <= (input+inputlen) ) {
				len++;
				if( v ) {
					in[ i ] = (unsigned char) (v - 1);
				}
			}
			else {
				in[i] = 0;
			}
		}
		if( len )
		{
			ILibdecodeblock( in, out );
			out += len-1;
		}
	}
	*out = 0;
	return (int)(out-*output);
}

int ILibInPlaceXmlUnEscape(char* data)
{
	char* end = data+strlen(data);
	char* i = data;              /* src */
	char* j = data;              /* dest */
	while (j < end)
	{
		if (j[0] == '&' && j[1] == 'q' && j[2] == 'u' && j[3] == 'o' && j[4] == 't' && j[5] == ';')   // &quot;
		{
			i[0] = '"';
			j += 5;
		}
		else if (j[0] == '&' && j[1] == 'a' && j[2] == 'p' && j[3] == 'o' && j[4] == 's' && j[5] == ';')   // &apos;
		{
			i[0] = '\'';
			j += 5;
		}
		else if (j[0] == '&' && j[1] == 'a' && j[2] == 'm' && j[3] == 'p' && j[4] == ';')   // &amp;
		{
			i[0] = '&';
			j += 4;
		}
		else if (j[0] == '&' && j[1] == 'l' && j[2] == 't' && j[3] == ';')   // &lt;
		{
			i[0] = '<';
			j += 3;
		}
		else if (j[0] == '&' && j[1] == 'g' && j[2] == 't' && j[3] == ';')   // &gt;
		{
			i[0] = '>';
			j += 3;
		}
		else
		{
			i[0] = j[0];
		}
		i++;
		j++;
	}
	i[0] = '\0';
	return (int)(i - data);
}

int ILibXmlEscapeLength(const char* data)
{
	int i = 0, j = 0;
	while (data[i] != 0)
	{
		switch (data[i])
		{
			case '"':
			j += 6;
			break;
			case '\'':
			j += 6;
			break;
			case '<':
			j += 4;
			break;
			case '>':
			j += 4;
			break;
			case '&':
			j += 5;
			break;
			default:
			j++;
		}
		i++;
	}
	return j;
}

int ILibXmlEscape(char* outdata, const char* indata)
{
	int i=0;
	int inlen;
	char* out;
	
	out = outdata;
	inlen = (int)strlen(indata);
	
	for (i=0; i < inlen; i++)
	{
		if (indata[i] == '"')
		{
			memcpy(out, "&quot;", 6);
			out = out + 6;
		}
		else
		if (indata[i] == '\'')
		{
			memcpy(out, "&apos;", 6);
			out = out + 6;
		}
		else
		if (indata[i] == '<')
		{
			memcpy(out, "&lt;", 4);
			out = out + 4;
		}
		else
		if (indata[i] == '>')
		{
			memcpy(out, "&gt;", 4);
			out = out + 4;
		}
		else
		if (indata[i] == '&')
		{
			memcpy(out, "&amp;", 5);
			out = out + 5;
		}
		else
		{
			out[0] = indata[i];
			out++;
		}
	}
	
	out[0] = 0;
	
	return (int)(out - outdata);
}

void ILibLifeTime_Add(void *LifetimeMonitorObject,void *data, int seconds, void* Callback, void* Destroy)
{
	int NeedUnBlock = 0;
	struct timeval tv;
	struct LifeTimeMonitorData *temp;
	struct LifeTimeMonitorData *ltms = (struct LifeTimeMonitorData*)MALLOC(sizeof(struct LifeTimeMonitorData));
	struct ILibLifeTime *UPnPLifeTime = (struct ILibLifeTime*)LifetimeMonitorObject;
	
	gettimeofday(&tv,NULL);
	
	ltms->data = data;
	ltms->ExpirationTick = tv.tv_sec + seconds;
	ltms->CallbackPtr = Callback;
	ltms->DestroyPtr = Destroy;
	ltms->Next = NULL;
	ltms->Prev = NULL;
	
	sem_wait(&(UPnPLifeTime->SyncLock));
	if(UPnPLifeTime->LM==NULL)
	{
		UPnPLifeTime->LM = ltms;
		NeedUnBlock = 1;
	}
	else
	{
		temp = UPnPLifeTime->LM;
		while(temp!=NULL)
		{
			if(ltms->ExpirationTick<=temp->ExpirationTick)
			{
				ltms->Next = temp;
				if(temp->Prev==NULL)
				{
					UPnPLifeTime->LM=ltms;
					temp->Prev = ltms;
					NeedUnBlock = 1;
				}
				else
				{
					ltms->Prev = temp->Prev;
					temp->Prev->Next = ltms;
					temp->Prev = ltms;
				}
				break;
			}
			else if(temp->Next == NULL)
			{
				ltms->Next = NULL;
				ltms->Prev = temp;
				temp->Next = ltms;
				break;
			}
			temp = temp->Next;
		}
	}
	if(NeedUnBlock!=0) {ILibForceUnBlockChain(UPnPLifeTime->Chain);}
	sem_post(&(UPnPLifeTime->SyncLock));
}

void ILibLifeTime_Check(void *LifeTimeMonitorObject,fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime)
{
	struct timeval tv;
	long CurrentTick;
	struct LifeTimeMonitorData *Temp,*EVT,*Last=NULL;
	struct ILibLifeTime *UPnPLifeTime = (struct ILibLifeTime*)LifeTimeMonitorObject;
	int nexttick;
	
	EVT = NULL;
	sem_wait(&(UPnPLifeTime->SyncLock));
	if(UPnPLifeTime->LM!=NULL)
	{
		gettimeofday(&tv,NULL);
		CurrentTick = tv.tv_sec;
		Temp = UPnPLifeTime->LM;
		while(Temp!=NULL && Temp->ExpirationTick<=CurrentTick)
		{
			EVT = UPnPLifeTime->LM;
			Last = Temp;
			Temp = Temp->Next;
		}
		if(EVT != NULL)
		{
			if(Temp!=NULL)
			{
				UPnPLifeTime->LM = Temp;
				if(UPnPLifeTime->LM!=NULL)
				{
					UPnPLifeTime->LM->Prev = NULL;
				}
				Last->Next = NULL;
			}
			else
			{
				UPnPLifeTime->LM = NULL;
			}
		}
		sem_post(&(UPnPLifeTime->SyncLock));
		
		while(EVT!=NULL)
		{
			EVT->CallbackPtr(EVT->data);
			Temp = EVT;
			EVT = EVT->Next;
			FREE(Temp);
		}
		
		if(UPnPLifeTime->LM!=NULL)
		{
			nexttick = UPnPLifeTime->LM->ExpirationTick-CurrentTick;
			if(nexttick<*blocktime) {*blocktime=nexttick;}
		}
	}
	else
	{
		sem_post(&(UPnPLifeTime->SyncLock));
	}
}

void ILibLifeTime_Remove(void *LifeTimeToken, void *data)
{
	struct ILibLifeTime *UPnPLifeTime = (struct ILibLifeTime*)LifeTimeToken;
	struct LifeTimeMonitorData *first,*last,*evt;
	
	evt = last = NULL;
	sem_wait(&(UPnPLifeTime->SyncLock));
	
	first = UPnPLifeTime->LM;
	while(first!=NULL)
	{
		if(first->data==data)
		{
			if(first->Prev==NULL)
			{
				UPnPLifeTime->LM = first->Next;
				if(UPnPLifeTime->LM!=NULL)
				{
					UPnPLifeTime->LM->Prev = NULL;
				}
			}
			else
			{
				first->Prev->Next = first->Next;
				if(first->Next!=NULL)
				{
					first->Next->Prev = first->Prev;
				}
			}
			if(evt==NULL)
			{
				evt = last = first;
				evt->Prev = evt->Next = NULL;
			}
			else
			{
				last->Next = first;
				first->Prev = last;
				first->Next = NULL;
				last = first;
			}
		}
		first = first->Next;
	}
	sem_post(&(UPnPLifeTime->SyncLock));
	while(evt!=NULL)
	{
		first = evt->Next;
		if(evt->DestroyPtr!=NULL) {evt->DestroyPtr(evt->data);}
		FREE(evt);
		evt = first;
	}
}
void ILibLifeTime_Flush(void *LifeTimeToken)
{
	struct ILibLifeTime *UPnPLifeTime = (struct ILibLifeTime*)LifeTimeToken;
	struct LifeTimeMonitorData *temp,*temp2;
	
	sem_wait(&(UPnPLifeTime->SyncLock));
	
	temp = UPnPLifeTime->LM;
	UPnPLifeTime->LM = NULL;
	sem_post(&(UPnPLifeTime->SyncLock));
	
	while(temp!=NULL)
	{
		temp2 = temp->Next;
		if(temp->DestroyPtr!=NULL) {temp->DestroyPtr(temp->data);}
		FREE(temp);
		temp = temp2;
	}
}
void ILibLifeTime_Destroy(void *LifeTimeToken)
{
	struct ILibLifeTime *UPnPLifeTime = (struct ILibLifeTime*)LifeTimeToken;
	ILibLifeTime_Flush(LifeTimeToken);
	sem_destroy(&(UPnPLifeTime->SyncLock));
}
void *ILibCreateLifeTime(void *Chain)
{
	struct ILibLifeTime *RetVal = (struct ILibLifeTime*)MALLOC(sizeof(struct ILibLifeTime));
	RetVal->LM = NULL;
	RetVal->PreSelect = &ILibLifeTime_Check;
	RetVal->PostSelect = NULL;
	RetVal->Destroy = &ILibLifeTime_Destroy;
	RetVal->Chain = Chain;
	sem_init(&(RetVal->SyncLock),0,1);
	ILibAddToChain(Chain,RetVal);
	return((void*)RetVal);
}
int ILibFindEntryInTable(char *Entry, char **Table)
{
	int i = 0;
	
	while(Table[i]!=NULL)
	{
		if(strcmp(Entry,Table[i])==0)
		{
			return(i);
		}
		++i;
	}
	
	return(-1);
}
