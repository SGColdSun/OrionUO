/***********************************************************************************
**
** GLTexture.cpp
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "GLTexture.h"
#include "GLEngine.h"
//----------------------------------------------------------------------------------
CGLTexture::CGLTexture()
: m_Width(0), m_Height(0), Texture(0)
{
}
//----------------------------------------------------------------------------------
CGLTexture::~CGLTexture()
{
	Clear();
}
//----------------------------------------------------------------------------------
void CGLTexture::Draw(const int &x, const int &y, const bool &checktrans)
{
	if (Texture != 0)
	{
		if (checktrans)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			g_GL.Draw(Texture, x, y, m_Width, m_Height);

			glDisable(GL_BLEND);

			glEnable(GL_STENCIL_TEST);

			g_GL.Draw(Texture, x, y, m_Width, m_Height);

			glDisable(GL_STENCIL_TEST);
		}
		else
			g_GL.Draw(Texture, x, y, m_Width, m_Height);
	}
}
//----------------------------------------------------------------------------------
void CGLTexture::Draw(const int &x, const int &y, int width, int height, const bool &checktrans)
{
	if (Texture != 0)
	{
		if (!width)
			width = m_Width;

		if (!height)
			height = m_Height;

		if (checktrans)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			g_GL.Draw(Texture, x, y, m_Width, m_Height, width, height);

			glDisable(GL_BLEND);

			glEnable(GL_STENCIL_TEST);

			g_GL.Draw(Texture, x, y, m_Width, m_Height, width, height);

			glDisable(GL_STENCIL_TEST);
		}
		else
			g_GL.Draw(Texture, x, y, m_Width, m_Height, width, height);
	}
}
//----------------------------------------------------------------------------------
void CGLTexture::DrawRotated(const int &x, const int &y, const float &angle)
{
	if (Texture != 0)
		g_GL.DrawRotated(Texture, x, y, m_Width, m_Height / 2, angle);
}
//----------------------------------------------------------------------------------
void CGLTexture::DrawTransparent(const int &x, const int &y, const bool &stencil)
{
	if (Texture != 0)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0f, 1.0f, 1.0f, 0.25f);

		g_GL.Draw(Texture, x, y, m_Width, m_Height);

		glDisable(GL_BLEND);

		if (stencil)
		{
			glEnable(GL_STENCIL_TEST);

			g_GL.Draw(Texture, x, y, m_Width, m_Height);

			glDisable(GL_STENCIL_TEST);
		}
	}
}
//----------------------------------------------------------------------------------
void CGLTexture::Clear()
{
	m_Width = 0;
	m_Height = 0;

	if (Texture != 0)
	{
		glDeleteTextures(1, &Texture);
		Texture = 0;
	}

#if UO_ENABLE_TEXTURE_DATA_SAVING == 1
	PixelsData.clear();
#endif
}
//----------------------------------------------------------------------------------