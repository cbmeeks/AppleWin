/*
AppleWin : An Apple //e emulator for Windows

Copyright (C) 1994-1996, Michael O'Brien
Copyright (C) 1999-2001, Oliver Schmidt
Copyright (C) 2002-2005, Tom Charlesworth
Copyright (C) 2006-2015, Tom Charlesworth, Michael Pohoreski

AppleWin is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

AppleWin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with AppleWin; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "StdAfx.h"

#include "YamlHelper.h"

int YamlHelper::InitParser(const char* pPathname)
{
	m_hFile = fopen(pPathname, "r");
	if (m_hFile == NULL)
	{
		return 0;
	}

	if (!yaml_parser_initialize(&m_parser))
	{
		return 0;
	}

	yaml_parser_set_input_file(&m_parser, m_hFile);

	return 1;
}

void YamlHelper::FinaliseParser(void)
{
	if (m_hFile)
		fclose(m_hFile);

	m_hFile = NULL;
}

int YamlHelper::GetScalar(std::string& scalar)
{
	int res = 1;
	bool bDone = false;

	while (!bDone)
	{
		if (!yaml_parser_parse(&m_parser, &m_newEvent))
		{
			//printf("Parser error %d\n", m_parser.error);
			throw std::string("Parser error");
		}

		switch(m_newEvent.type)
		{
		case YAML_SCALAR_EVENT:
			scalar = m_scalarName = (const char*)m_newEvent.data.scalar.value;
			res = 1;
			bDone = true;
			break;
		case YAML_STREAM_END_EVENT:
			res = 0;
			bDone = true;
			break;
		}
	}

	return res;
}

int YamlHelper::ParseMap(void)
{
	m_mapYaml.clear();

	if (!yaml_parser_parse(&m_parser, &m_newEvent))
	{
		//printf("Parser error %d\n", m_parser.error);
		throw std::string("Parser error");
	}

	if (m_newEvent.type != YAML_MAPPING_START_EVENT)
	{
		//printf("Unexpected yaml event (%d)\n", m_newEvent.type);
		throw std::string("Unexpected yaml event");
	}

	//

	m_mapName = m_scalarName;
	const char*& pValue = (const char*&) m_newEvent.data.scalar.value;

	bool bKey = true;
	char* pKey = NULL;
	int res = 1;
	bool bDone = false;

	while (!bDone)
	{
		if (!yaml_parser_parse(&m_parser, &m_newEvent))
		{
			//printf("Parser error %d\n", m_parser.error);
			throw std::string("Parser error");
		}

		switch(m_newEvent.type)
		{
		case YAML_STREAM_END_EVENT:
			res = 0;
			bDone = true;
			break;
		case YAML_MAPPING_START_EVENT:
			break;
		case YAML_MAPPING_END_EVENT:
			bDone = true;
			break;
		case YAML_SCALAR_EVENT:
			if (bKey)
			{
				pKey = _strdup(pValue);
			}
			else
			{
				m_mapYaml[std::string(pKey)] = std::string(pValue);
				delete [] pKey; pKey = NULL;
			}

			bKey = bKey ? false : true;
			break;
		}
	}

	if (pKey)
		delete [] pKey;

	return res;
}

std::string YamlHelper::GetMapValue(const std::string key)
{
	MapYaml::const_iterator iter = m_mapYaml.find(key);
	if (iter == m_mapYaml.end())
		return "";	// not found

	std::string value = iter->second;

	m_mapYaml.erase(iter);

	return value;
}

void YamlHelper::GetMapRemainder(void)
{
	for (MapYaml::iterator it = m_mapYaml.begin(); it != m_mapYaml.end(); ++it)
	{
		const char* pKey = it->first.c_str();
		char szDbg[100];
		sprintf(szDbg, "%s: Unknown key (%s)\n", m_mapName.c_str(), pKey);
		OutputDebugString(szDbg);
	}

	m_mapYaml.clear();
}

void YamlHelper::MakeAsciiToHexTable(void)
{
	memset(m_AsciiToHex, -1, sizeof(m_AsciiToHex));

	for (int i = '0'; i<= '9'; i++)
		m_AsciiToHex[i] = i - '0';

	for (int i = 'A'; i<= 'F'; i++)
		m_AsciiToHex[i] = i - 'A' + 0xA;

	for (int i = 'a'; i<= 'f'; i++)
		m_AsciiToHex[i] = i - 'a' + 0xA;
}

void YamlHelper::GetMapValueMemory(const LPBYTE pMemBase)
{
	const UINT kAddrSpaceSize = 64*1024;

	for (MapYaml::iterator it = m_mapYaml.begin(); it != m_mapYaml.end(); ++it)
	{
		const char* pKey = it->first.c_str();
		UINT addr = strtoul(pKey, NULL, 16);
		if (addr >= kAddrSpaceSize)
			throw std::string("Memory: line address too big: " + it->first);

		LPBYTE pDst = (LPBYTE) (pMemBase + addr);
		const LPBYTE pDstEnd = (LPBYTE) (pMemBase + kAddrSpaceSize);

		const char* pValue = it->second.c_str();
		size_t len = strlen(pValue);
		if (len & 1)
			throw std::string("Memory: hex data must be an even number of nibbles on line address: " + it->first);

		for (UINT i = 0; i<len; i+=2)
		{
			if (pDst >= pDstEnd)
				throw std::string("Memory: hex data overflowed 64K address space on line address: " + it->first);

			BYTE ah = m_AsciiToHex[ (BYTE)(*pValue++) ];
			BYTE al = m_AsciiToHex[ (BYTE)(*pValue++) ];
			if ((ah | al) & 0x80)
				throw std::string("Memory: hex data contains illegal character on line address: " + it->first);

			*pDst++ = (ah<<4) | al;
		}
	}

	m_mapYaml.clear();
}
