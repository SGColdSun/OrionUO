/***********************************************************************************
**
** SpeechManager.cpp
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "SpeechManager.h"
#include "FileManager.h"
//----------------------------------------------------------------------------------
CSpeechManager g_SpeechManager;
//----------------------------------------------------------------------------------
//------------------------------------CLangCode-------------------------------------
//----------------------------------------------------------------------------------
CLangCode::CLangCode()
: m_Code(), m_LangString(), m_Unknown(0), m_LangName(), m_LangCountry()
{
}
//----------------------------------------------------------------------------------
CLangCode::~CLangCode()
{
}
//----------------------------------------------------------------------------------
//------------------------------------CSpeechItem-----------------------------------
//----------------------------------------------------------------------------------
CSpeechItem::CSpeechItem()
: m_Code(0), m_Data(L""), m_CheckStart(false), m_CheckEnd(false)
{
}
//----------------------------------------------------------------------------------
CSpeechItem::~CSpeechItem()
{
}
//----------------------------------------------------------------------------------
//-----------------------------------CSpeechManager---------------------------------
//----------------------------------------------------------------------------------
CSpeechManager::CSpeechManager()
{
}
//----------------------------------------------------------------------------------
CSpeechManager::~CSpeechManager()
{
	m_SpeechEntries.clear();
	m_LangCodes.clear();
}
//----------------------------------------------------------------------------------
/*!
�������� ������ �� Speech.mul
@return true ��� �������� ��������
*/
bool CSpeechManager::LoadSpeech()
{
	//�������� ���������.
	//if (!LoadLangCodes())
	//	return false;

	WISP_FILE::CMappedFile &file = g_FileManager.m_SpeechMul;

	while (!file.IsEOF())
	{
		CSpeechItem item;
		item.Code = file.ReadUInt16BE();
		WORD len = file.ReadUInt16BE();
		wstring str = DecodeUTF8(file.ReadString(len));
		//������� ������� ���� ��������.
		str = str.substr(0, str.length() - 2);
		const WCHAR *data = str.c_str();

		
		DFOR(i, str.length(), 1)
		{
			if (data[i])
			{
				if (data[i] == L'*')
				{
					item.CheckEnd = true;
					str.resize(i);
				}

				break;
			}
		}

		if (str.length() && *str.c_str() == L'*')
		{
			item.CheckStart = true;
			str.erase(str.begin());
		}

		item.Data = str;
		m_SpeechEntries.push_back(item);

		//TPRINT("[0x%04X]=(len=%i, cs=%i, ce=%i) %s\n", item.Code, len, item.CheckStart, item.CheckEnd, ToString(str).c_str());
	}

	return true;
}
//----------------------------------------------------------------------------------
/*!
�������� ������ �� Langcode.iff
@return true ��� �������� ��������
*/
bool CSpeechManager::LoadLangCodes()
{
	WISP_FILE::CMappedFile &file = g_FileManager.m_LangcodeIff;

	//������� ��������� �����
	file.ReadString(36);

	while (!file.IsEOF())
	{
		CLangCode langCodeData;

		langCodeData.Code = file.ReadString(4);

		DWORD entryLen = file.ReadUInt32BE();
		langCodeData.LangString = file.ReadString(0); 
		langCodeData.Unknown = file.ReadUInt32LE();
		langCodeData.LangName = file.ReadString(0);
		langCodeData.LangCountry = file.ReadString(0);

		//������ LangName � LangCountry + null terminator ������ �������� ������ ����������� � �����.
		if ((langCodeData.LangName.length() + langCodeData.LangCountry.length() + 2) % 2)
		{
			int nullTerminator = file.ReadUInt8();
			
			if (nullTerminator != 0) {
				throw "speechManager @ 138, invalid null terminator in langcodes.iff";
			}
		}

		m_LangCodes.push_back(langCodeData);
		//TPRINT("[0x%04X]=(len=%i, cs=%i, ce=%i) %s\n", item.Code, len, item.CheckStart, item.CheckEnd, ToString(str).c_str());
	}

	if (m_LangCodes.size() != 135)
		return false;

	return true;
}
//----------------------------------------------------------------------------------
void CSpeechManager::GetKeywords(const wchar_t *text, UINT_LIST &codes)
{
	int size = m_SpeechEntries.size();
	wstring input = ToLowerW(text);
	//to lower, case insensitive approach.

	IFOR(i, 0, size) 
	{
		CSpeechItem entry = m_SpeechEntries[i];
		wstring data = entry.Data;

		if (data.length() > input.length() || data.length() == 0)
			continue;

		if (!entry.CheckStart)
		{
			wstring start = input.substr(0, data.length());
			int hits = start.find(data);

			if (hits < 0)
				continue;
		}

		if (!entry.CheckEnd)
		{
			wstring end = input.substr(input.length() - data.length());
			int hits = end.find(data);

			if (hits < 0)
				continue;
		}

		int hits = input.find(data);

		if (hits > -1)
			codes.push_back(entry.Code);
	}
}
//---------------------------------------------------------------------------
