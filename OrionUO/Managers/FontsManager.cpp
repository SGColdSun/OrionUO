/***********************************************************************************
**
** FontsManager.cpp
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "FontsManager.h"
#include "../GLEngine/GLEngine.h"
#include "../Wisp/WispMappedFile.h"
#include "FileManager.h"
#include "../OrionUO.h"
#include "ColorManager.h"
//----------------------------------------------------------------------------------
CFontsManager g_FontManager;
//----------------------------------------------------------------------------------
CFontsManager::CFontsManager()
: m_SavePixels(false), m_UseHTML(false), m_Font(NULL), m_FontCount(0),
m_HTMLColor(0xFFFFFFFF), m_RecalculateWidthByInfo(false), m_UnusePartialHue(false)
{
	memset(m_UnicodeFontAddress, 0, sizeof(m_UnicodeFontAddress));
	memset(m_UnicodeFontSize, 0, sizeof(m_UnicodeFontSize));
}
//----------------------------------------------------------------------------------
CFontsManager::~CFontsManager()
{
	delete []m_Font;
	m_FontCount = 0;
	m_WebLink.clear();
}
//----------------------------------------------------------------------------------
/*!
�������� ASCII �������
@return true ��� �������� ��������
*/
bool CFontsManager::LoadFonts()
{
	WISP_FILE::CMappedFile &fontFile = g_FileManager.m_FontsMul;
	m_FontCount = 0;

	while (!fontFile.IsEOF())
	{
		bool exit = false;
		fontFile.Move(1);

		IFOR(i, 0, 224)
		{
			PFONT_HEADER fh = (PFONT_HEADER)fontFile.Ptr;
			fontFile.Move(sizeof(FONT_HEADER));

			int bcount = fh->Width * fh->Height * 2;

			if (fontFile.Ptr + bcount > fontFile.End) //Bad font file...
			{
				exit = true;

				break;
			}

			fontFile.Move(bcount);
		}

		if (exit)
			break;

		m_FontCount++;
	}

	if (m_FontCount < 1)
	{
		m_Font = NULL;
		m_FontCount = 0;

		return false;
	}

	m_Font = new FONT_DATA[m_FontCount];

	fontFile.ResetPtr();

	IFOR(i, 0, m_FontCount)
	{
		FONT_DATA &fd = m_Font[i];
		fd.Header = fontFile.ReadUInt8();

		IFOR(j, 0, 224)
		{
			PFONT_HEADER fh = (PFONT_HEADER)fontFile.Ptr;
			fontFile.Move(sizeof(FONT_HEADER));

			FONT_CHARACTER_DATA &fcd = fd.Chars[j];
			fcd.Width = fh->Width;
			fcd.Height = fh->Height;

			int bcount = fh->Width * fh->Height * 2;

			fcd.Data = (pushort)fontFile.Ptr;

			fontFile.Move(bcount);
		}
	}

	IFOR(i, 0, g_FileManager.UnicodeFontsCount)
	{
		m_UnicodeFontAddress[i] = (uint)g_FileManager.m_UnifontMul[i].Start;
		m_UnicodeFontSize[i] = g_FileManager.m_UnifontMul[i].Size;
	}

	return true;
}
//----------------------------------------------------------------------------------
BYTE CFontsManager::UnicodeFontExists(const uchar &font)
{
	if (font >= g_FileManager.UnicodeFontsCount)
		return 0;
	else if (m_UnicodeFontAddress[font] == NULL)
		return 0;

	return font;
}
//----------------------------------------------------------------------------------
/*!
������� �� ������ �� �������
@param [__in] link ������ ������
@return 
*/
void CFontsManager::GoToWebLink(ushort link)
{
	WEBLINK_MAP::iterator it = m_WebLink.find(link);

	if (it != m_WebLink.end())
	{
		(*it).second.Visited = true;
		g_Orion.GoToWebLink((*it).second.WebLink);
	}
}
//----------------------------------------------------------------------------------
//----------------------------------------ASCII-------------------------------------
//----------------------------------------------------------------------------------
/*!
�������� �������� ������� �������
@param [__in] font �����
@param [__in] index ������ �������
@return �������� � ��������
*/
int CFontsManager::GetFontOffsetY(uchar font, uchar index)
{
	int offsY = 0;

	if (index == 0xB8)
		offsY = 1;
	else if (!(index >= 0x41 && index <= 0x5A) && !(index >= 0xC0 && index <= 0xDF) && index != 0xA8)
	{
		const int offsetCharTable[] = {2, 0, 2, 2, 0, 0, 2, 2, 0, 0};
		const int offsetSymbolTable[] = {1, 0, 1, 1, -1, 0, 1, 1, 0, 0};

		if (font < 10)
		{
			if (index >= 0x61 && index <= 0x7A)
				offsY = offsetCharTable[font];
			else
				offsY = offsetSymbolTable[font];
		}
		else
			offsY = 2;
	}

	return offsY;
}
//----------------------------------------------------------------------------------
/*!
�������� ������� ������� � ������
@param [__in] font �����
@param [__in] str �����
@param [__in] pos ������� ������� � ������
@param [__in] width ������ ��������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@return ���������� �������
*/
POINT CFontsManager::GetCaretPosA(uchar font, const char *str, int pos, int width, TEXT_ALIGN_TYPE align, ushort flags)
{
	POINT p = {0, 0};

	if (font > m_FontCount || pos < 1)
		return p;

	int len = strlen(str);

	if (len < 1)
		return p;

	FONT_DATA &fd = m_Font[font];

	if (!width)
		width = GetWidthA(font, str, len);

	PMULTILINES_FONT_INFO info = GetInfoA(font, str, len, align, flags, width);
	if (info == NULL)
		return p;

	int height = 0;
	PMULTILINES_FONT_INFO ptr = info;

	while (ptr != NULL)
	{
		info = ptr;

		if (pos > 0)
		{
			len = ptr->CharCount;

			if (len && pos - len < 1)
			{
				IFOR(i, 0, len)
				{
					uchar index = m_FontIndex[(uchar)ptr->Data[i].item];

					if (index >= 0xE0)
						continue;

					p.x += fd.Chars[index].Width;
					pos--;

					if (pos < 1)
						break;
				}
			}

			pos -= len;

			if (pos <= 1)
			{
				p.y = height;

				if (pos == 1)
					p.y += ptr->MaxHeight;

				break;
			}
			else
				height += ptr->MaxHeight;

			pos--;
		}

		ptr = ptr->m_Next;

		info->Data.clear();
		delete info;
	}

	return p;
}
//----------------------------------------------------------------------------------
/*!
��������� ��������� �������
@param [__in] font �����
@param [__in] str �����
@param [__in] x ���������� ������� �� ��� X
@param [__in] y ���������� ������� �� ��� Y
@param [__in] width ������ ��������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@return ������� ������� � ������
*/
int CFontsManager::CalculateCaretPosA(uchar font, const char *str, int x, int y, int width, TEXT_ALIGN_TYPE align, ushort flags)
{
	if (font > m_FontCount || x < 0 || y < 0)
		return 0;

	int len = strlen(str);

	if (len < 1)
		return 0;

	FONT_DATA &fd = m_Font[font];

	if (!width)
		width = GetWidthA(font, str, len);

	if (x >= width)
		return len;

	PMULTILINES_FONT_INFO info = GetInfoA(font, str, len, align, flags, width);
	if (info == NULL)
		return 0;

	int height = GetHeightA(info);

	PMULTILINES_FONT_INFO ptr = info;

	if (y >= height)
	{
		while (ptr != NULL)
		{
			info = ptr;
			ptr = ptr->m_Next;

			info->Data.clear();
			delete info;
		}

		return 0;
	}

	height = 0;

	int pos = 0;
	bool found = false;

	while (ptr != NULL)
	{
		info = ptr;

		height += ptr->MaxHeight;
		width = 0;

		if (!found)
		{
			if (y < height)
			{
				len = ptr->CharCount;

				IFOR(i, 0, len)
				{
					uchar index = m_FontIndex[(uchar)ptr->Data[i].item];

					if (index >= 0xE0)
						continue;

					width += fd.Chars[index].Width;

					if (width > x)
						break;

					pos++;
				}

				found = true;
			}
			else
			{
				pos += ptr->CharCount;
				pos++;
			}
		}

		ptr = ptr->m_Next;

		info->Data.clear();
		delete info;
	}

	return pos;
}
//----------------------------------------------------------------------------------
/*!
�������� ������ ������
@param [__in] font �����
@param [__in] str �����
@param [__in_opt] len ����� ������
@return ������ ������ � ��������
*/
int CFontsManager::GetWidthA(uchar font, const char *str, int len)
{
	if (font > m_FontCount)
		return 0;

	if (!len)
	{
		len = strlen(str);

		if (!len)
			return 0;
	}

	FONT_DATA &fd = m_Font[font];
	int textLength = 0;

	IFOR(i, 0, len)
	{
		uchar index = m_FontIndex[(uchar)str[i]];

		if (index >= 0xE0)
			continue;

		textLength += fd.Chars[index].Width;
	}

	return textLength;
}
//----------------------------------------------------------------------------------
/*!
�������� ������ ������ (� ������ ���������� ���������)
@param [__in] font �����
@param [__in] str �����
@param [__in] len ����� ������
@param [__in] maxWidth ������������ ������ ��������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@return ������ ������ � ��������
*/
int CFontsManager::GetWidthExA(uchar font, const char *str, int len, int maxWidth, TEXT_ALIGN_TYPE align, ushort flags)
{
	if (font > m_FontCount)
		return 0;

	if (!len)
	{
		len = strlen(str);

		if (!len)
			return 0;
	}

	PMULTILINES_FONT_INFO info = GetInfoA(font, str, len, align, flags, maxWidth);

	int textWidth = 0;
	PMULTILINES_FONT_INFO ptr = info;

	while (ptr != NULL)
	{
		info = ptr;

		if (ptr->Width > textWidth)
			textWidth = ptr->Width;

		ptr = ptr->m_Next;

		info->Data.clear();
		delete info;
	}

	return textWidth;
}
//----------------------------------------------------------------------------------
/*!
�������� ������ ������
@param [__in] font �����
@param [__in] str �����
@param [__in_opt] width ������ ��������
@param [__in_opt] align ������������ ������
@param [__in_opt] flags ������� ������
@return ������ ������ � ��������
*/
int CFontsManager::GetHeightA(uchar font, const char *str, int width, TEXT_ALIGN_TYPE align, ushort flags)
{
	if (font > m_FontCount)
		return 0;

	int len = strlen(str);

	if (len < 1)
		return 0;

	if (!width)
		width = GetWidthA(font, str, len);

	PMULTILINES_FONT_INFO info = GetInfoA(font, str, len, align, flags, width);

	int textHeight = 0;
	PMULTILINES_FONT_INFO ptr = info;

	while (ptr != NULL)
	{
		info = ptr;

		textHeight += ptr->MaxHeight;

		ptr = ptr->m_Next;

		info->Data.clear();
		delete info;
	}

	return textHeight;
}
//----------------------------------------------------------------------------------
/*!
�������� ������ ������ �� ������ �����
@param [__in] info ������ �� �������������� �����
@return ������ ������ � ��������
*/
int CFontsManager::GetHeightA(PMULTILINES_FONT_INFO info)
{
	int textHeight = 0;

	while (info != NULL)
	{
		textHeight += info->MaxHeight;

		info = info->m_Next;
	}

	return textHeight;
}
//----------------------------------------------------------------------------------
/*!
�������� ����� ��������� ������
@param [__in] font �����
@param [__in] str �����
@param [__in] len ����� ������
@param [__in] width ������ ��������
@param [__in] IsCropped ������������ �����, �������� �� ��������� ������� ����� ���������� � ���������� �� ����������
@return �������������� �����
*/
string CFontsManager::GetTextByWidthA(uchar font, const char *str, int len, int width, bool IsCropped)
{
	if (!len || font > m_FontCount)
		return string("");

	FONT_DATA &fd = m_Font[font];

	if (IsCropped)
	{
		uchar idx = m_FontIndex[(uchar)'.'];

		if (idx < 0xE0)
			width -= fd.Chars[idx].Width * 3;
	}

	int textLength = 0;
	string result = "";

	IFOR(i, 0, len)
	{
		uchar index = m_FontIndex[(uchar)str[i]];

		if (index >= 0xE0)
			continue;

		textLength += fd.Chars[index].Width;

		if (textLength > width)
			break;

		result += str[i];

	}

	if (IsCropped)
		result += "...";

	return result;
}
//----------------------------------------------------------------------------------
/*!
�������� ���������� � ������ (� �������������� ����)
@param [__in] font �����
@param [__in] str �����
@param [__in] len ����� ������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@param [__in] width ������ ��������
@return ������ �� �������������� ����� ��� NULL
*/
PMULTILINES_FONT_INFO CFontsManager::GetInfoA(uchar font, const char *str, int len, TEXT_ALIGN_TYPE align, ushort flags, int width)
{
	if (font > m_FontCount)
		return NULL;

	FONT_DATA &fd = m_Font[font];

	PMULTILINES_FONT_INFO info = new MULTILINES_FONT_INFO();
	info->Reset();
	info->Align = align;

	PMULTILINES_FONT_INFO ptr = info;

	int indentionOffset = 0;
	ptr->IndentionOffset = 0;

	bool isFixed = (flags & UOFONT_FIXED);
	bool isCropped = (flags & UOFONT_CROPPED);

	int charCount = 0;
	int lastSpace = 0;
	int readWidth = 0;

	IFOR(i, 0, len)
	{
		char si = str[i];

		if (si == '\r' || si == '\n')
		{
			if (si == '\r' || isFixed || isCropped)
				continue;
			else
				si = '\n';
		}

		uchar index = m_FontIndex[(uchar)si];
		if (index >= 0xE0 && si != '\n')
			continue;

		if (si == ' ')
		{
			lastSpace = i;
			ptr->Width += readWidth;
			readWidth = 0;
			ptr->CharCount += charCount;
			charCount = 0;
		}

		FONT_CHARACTER_DATA &fcd = fd.Chars[index];

		if (si == '\n' || ptr->Width + readWidth + fcd.Width > width)
		{
			if (lastSpace == ptr->CharStart && !lastSpace && si != '\n')
				ptr->CharStart = 1;

			if (si == '\n')
			{
				ptr->Width += readWidth;
				ptr->CharCount += charCount;

				lastSpace = i;

				if (!ptr->Width)
					ptr->Width = 1;

				if (!ptr->MaxHeight)
					ptr->MaxHeight = 14;

				ptr->Data.resize(ptr->CharCount);

				PMULTILINES_FONT_INFO newptr = new MULTILINES_FONT_INFO();
				newptr->Reset();

				ptr->m_Next = newptr;

				ptr = newptr;

				ptr->Align = align;
				ptr->CharStart = i + 1;

				readWidth = 0;
				charCount = 0;
				indentionOffset = 0;

				ptr->IndentionOffset = 0;

				continue;
			}
			else if (lastSpace + 1 == ptr->CharStart && !isFixed && !isCropped)
			{
				ptr->Width += readWidth;
				ptr->CharCount += charCount;

				if (!ptr->Width)
					ptr->Width = 1;

				if (!ptr->MaxHeight)
					ptr->MaxHeight = 14;

				PMULTILINES_FONT_INFO newptr = new MULTILINES_FONT_INFO();
				newptr->Reset();

				ptr->m_Next = newptr;

				ptr = newptr;

				ptr->Align = align;
				ptr->CharStart = i;
				lastSpace = i - 1;
				charCount = 0;

				if (ptr->Align == TS_LEFT && (flags & UOFONT_INDENTION))
					indentionOffset = 14;

				ptr->IndentionOffset = indentionOffset;

				readWidth = indentionOffset;
			}
			else
			{
				if (isFixed)
				{
					MULTILINES_FONT_DATA mfd = {si, flags, font, 0, 0xFFFFFFFF};
					ptr->Data.push_back(mfd);

					readWidth += fcd.Width;

					if (fcd.Height > ptr->MaxHeight)
						ptr->MaxHeight = fcd.Height;

					charCount++;

					ptr->Width += readWidth;
					ptr->CharCount += charCount;
				}

				i = lastSpace + 1;
				si = str[i];

				if (!ptr->Width)
					ptr->Width = 1;

				if (!ptr->MaxHeight)
					ptr->MaxHeight = 14;

				ptr->Data.resize(ptr->CharCount);
				charCount = 0;

				if (isFixed || isCropped)
					break;

				PMULTILINES_FONT_INFO newptr = new MULTILINES_FONT_INFO();
				newptr->Reset();

				ptr->m_Next = newptr;

				ptr = newptr;

				ptr->Align = align;
				ptr->CharStart = i;

				if (ptr->Align == TS_LEFT && (flags & UOFONT_INDENTION))
					indentionOffset = 14;

				ptr->IndentionOffset = indentionOffset;

				readWidth = indentionOffset;
			}
		}

		MULTILINES_FONT_DATA mfd = {si, flags, font, 0, 0xFFFFFFFF};
		ptr->Data.push_back(mfd);

		readWidth += fcd.Width;

		if (fcd.Height > ptr->MaxHeight)
			ptr->MaxHeight = fcd.Height;

		charCount++;
	}

	ptr->Width += readWidth;
	ptr->CharCount += charCount;

	if (!readWidth && len && (str[len - 1] == '\n' || str[len - 1] == '\r'))
	{
		ptr->Width = 1;
		ptr->MaxHeight = 14;
	}

	if (font == 4)
	{
		ptr = info;

		while (ptr != NULL)
		{
			if (ptr->Width > 1)
				ptr->MaxHeight = ptr->MaxHeight + 2;
			else
				ptr->MaxHeight = ptr->MaxHeight + 6;

			ptr = ptr->m_Next;
		}
	}

	return info;
}
//----------------------------------------------------------------------------------
/*!
������������� �������� ������
@param [__in] font �����
@param [__inout] th ������ � �������� ������
@param [__in] str �����
@param [__in_opt] color ����
@param [__in_opt] width ������ ��������
@param [__in_opt] align ������������ ������
@param [__in_opt] flags ������� ������
@return true ��� �������� ���������
*/
bool CFontsManager::GenerateA(uchar font, CGLTextTexture &th, const char *str, ushort color, int width, TEXT_ALIGN_TYPE align, ushort flags)
{
	if ((flags & UOFONT_FIXED) || (flags & UOFONT_CROPPED))
	{
		th.Clear();

		if (!width)
			return false;

		int len = strlen(str);
		if (!len)
			return false;

		int realWidth = GetWidthA(font, str, len);

		if (realWidth > width)
		{
			string newstr = GetTextByWidthA(font, str, len, width, (flags & UOFONT_CROPPED));
			return GenerateABase(font, th, newstr.c_str(), color, width, align, flags);
		}
	}

	return GenerateABase(font, th, str, color, width, align, flags);
}
//----------------------------------------------------------------------------------
/*!
������������� ������� �������� ������
@param [__in] font �����
@param [__inout] th ������ � �������� ������
@param [__in] str �����
@param [__in] color ���� ������
@param [__in] width ������ ��������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@return ������ �� ������ ��������
*/
UINT_LIST CFontsManager::GeneratePixelsA(uchar &font, CGLTextTexture &th, const char *str, ushort &color, int &width, TEXT_ALIGN_TYPE &align, ushort &flags)
{
	UINT_LIST pData;

	th.Clear();

	if (font > m_FontCount)
		return pData;

	int len = strlen(str);
	if (!len)
		return pData;

	FONT_DATA &fd = m_Font[font];

	if (!width)
		width = GetWidthA(font, str, len);

	if (!width)
		return pData;

	PMULTILINES_FONT_INFO info = GetInfoA(font, str, len, align, flags, width);
	if (info == NULL)
		return pData;

	width += 4;

	int height = GetHeightA(info);

	if (!height)
	{
		PMULTILINES_FONT_INFO ptr = info;

		while (ptr != NULL)
		{
			info = ptr;

			ptr = ptr->m_Next;

			info->Data.clear();
			delete info;
		}

		return pData;
	}
	
	int blocksize = (height + 1) * (width + 1);

	if (m_SavePixels)
		th.PixelsData.resize(blocksize, 0);

	pData.resize(blocksize, 0);

	int lineOffsY = 0;
	PMULTILINES_FONT_INFO ptr = info;

	bool partialHue = (font != 5 && font != 8) && !m_UnusePartialHue;
	int font6OffsetY = (int)(font == 6) * 7;

	while (ptr != NULL)
	{
		info = ptr;

		th.IncLinesCount();
		
		int w = 0;
		if (ptr->Align == TS_CENTER)
		{
			w = (((width - 10) - ptr->Width) / 2);
			if (w < 0)
				w = 0;
		}
		else if (ptr->Align == TS_RIGHT)
		{
			w = ((width - 10) - ptr->Width);
			if (w < 0)
				w = 0;
		}
		else if (ptr->Align == TS_LEFT && (flags & UOFONT_INDENTION))
			w = ptr->IndentionOffset;

		int count = (int)ptr->Data.size();

		IFOR(i, 0, count)
		{
			BYTE index = (BYTE)ptr->Data[i].item;
			int offsY = GetFontOffsetY(font, index);

			index = m_FontIndex[index];
			if (index >= 0xE0)
				continue;

			FONT_CHARACTER_DATA &fcd = fd.Chars[index];

			int dw = fcd.Width;
			int dh = fcd.Height;

			WORD charColor = color;

			IFOR(y, 0, dh)
			{
				int testY = y + lineOffsY + offsY;

				if (testY >= height)
					break;

				IFOR(x, 0, dw)
				{
					if ((x + w) >= width)
						break;

					WORD pic = fcd.Data[(y * dw) + x];

					if (pic)
					{
						DWORD pcl = 0;

						if (partialHue)
							pcl = g_ColorManager.GetPartialHueColor(pic, charColor);
						else
							pcl = g_ColorManager.GetColor(pic, charColor);

						int block = (testY * width) + (x + w);

						pData[block] = pcl << 8 | 0xFF; // (0xFF << 24) | (GetBValue(pcl) << 16) | (GetGValue(pcl) << 8) | GetRValue(pcl);

						if (m_SavePixels)
							th.PixelsData[block] = 1; //((pic >> 10) & 0x1F);
					}
				}
			}

			w += dw;
		}

		lineOffsY += (ptr->MaxHeight - font6OffsetY);

		ptr = ptr->m_Next;

		info->Data.clear();
		delete info;
	}

	th.Width = width;
	th.Height = height;

	return pData;
}
//----------------------------------------------------------------------------------
/*!
�������� ASCII ��������
@param [__in] font �����
@param [__out] th ������ ��������
@param [__in] str �����
@param [__in] color ����
@param [__in] width ������ ��������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@return true ��� �������� ���������
*/
bool CFontsManager::GenerateABase(uchar &font, CGLTextTexture &th, const char *str, ushort &color, int &width, TEXT_ALIGN_TYPE &align, ushort &flags)
{
	UINT_LIST pixels = GeneratePixelsA(font, th, str, color, width, align, flags);
	bool result = false;

	if (pixels.size())
	{
		g_GL.BindTexture32(th.Texture, th.Width, th.Height, &pixels[0]);

		result = true;
	}

	return true;
}
//----------------------------------------------------------------------------------
/*!
���������� �����
@param [__in] font �����
@param [__in] str �����
@param [__in] color ����
@param [__in] x �������� ���������� X
@param [__in] y �������� ���������� Y
@param [__in_opt] width ������ ��������
@param [__in_opt] align ������������ ������
@param [__in_opt] flags ������� ������
@return 
*/
void CFontsManager::DrawA(uchar font, const char *str, ushort color, int x, int y, int width, TEXT_ALIGN_TYPE align, ushort flags)
{
	CGLTextTexture th;

	if (GenerateA(font, th, str, color, width, align, flags))
		th.Draw(x, y);
}









