/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013, Cong Xu
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#include "text.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "grafx.h"
#include "blit.h"
#include "actors.h" /* for tableFlamed */

#define FIRST_CHAR      0
#define LAST_CHAR       153
#define CHARS_IN_FONT   (LAST_CHAR - FIRST_CHAR + 1)

#define CHAR_INDEX(c) ((int)c - FIRST_CHAR)


static int dxCDogsText = 0;
static int xCDogsText = 0;
static int yCDogsText = 0;
static int hCDogsText = 0;
static PicPaletted *gFont[CHARS_IN_FONT];


void CDogsTextInit(const char *filename, int offset)
{
	int i;

	dxCDogsText = offset;
	memset(gFont, 0, sizeof(gFont));
	ReadPics(filename, gFont, CHARS_IN_FONT, NULL);

	for (i = 0; i < CHARS_IN_FONT; i++)
	{
		if (gFont[i] != NULL)
		{
			hCDogsText = MAX(hCDogsText, gFont[i]->h);
		}
	}
}

void CDogsTextChar(char c)
{
	int i = CHAR_INDEX(c);
	if (i >= 0 && i <= CHARS_IN_FONT && gFont[i])
	{
		DrawTPic(xCDogsText, yCDogsText, gFont[i]);
		xCDogsText += 1 + gFont[i]->w + dxCDogsText;
	}
	else
	{
		i = CHAR_INDEX('.');
		DrawTPic(xCDogsText, yCDogsText, gFont[i]);
		xCDogsText += 1 + gFont[i]->w + dxCDogsText;
	}
}

void CDogsTextCharWithTable(char c, TranslationTable * table)
{
	int i = CHAR_INDEX(c);
	if (i >= 0 && i <= CHARS_IN_FONT && gFont[i])
	{
		DrawTTPic(xCDogsText, yCDogsText, gFont[i], table);
		xCDogsText += 1 + gFont[i]->w + dxCDogsText;
	}
	else
	{
		i = CHAR_INDEX('.');
		DrawTTPic(xCDogsText, yCDogsText, gFont[i], table);
		xCDogsText += 1 + gFont[i]->w + dxCDogsText;
	}
}

void CDogsTextString(const char *s)
{
	while (*s)
		CDogsTextChar(*s++);
}

void CDogsTextStringWithTable(const char *s, TranslationTable * table)
{
	while (*s)
		CDogsTextCharWithTable(*s++, table);
}

static PicPaletted *GetgFontPic(char c)
{
	int i = CHAR_INDEX(c);
	if (i < 0 || i > CHARS_IN_FONT || !gFont[i])
	{
		i = CHAR_INDEX('.');
	}
	assert(gFont[i]);
	return gFont[i];
}

Vec2i DrawTextCharMasked(
	char c, GraphicsDevice *device, Vec2i pos, color_t mask)
{
	PicPaletted *font = GetgFontPic(c);
	Pic pic;
	PicFromPicPaletted(&pic, font);
	BlitMasked(device, &pic, pos, mask, 1);
	pos.x += 1 + font->w + dxCDogsText;
	CDogsTextGoto(pos.x, pos.y);
	PicFree(&pic);
	return pos;
}

Vec2i DrawTextStringMasked(
	const char *s, GraphicsDevice *device, Vec2i pos, color_t mask)
{
	int left = pos.x;
	while (*s)
	{
		if (*s == '\n')
		{
			pos.x = left;
			pos.y += CDogsTextHeight();
		}
		else
		{
			pos = DrawTextCharMasked(*s, device, pos, mask);
		}
		s++;
	}
	return pos;
}

Vec2i DrawTextString(const char *s, GraphicsDevice *device, Vec2i pos)
{
	return DrawTextStringMasked(s, device, pos, colorWhite);
}

Vec2i TextGetSize(const char *s)
{
	Vec2i size = Vec2iZero();
	while (*s)
	{
		char *lineEnd = strchr(s, '\n');
		size.y += CDogsTextHeight();
		if (lineEnd)
		{
			size.x = MAX(size.x, TextGetSubstringWidth(s, lineEnd - s));
			s = lineEnd + 1;
		}
		else
		{
			size.x = MAX(size.x, TextGetStringWidth(s));
			s += strlen(s);
		}
	}
	return size;
}

