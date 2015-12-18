#pragma once

#include "yaml.h"

#define SS_YAML_KEY_FILEHDR "File_hdr"
#define SS_YAML_KEY_TAG "Tag"
#define SS_YAML_KEY_VERSION "Version"
#define SS_YAML_KEY_UNITHDR "Unit_hdr"
#define SS_YAML_KEY_TYPE "Type"
#define SS_YAML_KEY_APPLE2TYPE "Apple2Type"
#define SS_YAML_VALUE_AWSS "AWSS"

typedef std::map<std::string, std::string> MapYaml;

class YamlHelper
{
public:
	YamlHelper() :
		m_hFile(NULL)
	{
		MakeAsciiToHexTable();
	}

	~YamlHelper()
	{
		if (m_hFile)
			fclose(m_hFile);
	}

	int InitParser(const char* pPathname);
	void FinaliseParser(void);
	int GetScalar(std::string& scalar);

	//

	class YamlMap
	{
	public:
		YamlMap(YamlHelper& yamlHelper)
			: m_yamlHelper(yamlHelper)
		{
			if (!m_yamlHelper.ParseMap())
				throw std::string(m_yamlHelper.GetMapName() + ": Failed to parse map");
		}

		~YamlMap()
		{
			m_yamlHelper.GetMapRemainder();
		}

		UINT GetMapValueUINT(const std::string key)
		{
			std::string value = m_yamlHelper.GetMapValue(key);
			if (value == "") throw std::string(m_yamlHelper.GetMapName() + ": Missing: " + key);
			return strtoul(value.c_str(), NULL, 16);
		}

		UINT64 GetMapValueUINT64(const std::string key)
		{
			std::string value = m_yamlHelper.GetMapValue(key);
			if (value == "") throw std::string(m_yamlHelper.GetMapName() + ": Missing: " + key);
			return _strtoui64(value.c_str(), NULL, 16);
		}

		std::string GetMapValueSTRING(const std::string key)
		{
			std::string value = m_yamlHelper.GetMapValue(key);
			if (value == "") throw std::string(m_yamlHelper.GetMapName() + ": Missing: " + key);
			return value;
		}

		void GetMapValueMemory(const LPBYTE pMemBase)
		{
			m_yamlHelper.GetMapValueMemory(pMemBase);
		}

	private:
		YamlHelper& m_yamlHelper;
	};

private:
	std::string GetMapName(void) { return m_mapName; }
	int ParseMap(void);
	std::string GetMapValue(const std::string key);
	void GetMapValueMemory(const LPBYTE pMemBase);
	void GetMapRemainder(void);
	void MakeAsciiToHexTable(void);

	yaml_parser_t m_parser;
	yaml_event_t m_newEvent;

	std::string m_scalarName;

	MapYaml m_mapYaml;
	std::string m_mapName;

	FILE* m_hFile;

	char m_AsciiToHex[256];
};

// -----

class YamlSaveHelper
{
public:
	YamlSaveHelper(std::string pathname) :
		m_hFile(NULL)
	{
		m_hFile = fopen(pathname.c_str(), "wt");

		// todo: handle ERROR_ALREADY_EXISTS - ask if user wants to replace existing file
		// - at this point any old file will have been truncated to zero

		if(m_hFile == NULL)
			throw std::string("Save error");

		fprintf(m_hFile, "---\n");
	}

	~YamlSaveHelper()
	{
		if (m_hFile)
		{
			fprintf(m_hFile, "...\n");
			fclose(m_hFile);
		}
	}

	FILE* GetFile(void) { return m_hFile; }

private:
	FILE* m_hFile;
};
