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
#include <string.h>
#include "MyString.h"

#ifdef WIN32
#include <crtdbg.h>
#endif

#ifdef UNDER_CE
#define strnicmp _strnicmp
#endif

#ifdef _POSIX
#define strnicmp strncasecmp
#endif

int EndsWith(const char* str, const char* endsWith, int ignoreCase)
{
	int strLen, ewLen, offset;
	int cmp = 0;

	strLen = (int) strlen(str);
	ewLen = (int) strlen(endsWith);

	if (ewLen > strLen)
	{
		return 0;
	}

	offset = strLen - ewLen;

	if (ignoreCase != 0)
	{
		cmp = strnicmp(str+offset, endsWith, ewLen);
	}
	else
	{
		cmp = strncmp(str+offset, endsWith, ewLen);
	}

	if (cmp == 0)
	{
		return 1;
	}

	return 0;
}

int IndexOf(const char* str, const char* findThis)
{
	int i,j, strLen, ftLen;
	
	strLen = (int) strlen(str);
	ftLen = (int) strlen(findThis);

	if (ftLen <= strLen)
	{
		for (i=0; i < strLen; i++)
		{
			for (j=0; j < ftLen; j++)
			{
				int ij = i+j;
				if (str[ij] != findThis[j])
				{
					break;
				}
			}
			if (j == ftLen)
			{
				return i;
			}
		}
	}

	return -1;
}

int LastIndexOf(const char* str, const char* findThis)
{
	int i,j,strLen,ftLen;

	strLen = (int) strlen(str);
	ftLen = (int) strlen(findThis);

	if (ftLen <= strLen)
	{
		for (i=strLen-ftLen; i >= 0; i--)
		{
			for (j=0; j < ftLen; j++)
			{
				int ij = i+j;
				if (str[ij] != findThis[j])
				{
					break;
				}
			}
			if (j == ftLen)
			{
				return i;
			}
		}
	}

	return -1;
}

int StartsWith(const char* str, const char* startsWith, int ignoreCase)
{
	int cmp;

	if (ignoreCase != 0)
	{
		cmp = strnicmp(str, startsWith, (int) strlen(startsWith));
	}
	else
	{
		cmp = strncmp(str, startsWith, (int) strlen(startsWith));
	}

	if (cmp == 0)
	{
		return 1;
	}

	return 0;

}

int Utf8ToAnsi(char *dest, const char *src, int destLen)
{
	int di,si,si2,si3;
	int conv;

	conv = 0;
	di = 0;
	si = 0;
	destLen--;

	while ((src[si] != '\0') && (di < destLen))
	{
		si2 = si+1;
		si3 = si2+1;

		if ((unsigned char)src[si] <= 0x7F)
		{
			dest[di++] = src[si++];
			conv++;
		}
		else if ((unsigned char)src[si] <= 0xDF)
		{
			if (((unsigned char)src[si] & 0x1C) == 0)
			{
				dest[di++] = ((src[si] & 0x03) << 6) | (src[si2] & 0x3F);
				conv++;
			}
			si+=2;
		}
		else if ((unsigned char)src[si] <= 0xEF)
		{
			/* outside ansi char set */
			si+=3;
		}
		else if ((unsigned char)src[si] <= 0xF7)
		{
			/* outside ansi char set */
			si+=4;
		}
	}
	dest[di] = '\0';

	return conv;
}

int Utf8ToWide(wchar_t *dest, const char *src, int destLen)
{
	int di,si,si2,si3;
	int conv;

	conv = 0;
	di = 0;
	si = 0;
	destLen--;

	while ((src[si] != '\0') && (di < destLen))
	{
		si2 = si+1;
		si3 = si2+1;

		if ((unsigned char)src[si] <= 0x7F)
		{
			dest[di++] = src[si++];
			conv++;
		}
		else if ((unsigned char)src[si] <= 0xDF)
		{
			dest[di++] = ((src[si] & 0x1F) << 6) | (src[si2] & 0x3F);
			conv++;
			si+=2;
		}
		else if ((unsigned char)src[si] <= 0xEF)
		{
			dest[di++] = ((src[si] & 0x0F) << 12) | ((src[si2] & 0x3F) << 6) | (src[si3] & 0x3F);
			conv++;
			si+=3;
		}
		else if ((unsigned char)src[si] <= 0xF7)
		{
			/* code should never execute because it's for unicode characters greater than 16 bits */
			/*
			dest[di++] = ((src[si] & 0x07) << 18) | ((src[si2] & 0x3F) << 6) | ((src[si2] & 0x3F) << 6) | (src[si3] & 0x3F);
			conv++;
			 */
			si+=4;
		}
	}
	dest[di] = '\0';

	return conv;
}

/*
 *	isalnum() is not sufficient because locales can affect the return value
 *	digits:		0x30 - 0x39: 
 *	upper:		0x41 - 0x5A
 *	lower:		0x61 - 0x7A
 *	dot:		0x2E
 *	slash:		0x2F
 *	colon:		0x3A
 *	ampersand:	0x26
 *	underscore: 0x5F
 *	query:		0x3F
 */