void CDogsTextGoto(int x, int y)
{
	xCDogsText = x;
	yCDogsText = y;
}

void CDogsTextStringAt(int x, int y, const char *s)
{
	CDogsTextGoto(x, y);
	CDogsTextString(s);
}

void CDogsTextIntAt(int x, int y, int i)
{
	char s[32];
	CDogsTextGoto(x, y);
	sprintf(s, "%d", i);
	CDogsTextString(s);
}

void CDogsTextFormatAt(int x, int y, const char *fmt, ...)
{
	char s[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(s, fmt, argptr);
	va_end(argptr);
	CDogsTextGoto(x, y);
	CDogsTextString(s);
}

void CDogsTextStringWithTableAt(int x, int y, const char *s,
			   TranslationTable * table)
{
	CDogsTextGoto(x, y);
	CDogsTextStringWithTable(s, table);
}

int CDogsTextCharWidth(int c)
{
	if (c >= FIRST_CHAR && c <= LAST_CHAR && gFont[CHAR_INDEX(c)])
	{
		return 1 + gFont[CHAR_INDEX(c)]->w + dxCDogsText;
	}
	else
	{
		return 1 + gFont[CHAR_INDEX('.')]->w + dxCDogsText;
	}
}

int TextGetSubstringWidth(const char *s, int len)
{
	int w = 0;
	int i;
	if (len > (int)strlen(s))
	{
		len = (int)strlen(s);
	}
	for (i = 0; i < len; i++)
	{
		w += CDogsTextCharWidth(*s++);
	}
	return w;
}

int TextGetStringWidth(const char *s)
{
	return TextGetSubstringWidth(s, (int)strlen(s));
}

#define FLAG_SET(a, b)	((a & b) != 0)

void DrawTextStringSpecial(
	const char *s, unsigned int opts, Vec2i pos, Vec2i size, Vec2i padding)
{
	int x = 0;
	int y = 0;
	int w = TextGetStringWidth(s);
	int h = CDogsTextHeight();

	if (FLAG_SET(opts, TEXT_XCENTER))	{ x = pos.x + (size.x - w) / 2; }
	if (FLAG_SET(opts, TEXT_YCENTER))	{ y = pos.y + (size.y - h) / 2; }

	if (FLAG_SET(opts, TEXT_LEFT))		{ x = pos.x + padding.x; }
	if (FLAG_SET(opts, TEXT_RIGHT))		{ x = pos.x + size.x - w - padding.x; }

	if (FLAG_SET(opts, TEXT_TOP))		{ y = pos.y + padding.y; }
	if (FLAG_SET(opts, TEXT_BOTTOM))	{ y = pos.y + size.y - h - padding.y; }

	if (FLAG_SET(opts, TEXT_FLAMED))
	{
		CDogsTextStringWithTableAt(x, y, s, &tableFlamed);
	}
	else if (FLAG_SET(opts, TEXT_PURPLE))
	{
		CDogsTextStringWithTableAt(x, y, s, &tablePurple);
	}
	else
	{
		CDogsTextStringAt(x, y, s);
	}
}

void CDogsTextStringSpecial(const char *s, unsigned int opts, unsigned int xpad, unsigned int ypad)
{
	int scrw = gGraphicsDevice.cachedConfig.ResolutionWidth;
	int scrh = gGraphicsDevice.cachedConfig.ResolutionHeight;
	DrawTextStringSpecial(
		s, opts, Vec2iZero(), Vec2iNew(scrw, scrh), Vec2iNew(xpad, ypad));
}

int CDogsTextHeight(void)
{
	return hCDogsText;
}

char *PercentStr(int p)
{
	static char buf[8];
	sprintf(buf, "%d%%", p);
	return buf;
}
char *Div8Str(int i)
{
	static char buf[8];
	sprintf(buf, "%d", i/8);
	return buf;
}