//----------------------------------------------------------------------------------
//--------------------------------------Unicode-------------------------------------
//----------------------------------------------------------------------------------
/*!
�������� ������� ������� � ������
@param [__in] font �����
@param [__in] str �����
@param [__in] pos ������� ������� � ������
@param [__in] width ������ ��������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@return ���������� �������
*/
POINT CFontsManager::GetCaretPosW(uchar font, const wchar_t *str, int pos, int width, TEXT_ALIGN_TYPE align, ushort flags)
{
	POINT p = {0, 0};

	if (pos < 1 || font >= 20 || !m_UnicodeFontAddress[font])
		return p;

	int len = lstrlenW(str);

	if (len < 1)
		return p;

	if (!width)
		width = GetWidthW(font, str, len);

	PMULTILINES_FONT_INFO info = GetInfoW(font, str, len, align, flags, width);
	if (info == NULL)
		return p;
	
	puint table = (puint)m_UnicodeFontAddress[font];
	int height = 0;
	PMULTILINES_FONT_INFO ptr = info;

	while (ptr != NULL)
	{
		info = ptr;

		if (pos > 0)
		{
			len = ptr->CharCount;

			if (len && pos - len < 1)
			{
				IFOR(i, 0, len)
				{
					wchar_t ch = ptr->Data[i].item;
					int cwidth = 0;

					if (table[ch] && table[ch] != 0xFFFFFFFF)
					{
						puchar cptr = (puchar)((uint)table + table[ch]);
						p.x += ((char)cptr[0] + (char)cptr[2] + 1);
					}
					else if (ch == L' ')
						p.x += UNICODE_SPACE_WIDTH;
					else
						pos++;

					pos--;

					if (pos < 1)
						break;
				}
			}

			pos -= len;

			if (pos <= 1)
			{
				p.y = height;

				if (pos == 1)
					p.y += ptr->MaxHeight;

				break;
			}
			else
				height += ptr->MaxHeight;

			pos--;
		}

		ptr = ptr->m_Next;

		info->Data.clear();
		delete info;
	}

	return p;
}
//----------------------------------------------------------------------------------
/*!
��������� ��������� �������
@param [__in] font �����
@param [__in] str �����
@param [__in] x ���������� ������� �� ��� X
@param [__in] y ���������� ������� �� ��� Y
@param [__in] width ������ ��������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@return ������� ������� � ������
*/
int CFontsManager::CalculateCaretPosW(uchar font, const wchar_t *str, int x, int y, int width, TEXT_ALIGN_TYPE align, ushort flags)
{
	if (x < 0 || y < 0 || font >= 20 || !m_UnicodeFontAddress[font])
		return 0;

	int len = lstrlenW(str);

	if (len < 1)
		return 0;

	if (!width)
		width = GetWidthW(font, str, len);

	if (x >= width)
		return len;

	PMULTILINES_FONT_INFO info = GetInfoW(font, str, len, align, flags, width);
	if (info == NULL)
		return 0;

	int height = GetHeightW(info);

	PMULTILINES_FONT_INFO ptr = info;

	if (y >= height)
	{
		while (ptr != NULL)
		{
			info = ptr;
			ptr = ptr->m_Next;

			info->Data.clear();
			delete info;
		}

		return 0;
	}

	height = 0;

	puint table = (puint)m_UnicodeFontAddress[font];
	int pos = 0;
	bool found = false;

	while (ptr != NULL)
	{
		info = ptr;

		height += ptr->MaxHeight;
		width = 0;

		if (!found)
		{
			if (y < height)
			{
				len = ptr->CharCount;

				IFOR(i, 0, len)
				{
					wchar_t ch = ptr->Data[i].item;
					int cwidth = 0;

					if (table[ch] && table[ch] != 0xFFFFFFFF)
					{
						puchar cptr = (puchar)((uint)table + table[ch]);
						width += ((char)cptr[0] + (char)cptr[2] + 1);
					}
					else if (ch == L' ')
						width += UNICODE_SPACE_WIDTH;

					if (width > x)
						break;

					pos++;
				}

				found = true;
			}
			else
			{
				pos += ptr->CharCount;
				pos++;
			}
		}

		ptr = ptr->m_Next;

		info->Data.clear();
		delete info;
	}

	return pos;
}
//----------------------------------------------------------------------------------
/*!
�������� ������ ������
@param [__in] font �����
@param [__in] str �����
@param [__in_opt] len ����� ������
@return ������ ������ � ��������
*/
int CFontsManager::GetWidthW(uchar font, const wchar_t *str, int len)
{
	if (font >= 20 || !m_UnicodeFontAddress[font])
		return 0;

	if (!len)
	{
		len = lstrlenW(str);

		if (!len)
			return 0;
	}

	puint table = (puint)m_UnicodeFontAddress[font];
	int textLength = 0;

	IFOR(i, 0, len)
	{
		if (table[str[i]] && table[str[i]] != 0xFFFFFFFF)
		{
			puchar ptr = (puchar)((uint)table + table[str[i]]);
			textLength += ((char)ptr[0] + (char)ptr[2] + 1);
		}
		else if (str[i] == L' ')
			textLength += UNICODE_SPACE_WIDTH;
	}

	return textLength;
}
//----------------------------------------------------------------------------------
/*!
�������� ������ ������ (� ������ ���������� ���������)
@param [__in] font �����
@param [__in] str �����
@param [__in] len ����� ������
@param [__in] maxWidth ������������ ������ ��������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@return ������ ������ � ��������
*/
int CFontsManager::GetWidthExW(uchar font, const wchar_t *str, int len, int maxWidth, TEXT_ALIGN_TYPE align, ushort flags)
{
	if (font >= 20 || !m_UnicodeFontAddress[font])
		return 0;

	if (!len)
	{
		len = lstrlenW(str);

		if (!len)
			return 0;
	}

	PMULTILINES_FONT_INFO info = GetInfoW(font, str, len, align, flags, maxWidth);

	int textWidth = 0;
	PMULTILINES_FONT_INFO ptr = info;

	while (ptr != NULL)
	{
		info = ptr;

		if (ptr->Width > textWidth)
			textWidth = ptr->Width;

		ptr = ptr->m_Next;

		info->Data.clear();
		delete info;
	}

	return textWidth;
}
//----------------------------------------------------------------------------------
/*!
�������� ������ ������
@param [__in] font �����
@param [__in] str �����
@param [__in_opt] width ������ ��������
@param [__in_opt] align ������������ ������
@param [__in_opt] flags ������� ������
@return ������ ������ � ��������
*/
int CFontsManager::GetHeightW(uchar font, const wchar_t *str, int width, TEXT_ALIGN_TYPE align, ushort flags)
{
	if (font >= 20 || !m_UnicodeFontAddress[font])
		return 0;

	int len = lstrlenW(str);

	if (len < 1)
		return 0;

	if (!width)
		width = GetWidthW(font, str, len);

	PMULTILINES_FONT_INFO info = GetInfoW(font, str, len, align, flags, width);
	if (info == NULL)
		return 0;

	int textHeight = 0;
	PMULTILINES_FONT_INFO ptr = info;

	while (ptr != NULL)
	{
		info = ptr;

		if (m_UseHTML)
			textHeight += MAX_HTML_TEXT_HEIGHT;
		else
			textHeight += ptr->MaxHeight;

		ptr = ptr->m_Next;

		info->Data.clear();
		delete info;
	}

	return textHeight;
}
//----------------------------------------------------------------------------------
/*!
�������� ������ ������ �� ������ �����
@param [__in] info ������ �� �������������� �����
@return ������ ������ � ��������
*/
int CFontsManager::GetHeightW(PMULTILINES_FONT_INFO info)
{
	if (info == NULL)
		return 0;

	int textHeight = 0;

	while (info != NULL)
	{
		if (m_UseHTML)
			textHeight += MAX_HTML_TEXT_HEIGHT;
		else
			textHeight += info->MaxHeight;

		info = info->m_Next;
	}

	return textHeight;
}
//----------------------------------------------------------------------------------
/*!
�������� ����� ��������� ������
@param [__in] font �����
@param [__in] str �����
@param [__in] len ����� ������
@param [__in] width ������ ��������
@param [__in] IsCropped ������������ �����, �������� �� ��������� ������� ����� ���������� � ���������� �� ����������
@return �������������� �����
*/
wstring CFontsManager::GetTextByWidthW(uchar font, const wchar_t *str, int len, int width, bool IsCropped)
{
	if (font >= 20 || !m_UnicodeFontAddress[font] || !len)
		return wstring(L"");

	puint table = (puint)m_UnicodeFontAddress[font];

	if (IsCropped)
	{
		uint td = table[L'.'];

		if (td && td != 0xFFFFFFFF)
			width -= (*((puchar)((uint)table + td + 2)) * 3);
	}

	int textLength = 0;
	wstring result = L"";

	IFOR(i, 0, len)
	{
		uint ts = table[str[i]];
		char cw = 0;

		if (ts && ts != 0xFFFFFFFF)
		{
			puchar ptr = (puchar)((uint)table + ts);
			cw = ((char)ptr[0] + (char)ptr[2] + 1);
		}
		else if (str[i] == L' ')
			cw = UNICODE_SPACE_WIDTH;

		if (cw)
		{
			textLength += cw;

			if (textLength > width)
				break;

			result += str[i];
		}
	}

	if (IsCropped)
		result += L"...";

	return result;
}
//----------------------------------------------------------------------------------
/*!
�������� ������ ������
@param [__in] link ������
@param [__out] color ���� ������
@return ������ ������
*/
ushort CFontsManager::GetWebLinkID(wchar_t *link, uint &color)
{
	string webLink(ToString(link));
	ushort linkID = 0;

	WEBLINK_MAP::iterator it = m_WebLink.begin();

	for (; it != m_WebLink.end(); it++)
	{
		if ((*it).second.WebLink == webLink)
			break;
	}

	if (it == m_WebLink.end())
	{
		linkID = (ushort)m_WebLink.size() + 1;
		WEB_LINK wl = {false, webLink};
		m_WebLink[linkID] = wl;
	}
	else
	{
		if ((*it).second.Visited)
			color = 0xFF0000FF;

		linkID = (*it).first;
	}

	return linkID;
}
//----------------------------------------------------------------------------------
/*!
��������� HTML ������
@param [__in] font �����
@param [__in] str �����
@param [__in] len ����� ������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@return ������ HTML ��������
*/
HTML_char *CFontsManager::GetHTMLData(uchar font, const wchar_t *str, int &len, TEXT_ALIGN_TYPE align, ushort flags)
{
	if (len < 1)
		return NULL;

	HTML_char *data = new HTML_char[len];
	int newlen = 0;

	TEXT_ALIGN_TYPE current_align = align;
	ushort current_flags = flags;
	uchar current_font = font;
	uint charcolor = m_HTMLColor;
	uint current_charcolor = charcolor;
	ushort currentLink = 0;

	IFOR(i, 0, len)
	{
		wchar_t si = str[i];

		int copylen = len - i;

		if (si == L'<' && copylen > 2)
		{
			wchar_t lstr[12] = {0};

			if (copylen > 11)
				copylen = 11;

			memcpy(&lstr[0], &str[i], copylen * 2);

			_wcslwr(lstr);

			if (!memcmp(lstr, L"<center>", 16))
			{
				current_align = TS_CENTER;

				if (newlen)
					si = L'\n';
				else
					si = 0;

				i += 7;
			}
			else if (!memcmp(lstr, L"</center>", 18))
			{
				current_align = align;
				i += 8;
				si = L'\n';
			}
			else if (!memcmp(lstr, L"<left>", 12))
			{
				current_align = TS_LEFT;

				if (newlen)
					si = L'\n';
				else
					si = 0;

				i += 5;
			}
			else if (!memcmp(lstr, L"</left>", 14))
			{
				current_align = align;
				i += 6;
				si = L'\n';
			}
			else if (!memcmp(lstr, L"<right>", 14))
			{
				current_align = TS_RIGHT;

				if (newlen)
					si = L'\n';
				else
					si = 0;

				i += 6;
			}
			else if (!memcmp(lstr, L"</right>", 16))
			{
				current_align = align;
				i += 7;
				si = L'\n';
			}
			else if (!memcmp(lstr, L"<p>", 6))
			{
				if (!(current_flags & UOFONT_INDENTION))
					current_flags |= UOFONT_INDENTION;

				i += 2;
				continue;
			}
			else if (!memcmp(lstr, L"</p>", 8))
			{
				if (current_flags & UOFONT_INDENTION)
					current_flags ^= UOFONT_INDENTION;

				i += 3;
				si = L'\n';
			}
			else if (!memcmp(lstr, L"<u>", 6))
			{
				if (!(current_flags & UOFONT_UNDERLINE))
					current_flags |= UOFONT_UNDERLINE;

				i += 2;
				continue;
			}
			else if (!memcmp(lstr, L"</u>", 8))
			{
				if (current_flags & UOFONT_UNDERLINE)
					current_flags ^= UOFONT_UNDERLINE;

				i += 3;
				continue;
			}
			else if (!memcmp(lstr, L"<body>", 12))
			{
				i += 5;
				continue;
			}
			else if (!memcmp(lstr, L"</body>", 14))
			{
				i += 6;
				continue;
			}
			else if (!memcmp(lstr, L"<a", 4))
			{
				i += 2;

				for (; i < len; i++)
				{
					if (str[i] != L' ')
					{
						if (str[i] == L'>')
							i++;

						break;
					}
				}

				ushort oldFlags = current_flags;
				uint oldColor = current_charcolor;

				if (!(current_flags & UOFONT_UNDERLINE))
					current_flags |= UOFONT_UNDERLINE;

				current_charcolor = 0xFFFFCECE;
				currentLink = 0xFFFF;

				for (; i < len; i++)
				{
					si = str[i];

					copylen = len - i;

					if (copylen > 2)
					{
						if (copylen > 10)
							copylen = 10;

						wchar_t astr[11] = {0};
						memcpy(&astr[0], &str[i], copylen * 2);

						_wcslwr(astr);
			
						if (!memcmp(astr, L"</a>", 8))
						{
							currentLink = 0;
							current_flags = oldFlags;
							current_charcolor = oldColor;

							i += 3;

							break;
						}
						else if (!memcmp(astr, L"href=\"", 12))
						{
							i += 6;

							IFOR(j, i, len)
							{
								if (str[j] == L'"')
								{
									int linkSize = j - i;
									wchar_t *link = new wchar_t[linkSize + 1];
									memcpy(&link[0], &str[i], linkSize * 2);
									link[linkSize] = 0;

									currentLink = GetWebLinkID(link, current_charcolor);

									delete link;

									for (; j < len; j++)
									{
										if (str[j] == L'>')
											break;
									}

									i = j + 1;
									si = str[i];

									break;
								}
							}
						}
					}

					data[newlen].Char = si;
					data[newlen].Font = current_font;
					data[newlen].Align = current_align;
					data[newlen].Flags = current_flags;
					data[newlen].Color = current_charcolor;
					data[newlen].LinkID = currentLink;

					newlen++;
				}

				continue;
			}
			else if (!memcmp(lstr, L"<basefont", 18))
			{
				i += 9;

				while (i < len && str[i] != L'>')
				{
					memcpy(&lstr[0], &str[i], copylen * 2);
					_wcslwr(lstr);

					if (!memcmp(lstr, L"color=", 12))
					{
						i += 6;

						if (str[i] == L'"')
							i++;

						wchar_t colorStr[20] = {0};

						IFOR(j, i, len)
						{
							if (str[j] == L'>')
							{
								int colorLen = j - i;
								memcpy(&colorStr[0], &str[i], colorLen * 2);

								i = j;

								if (colorLen > 10)
									colorLen = 10;

								if (colorLen && colorStr[colorLen - 1] == L'"')
									colorStr[colorLen - 1] = 0;

								colorStr[colorLen] = 0;

								break;
							}
						}

						if (colorStr[0] == L'#')
						{
							char *end;
							charcolor = strtoul(ToString(colorStr + 1).c_str(), &end, 16);

							//if (!(charcolor & 0xFF))
							//	charcolor |= 0xFF;

							puchar clrBuf = (puchar)&charcolor;
							charcolor = (clrBuf[0] << 24) | (clrBuf[1] << 16) | (clrBuf[2] << 8) | 0xFF;

							current_charcolor = charcolor;
						}
						else
						{
							_wcslwr(colorStr);

							if (!memcmp(colorStr, L"red", 6))
								charcolor = 0xFF0000FF;
							else if (!memcmp(colorStr, L"cyan", 8))
								charcolor = 0x00FFFFFF;
							else if (!memcmp(colorStr, L"blue", 8))
								charcolor = 0x0000FFFF;
							else if (!memcmp(colorStr, L"darkblue", 16))
								charcolor = 0x0000A0FF;
							else if (!memcmp(colorStr, L"lightblue", 18))
								charcolor = 0xADD8E6FF;
							else if (!memcmp(colorStr, L"purple", 12))
								charcolor = 0x800080FF;
							else if (!memcmp(colorStr, L"yellow", 12))
								charcolor = 0x00FFFFFF;
							else if (!memcmp(colorStr, L"lime", 8))
								charcolor = 0x00FF00FF;
							else if (!memcmp(colorStr, L"magenta", 14))
								charcolor = 0xFF00FFFF;
							else if (!memcmp(colorStr, L"white", 10))
								charcolor = 0xFFFEFEFF;
							else if (!memcmp(colorStr, L"silver", 12))
								charcolor = 0xC0C0C0FF;
							else if (!memcmp(colorStr, L"gray", 8) || !memcmp(colorStr, L"grey", 8))
								charcolor = 0x808080FF;
							else if (!memcmp(colorStr, L"black", 10))
								charcolor = 0x010101FF;
							else if (!memcmp(colorStr, L"orange", 12))
								charcolor = 0xFFA500FF;
							else if (!memcmp(colorStr, L"brown", 10))
								charcolor = 0xA52A2AFF;
							else if (!memcmp(colorStr, L"maroon", 12))
								charcolor = 0x800000FF;
							else if (!memcmp(colorStr, L"green", 10))
								charcolor = 0x008000FF;
							else if (!memcmp(colorStr, L"olive", 10))
								charcolor = 0x808000FF;

							current_charcolor = charcolor;
						}
					}
					else
						i++;
				}

				continue;
			}
			else if (!memcmp(lstr, L"</basefont>", 22))
			{
				i += 10;

				continue;
			}
			else if (!memcmp(lstr, L"<br>", 8))
			{
				i += 3;
				si = L'\n';
			}
			else if (!memcmp(lstr, L"<br />", 12))
			{
				i += 5;
				si = L'\n';
			}
			else if (!memcmp(lstr, L"<i>", 6))
			{
				if (!(current_flags & UOFONT_ITALIC))
					current_flags |= UOFONT_ITALIC;

				i += 2;
				continue;
			}
			else if (!memcmp(lstr, L"</i>", 8))
			{
				if (current_flags & UOFONT_ITALIC)
					current_flags ^= UOFONT_ITALIC;

				i += 3;
				continue;
			}
			else if (!memcmp(lstr, L"<b>", 6))
			{
				if (!(current_flags & UOFONT_SOLID))
					current_flags |= UOFONT_SOLID;

				i += 2;
				continue;
			}
			else if (!memcmp(lstr, L"</b>", 8))
			{
				if (current_flags & UOFONT_SOLID)
					current_flags ^= UOFONT_SOLID;

				i += 3;
				continue;
			}
			else if (!memcmp(lstr, L"<big>", 10))
			{
				if (m_UnicodeFontAddress[0])
					current_font = 0;

				i += 4;
				continue;
			}
			else if (!memcmp(lstr, L"</big>", 12))
			{
				current_font = font;

				i += 5;
				continue;
			}
			else if (!memcmp(lstr, L"<h1>", 8))
			{
				if (!(current_flags & UOFONT_SOLID))
					current_flags |= UOFONT_SOLID;

				if (!(current_flags & UOFONT_UNDERLINE))
					current_flags |= UOFONT_UNDERLINE;

				if (m_UnicodeFontAddress[0])
					current_font = 0;

				i += 3;
				continue;
			}
			else if (!memcmp(lstr, L"<h2>", 8))
			{
				if (!(current_flags & UOFONT_SOLID))
					current_flags |= UOFONT_SOLID;

				if (m_UnicodeFontAddress[0])
					current_font = 0;

				i += 3;
				continue;
			}
			else if (!memcmp(lstr, L"<h3>", 8))
			{
				if (m_UnicodeFontAddress[0])
					current_font = 0;

				i += 3;
				continue;
			}
			else if (!memcmp(lstr, L"<h4>", 8))
			{
				if (!(current_flags & UOFONT_SOLID))
					current_flags |= UOFONT_SOLID;

				if (m_UnicodeFontAddress[1])
					current_font = 1;

				i += 3;
				continue;
			}
			else if (!memcmp(lstr, L"<h5>", 8))
			{
				if (!(current_flags & UOFONT_ITALIC))
					current_flags |= UOFONT_ITALIC;

				if (m_UnicodeFontAddress[1])
					current_font = 1;

				i += 3;
				continue;
			}
			else if (!memcmp(lstr, L"<h6>", 8))
			{
				if (m_UnicodeFontAddress[1])
					current_font = 1;

				i += 3;
				continue;
			}
			else if (!memcmp(lstr, L"</h1>", 10))
			{
				if (current_flags & UOFONT_SOLID)
					current_flags ^= UOFONT_SOLID;

				if (current_flags & UOFONT_UNDERLINE)
					current_flags ^= UOFONT_UNDERLINE;

				current_font = font;

				i += 4;
				continue;
			}
			else if (!memcmp(lstr, L"</h2>", 10))
			{
				if (current_flags & UOFONT_SOLID)
					current_flags ^= UOFONT_SOLID;

				current_font = font;

				i += 4;
				continue;
			}
			else if (!memcmp(lstr, L"</h3>", 10))
			{
				current_font = font;
				
				i += 4;
				continue;
			}
			else if (!memcmp(lstr, L"</h4>", 10))
			{
				if (current_flags & UOFONT_SOLID)
					current_flags ^= UOFONT_SOLID;

				current_font = font;
				
				i += 4;
				continue;
			}
			else if (!memcmp(lstr, L"</h5>", 10))
			{
				if (current_flags & UOFONT_ITALIC)
					current_flags ^= UOFONT_ITALIC;

				current_font = font;
				
				i += 4;
				continue;
			}
			else if (!memcmp(lstr, L"</h6>", 10))
			{
				current_font = font;

				i += 4;
				continue;
			}
			else if (!memcmp(lstr, L"<div ", 10))
			{
			}
			else if (!memcmp(lstr, L"</div ", 12))
			{
			}
		}

		if (si)
		{
			data[newlen].Char = si;
			data[newlen].Font = current_font;
			data[newlen].Align = current_align;
			data[newlen].Flags = current_flags;
			data[newlen].Color = current_charcolor;
			data[newlen].LinkID = currentLink;

			newlen++;
		}
	}

	len = newlen;

	return data;
}
//----------------------------------------------------------------------------------
/*!
��������� ������ �������������� ������ HTML
@param [__in] font �����
@param [__in] str �����
@param [__in] len ����� ������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@param [__in] width ������ ������
@return ������ �� ������
*/
PMULTILINES_FONT_INFO CFontsManager::GetInfoHTML(uchar font, const wchar_t *str, int len, TEXT_ALIGN_TYPE align, ushort flags, int width)
{
	HTML_char *htmlData = GetHTMLData(font, str, len, align, flags);

	if (htmlData == NULL)
		return NULL;

	PMULTILINES_FONT_INFO info = new MULTILINES_FONT_INFO();
	info->Reset();
	info->Align = align;

	PMULTILINES_FONT_INFO ptr = info;

	int indentionOffset = 0;

	ptr->IndentionOffset = indentionOffset;

	int charCount = 0;
	int lastSpace = 0;
	int readWidth = 0;

	bool isFixed = ((flags & UOFONT_FIXED) != 0);
	bool isCropped = ((flags & UOFONT_CROPPED) != 0);

	if (len)
		ptr->Align = htmlData[0].Align;

	IFOR(i, 0, len)
	{
		wchar_t si = htmlData[i].Char;
		puint table = (puint)m_UnicodeFontAddress[htmlData[i].Font];

		if (si == 0x000D || si == L'\n')
		{
			if (si == 0x000D || isFixed || isCropped)
				si = 0;
			else
				si = L'\n';
		}

		if ((!table[si] || table[si] == 0xFFFFFFFF) && si != L' ' && si != L'\n')
			continue;

		puchar data = (puchar)((uint)table + table[si]);

		if (si == L' ')
		{
			lastSpace = i;
			ptr->Width += readWidth;
			readWidth = 0;
			ptr->CharCount += charCount;
			charCount = 0;
		}

		int solidWidth = (int)(htmlData[i].Flags & UOFONT_SOLID);

		if (ptr->Width + readWidth + ((char)data[0] + (char)data[2]) + solidWidth > width || si == L'\n')
		{
			if (lastSpace == ptr->CharStart && !lastSpace && si != L'\n')
				ptr->CharStart = 1;

			if (si == L'\n')
			{
				ptr->Width += readWidth;
				ptr->CharCount += charCount;

				lastSpace = i;

				if (!ptr->Width)
					ptr->Width = 1;

				// (!ptr->MaxHeight)
					//ptr->MaxHeight = 14;
				ptr->MaxHeight = MAX_HTML_TEXT_HEIGHT;

				ptr->Data.resize(ptr->CharCount);

				PMULTILINES_FONT_INFO newptr = new MULTILINES_FONT_INFO();
				newptr->Reset();

				ptr->m_Next = newptr;

				ptr = newptr;

				ptr->Align = htmlData[i].Align;
				ptr->CharStart = i + 1;

				readWidth = 0;
				charCount = 0;
				indentionOffset = 0;

				ptr->IndentionOffset = indentionOffset;

				continue;
			}
			else if (lastSpace + 1 == ptr->CharStart && !isFixed && !isCropped)
			{
				ptr->Width += readWidth;
				ptr->CharCount += charCount;

				if (!ptr->Width)
					ptr->Width = 1;

				//if (!ptr->MaxHeight)
					//ptr->MaxHeight = 10;
				ptr->MaxHeight = MAX_HTML_TEXT_HEIGHT;

				PMULTILINES_FONT_INFO newptr = new MULTILINES_FONT_INFO();
				newptr->Reset();

				ptr->m_Next = newptr;

				ptr = newptr;

				ptr->Align = htmlData[i].Align;
				ptr->CharStart = i;
				lastSpace = i - 1;
				charCount = 0;

				if (ptr->Align == TS_LEFT && (htmlData[i].Flags & UOFONT_INDENTION))
					indentionOffset = 14;
				
				ptr->IndentionOffset = indentionOffset;

				readWidth = (int)indentionOffset;
			}
			else
			{
				if (isFixed)
				{
					MULTILINES_FONT_DATA mfd = {si, htmlData[i].Flags, htmlData[i].Font, htmlData[i].LinkID, htmlData[i].Color};
					ptr->Data.push_back(mfd);

					readWidth += ((char)data[0] + (char)data[2] + 1);

					//if (((char)data[1] + (char)data[3]) > ptr->MaxHeight)
						//ptr->MaxHeight = ((char)data[1] + (char)data[3]);
					ptr->MaxHeight = MAX_HTML_TEXT_HEIGHT;
				
					charCount++;

					ptr->Width += readWidth;
					ptr->CharCount += charCount;
				}

				i = lastSpace + 1;
				
				si = htmlData[i].Char;
				solidWidth = (int)(htmlData[i].Flags & UOFONT_SOLID);

				if (!ptr->Width)
					ptr->Width = 1;

				//if (!ptr->MaxHeight)
					//ptr->MaxHeight = 10;
				ptr->MaxHeight = MAX_HTML_TEXT_HEIGHT;

				ptr->Data.resize(ptr->CharCount);
				charCount = 0;

				if (isFixed || isCropped)
					break;

				PMULTILINES_FONT_INFO newptr = new MULTILINES_FONT_INFO();
				newptr->Reset();

				ptr->m_Next = newptr;

				ptr = newptr;

				ptr->Align = htmlData[i].Align;
				ptr->CharStart = i;

				if (ptr->Align == TS_LEFT && (htmlData[i].Flags & UOFONT_INDENTION))
					indentionOffset = 14;

				ptr->IndentionOffset = indentionOffset;

				readWidth = indentionOffset;
			}
		}
				
		MULTILINES_FONT_DATA mfd = {si, htmlData[i].Flags, htmlData[i].Font, htmlData[i].LinkID, htmlData[i].Color};
		ptr->Data.push_back(mfd);

		if (si == L' ')
		{
			readWidth += UNICODE_SPACE_WIDTH;

			//if (!ptr->MaxHeight)
				//ptr->MaxHeight = 5;
		}
		else
		{
			readWidth += ((char)data[0] + (char)data[2] + 1) + solidWidth;

			//if (((char)data[1] + (char)data[3]) > ptr->MaxHeight)
				//->MaxHeight = ((char)data[1] + (char)data[3]);
		}

		charCount++;
	}

	ptr->Width += readWidth;
	ptr->CharCount += charCount;
	ptr->MaxHeight = MAX_HTML_TEXT_HEIGHT;

	delete htmlData;

	return info;
}
//----------------------------------------------------------------------------------
/*!
�������� ���������� � ������ (� �������������� ����)
@param [__in] font �����
@param [__in] str �����
@param [__in] len ����� ������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@param [__in] width ������ ��������
@return ������ �� �������������� ����� ��� NULL
*/
PMULTILINES_FONT_INFO CFontsManager::GetInfoW(uchar font, const wchar_t *str, int len, TEXT_ALIGN_TYPE align, ushort flags, int width)
{
	if (font >= 20 || !m_UnicodeFontAddress[font])
		return NULL;

	if (m_UseHTML)
		return GetInfoHTML(font, str, len, align, flags, width);

	puint table = (puint)m_UnicodeFontAddress[font];

	PMULTILINES_FONT_INFO info = new MULTILINES_FONT_INFO();
	info->Reset();
	info->Align = align;

	PMULTILINES_FONT_INFO ptr = info;

	int indentionOffset = 0;

	ptr->IndentionOffset = 0;

	int charCount = 0;
	int lastSpace = 0;
	int readWidth = 0;

	bool isFixed = ((flags & UOFONT_FIXED) != 0);
	bool isCropped = ((flags & UOFONT_CROPPED) != 0);

	TEXT_ALIGN_TYPE current_align = align;
	ushort current_flags = flags;
	uchar current_font = font;
	uint charcolor = 0xFFFFFFFF;
	uint current_charcolor = 0xFFFFFFFF;
	uint lastspace_charcolor = 0xFFFFFFFF;
	uint lastspace_current_charcolor = 0xFFFFFFFF;

	IFOR(i, 0, len)
	{
		wchar_t si = str[i];

		if (si == L'\r' || si == L'\n')
		{
			if (isFixed || isCropped)
				si = 0;
			else
				si = L'\n';
		}

		if ((!table[si] || table[si] == 0xFFFFFFFF) && si != L' ' && si != L'\n')
			continue;

		puchar data = (puchar)((uint)table + table[si]);

		if (si == L' ')
		{
			lastSpace = i;
			ptr->Width += readWidth;
			readWidth = 0;
			ptr->CharCount += charCount;
			charCount = 0;
			lastspace_charcolor = charcolor;
			lastspace_current_charcolor = current_charcolor;
		}

		if (ptr->Width + readWidth + ((char)data[0] + (char)data[2]) > width || si == L'\n')
		{
			if (lastSpace == ptr->CharStart && !lastSpace && si != L'\n')
				ptr->CharStart = 1;

			if (si == L'\n')
			{
				ptr->Width += readWidth;
				ptr->CharCount += charCount;

				lastSpace = i;

				if (!ptr->Width)
					ptr->Width = 1;

				if (!ptr->MaxHeight)
					ptr->MaxHeight = 14;

				ptr->Data.resize(ptr->CharCount);

				PMULTILINES_FONT_INFO newptr = new MULTILINES_FONT_INFO();
				newptr->Reset();

				ptr->m_Next = newptr;

				ptr = newptr;

				ptr->Align = current_align;
				ptr->CharStart = i + 1;

				readWidth = 0;
				charCount = 0;
				indentionOffset = 0;

				ptr->IndentionOffset = 0;

				continue;
			}
			else if (lastSpace + 1 == ptr->CharStart && !isFixed && !isCropped)
			{
				ptr->Width += readWidth;
				ptr->CharCount += charCount;

				if (!ptr->Width)
					ptr->Width = 1;

				if (!ptr->MaxHeight)
					ptr->MaxHeight = 14;

				PMULTILINES_FONT_INFO newptr = new MULTILINES_FONT_INFO();
				newptr->Reset();

				ptr->m_Next = newptr;

				ptr = newptr;

				ptr->Align = current_align;
				ptr->CharStart = i;
				lastSpace = i - 1;
				charCount = 0;

				if (ptr->Align == TS_LEFT && (current_flags & UOFONT_INDENTION))
					indentionOffset = 14;

				ptr->IndentionOffset = indentionOffset;

				readWidth = indentionOffset;
			}
			else
			{
				if (isFixed)
				{
					MULTILINES_FONT_DATA mfd = {si, current_flags, current_font, 0, current_charcolor};
					ptr->Data.push_back(mfd);

					readWidth += ((char)data[0] + (char)data[2] + 1);

					if (((char)data[1] + (char)data[3]) > ptr->MaxHeight)
						ptr->MaxHeight = ((char)data[1] + (char)data[3]);

					charCount++;

					ptr->Width += readWidth;
					ptr->CharCount += charCount;
				}

				i = lastSpace + 1;

				charcolor = lastspace_charcolor;
				current_charcolor = lastspace_current_charcolor;
				si = str[i];

				if (!ptr->Width)
					ptr->Width = 1;

				if (!ptr->MaxHeight)
					ptr->MaxHeight = 14;

				ptr->Data.resize(ptr->CharCount);

				if (isFixed || isCropped)
					break;

				PMULTILINES_FONT_INFO newptr = new MULTILINES_FONT_INFO();
				newptr->Reset();

				ptr->m_Next = newptr;

				ptr = newptr;

				ptr->Align = current_align;
				ptr->CharStart = i;
				charCount = 0;

				if (ptr->Align == TS_LEFT && (current_flags & UOFONT_INDENTION))
					indentionOffset = 14;

				ptr->IndentionOffset = indentionOffset;

				readWidth = indentionOffset;
			}
		}

		MULTILINES_FONT_DATA mfd = {si, current_flags, current_font, 0, current_charcolor};
		ptr->Data.push_back(mfd);

		if (si == L' ')
		{
			readWidth += UNICODE_SPACE_WIDTH;

			if (!ptr->MaxHeight)
				ptr->MaxHeight = 5;
		}
		else
		{
			readWidth += ((char)data[0] + (char)data[2] + 1);

			if (((char)data[1] + (char)data[3]) > ptr->MaxHeight)
				ptr->MaxHeight = ((char)data[1] + (char)data[3]);
		}

		charCount++;
	}

	ptr->Width += readWidth;
	ptr->CharCount += charCount;

	if (!readWidth && len && (str[len - 1] == L'\n' || str[len - 1] == L'\r'))
	{
		ptr->Width = 1;
		ptr->MaxHeight = 14;
	}

	return info;
}
//----------------------------------------------------------------------------------
/*!
������������� �������� ������
@param [__in] font �����
@param [__inout] th ������ � �������� ������
@param [__in] str �����
@param [__in_opt] color ����
@param [__in_opt] cell ������ � ������� ������
@param [__in_opt] width ������ ��������
@param [__in_opt] align ������������ ������
@param [__in_opt] flags ������� ������
@return true ��� �������� ���������
*/
bool CFontsManager::GenerateW(uchar font, CGLTextTexture &th, const wchar_t *str, ushort color, uchar cell, int width, TEXT_ALIGN_TYPE align, ushort flags)
{
	if ((flags & UOFONT_FIXED) || (flags & UOFONT_CROPPED))
	{
		th.Clear();

		if (!width)
			return false;

		int len = lstrlenW(str);
		if (!len)
			return false;

		int realWidth = GetWidthW(font, str, len);

		if (realWidth > width)
		{
			wstring newstr = GetTextByWidthW(font, str, len, width, (flags & UOFONT_CROPPED));
			return GenerateWBase(font, th, newstr.c_str(), color, cell, width, align, flags);
		}
	}

	return GenerateWBase(font, th, str, color, cell, width, align, flags);
}
//----------------------------------------------------------------------------------
/*!
������������� ������� �������� ������
@param [__in] font �����
@param [__inout] th ������ � �������� ������
@param [__in] str �����
@param [__in] color ���� ������
@param [__in] cell ������ � ������� ������
@param [__in] width ������ ��������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@return ������ �� ������ ��������
*/
UINT_LIST CFontsManager::GeneratePixelsW(uchar &font, CGLTextTexture &th, const wchar_t *str, ushort &color, uchar &cell, int &width, TEXT_ALIGN_TYPE &align, ushort &flags)
{
	UINT_LIST pData;

	if (font >= 20 || !m_UnicodeFontAddress[font])
		return pData;

	int len = lstrlenW(str);
	if (!len)
		return pData;

	int oldWidth = width;

	if (!width)
	{
		width = GetWidthW(font, str, len);

		if (!width)
			return pData;
	}

	PMULTILINES_FONT_INFO info = GetInfoW(font, str, len, align, flags, width);
	if (info == NULL)
		return pData;

	if (!oldWidth && m_RecalculateWidthByInfo)
	{
		PMULTILINES_FONT_INFO ptr = info;
		width = 0;

		while (ptr != NULL)
		{
			if (ptr->Width > width)
				width = ptr->Width;

			ptr = ptr->m_Next;
		}
	}

	width += 4;

	int height = GetHeightW(info);

	if (!height)
	{
		PMULTILINES_FONT_INFO ptr = info;

		while (ptr != NULL)
		{
			info = ptr;

			ptr = ptr->m_Next;

			info->Data.clear();
			delete info;
		}

		return pData;
	}

	height += 4;

	int blocksize = (height + 1) * (width + 1);

	if (m_SavePixels)
		th.PixelsData.resize(blocksize, 0);

	pData.resize(blocksize, 0);

	puint table = (puint)m_UnicodeFontAddress[font];

	int lineOffsY = 1;

	PMULTILINES_FONT_INFO ptr = info;

	uint datacolor = 0;

	if (/*m_UseHTML &&*/ color == 0xFFFF)
		datacolor = 0xFFFFFFFE;
	else
		datacolor = g_ColorManager.GetPolygoneColor(cell, color) << 8 | 0xFF;

	bool isItalic = (flags & UOFONT_ITALIC);
	bool isSolid = (flags & UOFONT_SOLID);
	bool isBlackBorder = (flags & UOFONT_BLACK_BORDER);
	bool isUnderline = (flags & UOFONT_UNDERLINE);
	uint blackColor = 0x010101FF;

	bool isLink = false;
	int linkStartX = 0;
	int linkStartY = 0;

	while (ptr != NULL)
	{
		info = ptr;

		th.IncLinesCount();

		int w = 0;
		if (ptr->Align == TS_CENTER)
		{
			w = (((width - 10) - ptr->Width) / 2);
			if (w < 0)
				w = 0;
		}
		else if (ptr->Align == TS_RIGHT)
		{
			w = ((width - 10) - ptr->Width);

			if (w < 0)
				w = 0;
		}
		else if (ptr->Align == TS_LEFT && (flags & UOFONT_INDENTION))
			w = ptr->IndentionOffset;

		ushort oldLink = 0;

		int dataSize = (int)ptr->Data.size();
		IFOR(i, 0, dataSize)
		{
			wchar_t si = ptr->Data[i].item;

			table = (puint)m_UnicodeFontAddress[ptr->Data[i].font];

			if (!isLink)
			{
				oldLink = ptr->Data[i].linkID;

				if (oldLink)
				{
					isLink = true;
					linkStartX = w;
					linkStartY = lineOffsY + 3;
				}
			}
			else if (!ptr->Data[i].linkID || i + 1 == dataSize)
			{
				isLink = false;
				int linkHeight = lineOffsY - linkStartY;
				if (linkHeight < 14)
					linkHeight = 14;

				int ofsX = 0;

				if (si == L' ')
					ofsX = UNICODE_SPACE_WIDTH;
				else if ((!table[si] || table[si] == 0xFFFFFFFF) && si != L' ')
				{
				}
				else
				{
					puchar xData = (puchar)((uint)table + table[si]);
					ofsX = (char)xData[2];
				}

				WEB_LINK_RECT wlr = { oldLink, linkStartX, linkStartY, w - ofsX, linkHeight };
				th.AddWebLink(wlr);
				oldLink = 0;
			}

			/*if (m_UseHTML)
			{
			if (i >= ptr->Data.size()) break;
			si = ptr->Data[i].item;
			}*/

			if ((!table[si] || table[si] == 0xFFFFFFFF) && si != L' ')
				continue;

			puchar data = (puchar)((uint)table + table[si]);

			int offsX = 0;
			int offsY = 0;
			int dw = 0;
			int dh = 0;

			if (si == L' ')
			{
				offsX = 0;
				dw = UNICODE_SPACE_WIDTH;
			}
			else
			{
				offsX = (char)data[0] + 1;
				offsY = (char)data[1];
				dw = (char)data[2];
				dh = (char)data[3];

				data += 4;
			}

			int tmpW = w;
			uint charcolor = datacolor;

			if (si != L' ')
			{
				if (m_UseHTML && i < (int)ptr->Data.size())
				{
					isItalic = (ptr->Data[i].flags & UOFONT_ITALIC);
					isSolid = (ptr->Data[i].flags & UOFONT_SOLID);
					isBlackBorder = (ptr->Data[i].flags & UOFONT_BLACK_BORDER);
					isUnderline = (ptr->Data[i].flags & UOFONT_UNDERLINE);

					if (ptr->Data[i].color != 0xFFFFFFFF)
						charcolor = ptr->Data[i].color;
				}

				int scanlineCount = (int)((dw - 1) / 8) + 1;

				IFOR(y, 0, dh)
				{
					int testY = offsY + lineOffsY + y;

					if (testY >= height)
						break;

					puchar scanlines = data;
					data += scanlineCount;

					int italicOffset = 0;
					if (isItalic)
						italicOffset = (int)((dh - y) / ITALIC_FONT_KOEFFICIENT);

					int testX = w + offsX + italicOffset + (int)isSolid;

					IFOR(c, 0, scanlineCount)
					{
						IFOR(j, 0, 8)
						{
							int x = (c * 8) + j;
							if (x >= dw)
								break;

							int nowX = testX + x;

							if (nowX >= width)
								break;

							BYTE cl = scanlines[c] & (1 << (7 - j));

							int block = (testY * width) + nowX;

							if (cl)
							{
								pData[block] = charcolor;

								if (m_SavePixels)
									th.PixelsData[block] = cell;
							}
						}
					}
				}

				if (isSolid)
				{
					uint solidColor = blackColor;
					if (solidColor == charcolor)
						solidColor++;

					int minXOk = ((w + offsX) > 0) ? -1 : 0;
					int maxXOk = ((w + offsX + dw) < width) ? 1 : 0;

					maxXOk += dw;

					IFOR(cy, 0, dh)
					{
						int testY = offsY + lineOffsY + cy;

						if (testY >= height)
							break;

						int italicOffset = 0;
						if (isItalic && cy >= 0 && cy < dh)
							italicOffset = (int)((dh - cy) / ITALIC_FONT_KOEFFICIENT);

						IFOR(cx, minXOk, maxXOk)
						{
							int testX = cx + w + offsX + italicOffset;

							if (testX >= width + italicOffset)
								break;

							int block = (testY * width) + testX;

							if (!pData[block] && pData[block] != solidColor)
							{
								int endX = (cx < dw) ? 2 : 1;

								if (endX == 2 && (testX + 1) >= width)
									endX--;

								IFOR(x, 0, endX)
								{
									int nowX = testX + x;

									int testBlock = (testY * width) + nowX;

									if (pData[testBlock] && pData[testBlock] != solidColor)
									{
										pData[block] = solidColor;

										if (m_SavePixels)
											th.PixelsData[block] = cell;

										break;
									}
								}
							}
						}
					}

					IFOR(cy, 0, dh)
					{
						int testY = offsY + lineOffsY + cy;

						if (testY >= height)
							break;

						int italicOffset = 0;
						if (isItalic)
							italicOffset = (int)((dh - cy) / ITALIC_FONT_KOEFFICIENT);

						IFOR(cx, 0, dw)
						{
							int testX = cx + w + offsX + italicOffset;

							if (testX >= width + italicOffset)
								break;

							int block = (testY * width) + testX;

							if (pData[block] == solidColor)
							{
								pData[block] = charcolor;
								if (m_SavePixels)
									th.PixelsData[block] = cell;
							}
						}
					}
				}

				if (isBlackBorder && blackColor != charcolor)
				{
					int minXOk = (w + offsX > 0) ? -1 : 0;
					int minYOk = (offsY + lineOffsY > 0) ? -1 : 0;
					int maxXOk = (w + offsX + dw < width) ? 1 : 0;
					int maxYOk = (offsY + lineOffsY + dh < height) ? 1 : 0;

					maxXOk += dw;
					maxYOk += dh;

					IFOR(cy, minYOk, maxYOk)
					{
						int testY = offsY + lineOffsY + cy;

						if (testY >= height)
							break;

						int italicOffset = 0;
						if (isItalic && cy >= 0 && cy < dh)
							italicOffset = (int)((dh - cy) / ITALIC_FONT_KOEFFICIENT);

						IFOR(cx, minXOk, maxXOk)
						{
							int testX = cx + w + offsX + italicOffset;

							if (testX >= width + italicOffset)
								break;

							int block = (testY * width) + testX;

							if (!pData[block] && pData[block] != blackColor)
							{
								int startX = (cx > 0) ? -1 : 0;
								int startY = (cy > 0) ? -1 : 0;
								int endX = (cx < dw - 1) ? 2 : 1;
								int endY = (cy < dh - 1) ? 2 : 1;

								if (endX == 2 && (testX + 1) >= width)
									endX--;

								bool passed = false;

								IFOR(x, startX, endX)
								{
									int nowX = testX + x;

									IFOR(y, startY, endY)
									{
										int testBlock = ((testY + y) * width) + nowX;

										if (pData[testBlock] && pData[testBlock] != blackColor)
										{
											pData[block] = blackColor;
											if (m_SavePixels)
												th.PixelsData[block] = cell;

											passed = true;

											break;
										}
									}

									if (passed)
										break;
								}
							}
						}
					}
				}

				w += (dw + offsX + (int)isSolid);
			}
			else if (si == L' ')
			{
				w += UNICODE_SPACE_WIDTH;

				if (m_UseHTML)
				{
					isUnderline = (ptr->Data[i].flags & UOFONT_UNDERLINE);

					if (ptr->Data[i].color != 0xFFFFFFFF)
						charcolor = ptr->Data[i].color;
				}
			}

			if (isUnderline)
			{
				int minXOk = ((tmpW + offsX) > 0) ? -1 : 0;
				int maxXOk = ((w + offsX + dw) < width) ? 1 : 0;

				puchar aData = (puchar)((uint)table + table[L'a']);

				int testY = lineOffsY + (char)aData[1] + (char)aData[3];

				if (testY >= height)
					break;

				IFOR(cx, minXOk, dw + maxXOk)
				{
					int testX = (cx + tmpW + offsX + (int)isSolid);

					if (testX >= width)
						break;

					int block = (testY * width) + testX;

					pData[block] = charcolor;
					if (m_SavePixels)
						th.PixelsData[block] = cell;
				}
			}
		}

		lineOffsY += ptr->MaxHeight;

		ptr = ptr->m_Next;

		info->Data.clear();
		delete info;
	}

	th.Width = width;
	th.Height = height;

	return pData;
}
//----------------------------------------------------------------------------------
/*!
�������� Unicode ��������
@param [__in] font �����
@param [__out] th ������ ��������
@param [__in] str �����
@param [__in] color ����
@param [__in] cell ����� ������ � ������� ������
@param [__in] width ������ ��������
@param [__in] align ������������ ������
@param [__in] flags ������� ������
@return true ��� �������� ���������
*/
bool CFontsManager::GenerateWBase(uchar &font, CGLTextTexture &th, const wchar_t *str, ushort &color, uchar &cell, int &width, TEXT_ALIGN_TYPE &align, ushort &flags)
{
	UINT_LIST pixels = GeneratePixelsW(font, th, str, color, cell, width, align, flags);
	bool result = false;

	if (pixels.size())
	{
		g_GL.BindTexture32(th.Texture, th.Width, th.Height, &pixels[0]);

		result = true;
	}

	return result;
}
//----------------------------------------------------------------------------------
/*!
���������� �����
@param [__in] font �����
@param [__in] str �����
@param [__in] color ����
@param [__in] x �������� ���������� X
@param [__in] y �������� ���������� Y
@param [__in_opt] cell ������ � ������� ������
@param [__in_opt] width ������ ��������
@param [__in_opt] align ������������ ������
@param [__in_opt] flags ������� ������
@return
*/
void CFontsManager::DrawW(uchar font, const wchar_t *str, ushort color, int x, int y, uchar cell, int width, TEXT_ALIGN_TYPE align, ushort flags)
{
	CGLTextTexture th;

	if (GenerateW(font, th, str, color, cell, width, align, flags))
		th.Draw(x, y);
}
//----------------------------------------------------------------------------------
//!������� ���������� ASCII �������
const uchar CFontsManager::m_FontIndex[256] =
{
	0xFF, //0
	0xFF, //1
	0xFF, //2
	0xFF, //3
	0xFF, //4
	0xFF, //5
	0xFF, //6
	0xFF, //7
	0xFF, //8
	0xFF, //9
	0xFF, //10
	0xFF, //11
	0xFF, //12
	0xFF, //13
	0xFF, //14
	0xFF, //15
	0xFF, //16
	0xFF, //17
	0xFF, //18
	0xFF, //19
	0xFF, //20
	0xFF, //21
	0xFF, //22
	0xFF, //23
	0xFF, //24
	0xFF, //25
	0xFF, //26
	0xFF, //27
	0xFF, //28
	0xFF, //29
	0xFF, //30
	0xFF, //31
	0,    //32
	1,    //33
	2,    //34
	3,    //35
	4,    //36
	5,    //37
	6,    //38
	7,    //39
	8,    //40
	9,    //41
	10,   //42
	11,   //43
	12,   //44
	13,   //45
	14,   //46
	15,   //47
	16,   //48
	17,   //49
	18,   //50
	19,   //51
	20,   //52
	21,   //53
	22,   //54
	23,   //55
	24,   //56
	25,   //57
	26,   //58
	27,   //59
	28,   //60
	29,   //61
	30,   //62
	31,   //63
	32,   //64
	33,   //65
	34,   //66
	35,   //67
	36,   //68
	37,   //69
	38,   //70
	39,   //71
	40,   //72
	41,   //73
	42,   //74
	43,   //75
	44,   //76
	45,   //77
	46,   //78
	47,   //79
	48,   //80
	49,   //81
	50,   //82
	51,   //83
	52,   //84
	53,   //85
	54,   //86
	55,   //87
	56,   //88
	57,   //89
	58,   //90
	59,   //91
	60,   //92
	61,   //93
	62,   //94
	63,   //95
	64,   //96
	65,   //97
	66,   //98
	67,   //99
	68,   //100
	69,   //101
	70,   //102
	71,   //103
	72,   //104
	73,   //105
	74,   //106
	75,   //107
	76,   //108
	77,   //109
	78,   //110
	79,   //111
	80,   //112
	81,   //113
	82,   //114
	83,   //115
	84,   //116
	85,   //117
	86,   //118
	87,   //119
	88,   //120
	89,   //121
	90,   //122
	0xFF, //123
	0xFF, //124
	0xFF, //125
	0xFF, //126
	0xFF, //127
	0xFF, //128
	0xFF, //129
	0xFF, //130
	0xFF, //131
	0xFF, //132
	0xFF, //133
	0xFF, //134
	0xFF, //135
	0xFF, //136
	0xFF, //137
	0xFF, //138
	0xFF, //139
	0xFF, //140
	0xFF, //141
	0xFF, //142
	0xFF, //143
	0xFF, //144
	0xFF, //145
	0xFF, //146
	0xFF, //147
	0xFF, //148
	0xFF, //149
	0xFF, //150
	0xFF, //151
	0xFF, //152
	0xFF, //153
	0xFF, //154
	0xFF, //155
	0xFF, //156
	0xFF, //157
	0xFF, //158
	0xFF, //159
	0xFF, //160
	0xFF, //161
	0xFF, //162
	0xFF, //163
	0xFF, //164
	0xFF, //165
	0xFF, //166
	0xFF, //167
	136,  //168
	0xFF, //169
	0xFF, //170
	0xFF, //171
	0xFF, //172
	0xFF, //173
	0xFF, //174
	0xFF, //175
	0xFF, //176
	0xFF, //177
	0xFF, //178
	0xFF, //179
	0xFF, //180
	0xFF, //181
	0xFF, //182
	0xFF, //183
	152,  //184
	0xFF, //185
	0xFF, //186
	0xFF, //187
	0xFF, //188
	0xFF, //189
	0xFF, //190
	0xFF, //191
	160,  //192
	161,  //193
	162,  //194
	163,  //195
	164,  //196
	165,  //197
	166,  //198
	167,  //199
	168,  //200
	169,  //201
	170,  //202
	171,  //203
	172,  //204
	173,  //205
	174,  //206
	175,  //207
	176,  //208
	177,  //209
	178,  //210
	179,  //211
	180,  //212
	181,  //213
	182,  //214
	183,  //215
	184,  //216
	185,  //217
	186,  //218
	187,  //219
	188,  //220
	189,  //221
	190,  //222
	191,  //223
	192,  //224
	193,  //225
	194,  //226
	195,  //227
	196,  //228
	197,  //229
	198,  //230
	199,  //231
	200,  //232
	201,  //233
	202,  //234
	203,  //235
	204,  //236
	205,  //237
	206,  //238
	207,  //239
	208,  //240
	209,  //241
	210,  //242
	211,  //243
	212,  //244
	213,  //245
	214,  //246
	215,  //247
	216,  //248
	217,  //249
	218,  //250
	219,  //251
	220,  //252
	221,  //253
	222,  //254
	223   //255
};
//----------------------------------------------------------------------------------
