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

#ifndef __UpnpMicrostack__
#define __UpnpMicrostack__


/*
*
*	Target Platform = POSIX
*
*	HTTP Mode = 1.1
*	IPAddressMonitoring = YES
*
*/


struct UpnpDataObject;
struct packetheader;

/* These methods must be implemented by the user */
extern void UpnpConnectionManager_GetCurrentConnectionInfo(void* upnptoken,int ConnectionID);
extern void UpnpConnectionManager_GetProtocolInfo(void* upnptoken);
extern void UpnpConnectionManager_GetCurrentConnectionIDs(void* upnptoken);
extern void UpnpContentDirectory_Browse(void* upnptoken,char* ObjectID,char* BrowseFlag,char* Filter,unsigned int StartingIndex,unsigned int RequestedCount,char* SortCriteria);
extern void UpnpContentDirectory_GetSortCapabilities(void* upnptoken);
extern void UpnpContentDirectory_GetSystemUpdateID(void* upnptoken);
extern void UpnpContentDirectory_GetSearchCapabilities(void* upnptoken);
extern void UpnpPresentationRequest(void* upnptoken, struct packetheader *packet);

/* UPnP Stack Management */
void *UpnpCreateMicroStack(void *Chain, const char* FriendlyName, const char* UDN, const char* SerialNumber, const int NotifyCycleSeconds, const unsigned short PortNum);
void UpnpIPAddressListChanged(void *MicroStackToken);
int UpnpGetLocalPortNumber(void *token);
int   UpnpGetLocalInterfaceToHost(const void* UPnPToken);
void* UpnpGetWebServerToken(const void *MicroStackToken);

/* Invocation Response Methods */
void UpnpResponse_Error(const void* UPnPToken, const int ErrorCode, const char* ErrorMsg);
void UpnpResponseGeneric(const void* UPnPToken,const char* ServiceURI,const char* MethodName,const char* Params);
void UpnpAsyncResponse_START(const void* UPnPToken, const char* actionName, const char* serviceUrnWithVersion);
void UpnpAsyncResponse_DONE(const void* UPnPToken, const char* actionName);
void UpnpAsyncResponse_OUT(const void* UPnPToken, const char* outArgName, const char* bytes, const int byteLength, const int startArg, const int endArg);
void UpnpResponse_ContentDirectory_Browse(const void* UPnPToken, const char* Result, const unsigned int NumberReturned, const unsigned int TotalMatches, const unsigned int UpdateID);
void UpnpResponse_ContentDirectory_GetSortCapabilities(const void* UPnPToken, const char* SortCaps);
void UpnpResponse_ContentDirectory_GetSystemUpdateID(const void* UPnPToken, const unsigned int Id);
void UpnpResponse_ContentDirectory_GetSearchCapabilities(const void* UPnPToken, const char* SearchCaps);
void UpnpResponse_ConnectionManager_GetCurrentConnectionInfo(const void* UPnPToken, const int RcsID, const int AVTransportID, const char* ProtocolInfo, const char* PeerConnectionManager, const int PeerConnectionID, const char* Direction, const char* Status);
void UpnpResponse_ConnectionManager_GetProtocolInfo(const void* UPnPToken, const char* Source, const char* Sink);
void UpnpResponse_ConnectionManager_GetCurrentConnectionIDs(const void* UPnPToken, const char* ConnectionIDs);

/* State Variable Eventing Methods */
void UpnpSetState_ContentDirectory_SystemUpdateID(void *microstack,unsigned int val);
void UpnpSetState_ConnectionManager_SourceProtocolInfo(void *microstack,char* val);
void UpnpSetState_ConnectionManager_SinkProtocolInfo(void *microstack,char* val);
void UpnpSetState_ConnectionManager_CurrentConnectionIDs(void *microstack,char* val);

#endif
