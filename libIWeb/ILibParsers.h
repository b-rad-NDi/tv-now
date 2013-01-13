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

#ifndef __ILibParsers__
#define __ILibParsers__

#include <sys/socket.h>

#define UPnPMIN(a,b) (((a)<(b))?(a):(b))
#define MALLOC malloc
#define FREE free
#define ILibIsChainBeingDestroyed(Chain) (*((int*)Chain))

struct parser_result_field
{
	char* data;
	int datalength;
	struct parser_result_field *NextResult;
};
struct parser_result
{
	struct parser_result_field *FirstResult;
	struct parser_result_field *LastResult;
	int NumResults;
};
struct packetheader_field_node
{
	char* Field;
	int FieldLength;
	char* FieldData;
	int FieldDataLength;
	int UserAllocStrings;
	struct packetheader_field_node* NextField;
};
struct packetheader
{
	char* Directive;
	int DirectiveLength;
	char* DirectiveObj;
	int DirectiveObjLength;
	int StatusCode;
	char* StatusData;
	int StatusDataLength;
	char* Version;
	int VersionLength;
	char* Body;
	int BodyLength;
	int UserAllocStrings;
	int UserAllocVersion;
	
	struct packetheader_field_node* FirstField;
	struct packetheader_field_node* LastField;
	struct sockaddr_in *Source;
	int ReceivingAddress;
};
struct ILibXMLNode
{
	char* Name;
	int NameLength;
	
	char* NSTag;
	int NSLength;
	int StartTag;
	int EmptyTag;
	
	void *Reserved;
	struct ILibXMLNode *Next;
	struct ILibXMLNode *Parent;
	struct ILibXMLNode *Peer;
	struct ILibXMLNode *ClosingTag;
};
struct ILibXMLAttribute
{
	char* Name;
	int NameLength;
	
	char* Prefix;
	int PrefixLength;
	
	char* Value;
	int ValueLength;
	struct ILibXMLAttribute *Next;
};

int ILibFindEntryInTable(char *Entry, char **Table);

/* Stack Methods */
void ILibCreateStack(void **TheStack);
void ILibPushStack(void **TheStack, void *data);
void *ILibPopStack(void **TheStack);
void *ILibPeekStack(void **TheStack);
void ILibClearStack(void **TheStack);

/* Queue Methods */
void *ILibQueue_Create();
void ILibQueue_Destroy(void *q);
int ILibQueue_IsEmpty(void *q);
void ILibQueue_EnQueue(void *q, void *data);
void *ILibQueue_DeQueue(void *q);
void *ILibQueue_PeekQueue(void *q);

/* XML Parsing Methods */
int ILibReadInnerXML(struct ILibXMLNode *node, char **RetVal);
struct ILibXMLNode *ILibParseXML(char *buffer, int offset, int length);
struct ILibXMLAttribute *ILibGetXMLAttributes(struct ILibXMLNode *node);
int ILibProcessXMLNodeList(struct ILibXMLNode *nodeList);
void ILibDestructXMLNodeList(struct ILibXMLNode *node);
void ILibDestructXMLAttributeList(struct ILibXMLAttribute *attribute);

/* Chaining Methods */
void *ILibCreateChain();
void ILibAddToChain(void *chain, void *object);
void ILibStartChain(void *chain);
void ILibStopChain(void *chain);
void ILibForceUnBlockChain(void *Chain);

/* HashTree Methods */
void* ILibInitHashTree();
void ILibDestroyHashTree(void *tree);
int ILibHasEntry(void *hashtree, char* key, int keylength);
void ILibAddEntry(void* hashtree, char* key, int keylength, void *value);
void* ILibGetEntry(void *hashtree, char* key, int keylength);
void ILibDeleteEntry(void *hashtree, char* key, int keylength);
void *ILibHashTree_GetEnumerator(void *tree);
void ILibHashTree_DestroyEnumerator(void *tree_enumerator);
int ILibHashTree_MoveNext(void *tree_enumerator);
void ILibHashTree_GetValue(void *tree_enumerator, char **key, int *keyLength, void **data);
void ILibHashTree_Lock(void *hashtree);
void ILibHashTree_UnLock(void *hashtree);

/* LifeTimeMonitor Methods */
void ILibLifeTime_Add(void *LifetimeMonitorObject,void *data, int seconds, void* Callback, void* Destroy);
void ILibLifeTime_Remove(void *LifeTimeToken, void *data);
void ILibLifeTime_Flush();
void *ILibCreateLifeTime(void *Chain);

/* String Parsing Methods */
struct parser_result* ILibParseString(char* buffer, int offset, int length, char* Delimiter, int DelimiterLength);
struct parser_result* ILibParseStringAdv(char* buffer, int offset, int length, char* Delimiter, int DelimiterLength);
void ILibDestructParserResults(struct parser_result *result);
void ILibParseUri(char* URI, char** IP, unsigned short* Port, char** Path);
int ILibGetLong(char *TestValue, int TestValueLength, long* NumericValue);
int ILibGetULong(const char *TestValue, const int TestValueLength, unsigned long* NumericValue);

/* Packet Methods */
struct packetheader *ILibCreateEmptyPacket();
void ILibAddHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength, char* FieldData, int FieldDataLength);
char* ILibGetHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength);
void ILibSetVersion(struct packetheader *packet, char* Version, int VersionLength);
void ILibSetStatusCode(struct packetheader *packet, int StatusCode, char* StatusData, int StatusDataLength);
void ILibSetDirective(struct packetheader *packet, char* Directive, int DirectiveLength, char* DirectiveObj, int DirectiveObjLength);
void ILibDestructPacket(struct packetheader *packet);
struct packetheader* ILibParsePacketHeader(char* buffer, int offset, int length);
int ILibGetRawPacket(struct packetheader *packet,char **buffer);
struct packetheader* ILibClonePacket(struct packetheader *packet);

/* Network Helper Methods */
int ILibGetLocalIPAddressList(int** pp_int);
unsigned short ILibGetDGramSocket(int local, int *TheSocket);
unsigned short ILibGetStreamSocket(int local, unsigned short PortNumber,int *TheSocket);

void* dbg_malloc(int sz);
void dbg_free(void* ptr);
int dbg_GetCount();

/* XML escaping methods */
int ILibXmlEscape(char* outdata, const char* indata);
int ILibXmlEscapeLength(const char* data);
int ILibInPlaceXmlUnEscape(char* data);

/* Base64 handling methods */
int ILibBase64Encode(unsigned char* input, const int inputlen, unsigned char** output);
int ILibBase64Decode(unsigned char* input, const int inputlen, unsigned char** output);

/* Compression Handling Methods */
char* ILibDecompressString(unsigned char* CurrentCompressed, const int bufferLength, const int DecompressedLength);

#endif
