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

#ifndef MY_STRING_H

#include <ctype.h>
#include <stdlib.h>

/*
 *	EndsWith()
 *		str					: the string to analyze
 *		endsWith			: the token to find at the end of str
 *
 *	If "str" ends with "endsWith", then we return nonzero.
 */
int EndsWith(const char* str, const char* endsWith, int ignoreCase);

/*
 *	IndexOf()
 *		str					: the string to analyze
 *		findThis			: the token to find 
 *
 *	Returns the first index where findThis can be found in str.
 *	Returns -1 if not found.
 */
int IndexOf(const char* str, const char* findThis);

int LastIndexOf(const char* str, const char* findThis);

int StartsWith(const char* str, const char* startsWith, int ignoreCase);

int Utf8ToAnsi(char *dest, const char *src, int destLen);
int Utf8ToWide(wchar_t *dest, const char *src, int destLen);
int strToUtf8(char *dest, const char *src, int destSize, int isWide, int *charactersConverted);

int strUtf8Len(char *src, int isWide, int asEscapedUri);

/*
 *	Stores UTF8-compliant escaped URI in 'dest'.
 *	Returns number of bytes used in dest.
 */
int strToEscapedUri(char *dest, const char *src, int destSize, int isWide, int *charactersConverted);

#endif 