#define _isUriPrintable(c) (((c>=0x30) && (c<=0x39)) || ((c>=0x41) && (c<=0x5A)) || ((c>=0x61) && (c<=0x7A)) || (c==0x2E) || (c==0x2F) || (c==0x3A) || (c==0x36) || (c==0x5F) || (c==0x3F))

int strUtf8Len(char *src, int isWide, int asEscapedUri)
{
	wchar_t wcv;		/* wide char scalar */
	int i,j;

	i = wcv = j = 0;
	
	do
	{
		wcv =  (unsigned char) src[i++];

		if (isWide != 0)
		{
			wcv |= (((unsigned char) src[i++]) << 8);
		}

		if (wcv < 0x80)
		{
			if ((asEscapedUri!=0) && (!(_isUriPrintable((char)wcv))))
			{
				j+=3;
			}
			else 
			{
				j++;
			}
		}
		else if (wcv < 0x0800)
		{
			if (asEscapedUri!=0)
			{
				j+=6;
			}
			else 
			{
				j+=2;
			}
		}
		else if (wcv < 0x10000)
		{
			if (asEscapedUri!=0)
			{
				j+=9;
			}
			else 
			{
				j+=3;
			}
		}
		else
		{
			/*
			 *	this code should never execute with unicode chars with
			 *	more than 2 bytes because the unicode scalar won't fit in 16 bits.
			 */
		}
	}
	while (wcv != 0);

	return j;
}

int _strToUtf8(char *dest, const char *src, int destSize, int isWide, int *charactersConverted, int uriEscape)
{
	wchar_t wcv;		/* wide char scalar */
	int i,j;
	int cc;			/* char count */
	int destLen;

	destLen = destSize - 1;
	cc = 0;
	i = 0;
	wcv = 0;
	j=0;

	
	do
	{
		wcv =  (unsigned char) src[i++];

		if (isWide != 0)
		{
			wcv |= (((unsigned char) src[i++]) << 8);
		}

		if ((wcv < 0x80) && (j < destLen))
		{
			if (uriEscape != 0)
			{
				if (_isUriPrintable((char)wcv))
				{
					dest[j++] = (char) wcv;
					cc++;
				}
				else if (j < destLen - 3)
				{
					j += sprintf(dest+j, "%%%x", wcv);
					cc++;
				}
			}
			else
			{
				dest[j++] = (char) wcv;
				cc++;
			}
		}
		else if ((wcv < 0x0800) && (j < destLen - 1))
		{
			if (uriEscape != 0)
			{
				if (j < destLen - 6)
				{
					j += sprintf(dest+j, "%%%x", ((wcv >> 6) | 0xC0));
					j += sprintf(dest+j, "%%%x", ((wcv & 0x3F) | 0x80));

					cc++;
				}
			}
			else
			{
				dest[j++] = (char)((wcv >> 6) | 0xC0);
				dest[j++] = (char)((wcv & 0x3F) | 0x80);
				cc++;
			}
		}
		else if ((wcv < 0x10000) && (j < destLen - 2))
		{
			if (uriEscape != 0)
			{
				if (j < destLen - 9)
				{
					j += sprintf(dest+j, "%%%x", ((wcv >> 12) | 0xF0));
					j += sprintf(dest+j, "%%%x", (((wcv >> 6) & 0x3F) | 0x80));
					j += sprintf(dest+j, "%%%x", ((wcv & 0x3F) | 0x80));

					cc++;
				}
			}
			else
			{
				dest[j++] = (char)((wcv >> 12) | 0xF0);
				dest[j++] = (char)(((wcv >> 6) & 0x3F) | 0x80);
				dest[j++] = (char)((wcv & 0x3F) | 0x80);
				cc++;
			}
		}
		else if (j < destLen - 3)
		{
			/*
			 *	this code should never execute with unicode chars with
			 *	more than 2 bytes because the unicode scalar won't fit in 16 bits.
			 *
			dest[j++] = (char)(wcv >> 18 | 0xF0);
			dest[j++] = (char)(wcv >> 12 & 0x3F | 0x80);
			dest[j++] = (char)(wcv >> 6 & 0x3F | 0x80);
			dest[j++] = (char)(wcv & 0x3F | 0x80);
			cc++;
			 */
		}
		else
		{
			/* no more room in dest buffer */
			break;
		}
	}
	while (wcv != 0);


	if (wcv == 0)
	{
		j--;
		if (charactersConverted != NULL)
		{
			*charactersConverted = cc - 1;
		}
	}
	else
	{
		dest[j] = '\0';
		if (charactersConverted != NULL)
		{
			*charactersConverted = cc;
		}
	}
	return j;
}

int strToUtf8(char *dest, const char *src, int destSize, int isWide, int *charactersConverted)
{
	return _strToUtf8(dest, src, destSize, isWide, charactersConverted, 0);
}

int strToEscapedUri(char *dest, const char *src, int destSize, int isWide, int *charactersConverted)
{
	return _strToUtf8(dest, src, destSize, isWide, charactersConverted, 1);
}
