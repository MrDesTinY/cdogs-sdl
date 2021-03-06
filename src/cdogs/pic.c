/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
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
#include "pic.h"

#include "palette.h"
#include "utils.h"

Pic picNone = { { 0, 0 }, { 0, 0 }, NULL };

void PicFromPicPaletted(Pic *pic, PicPaletted *picP)
{
	int i;
	pic->size = Vec2iNew(picP->w, picP->h);
	pic->offset = Vec2iZero();
	CMALLOC(pic->data, pic->size.x * pic->size.y * sizeof *pic->data);
	for (i = 0; i < pic->size.x * pic->size.y; i++)
	{
		pic->data[i] = PaletteToColor(*(picP->data + i));
	}
}
void PicFromPicPalettedOffset(
	Pic *pic, PicPaletted *picP, const TOffsetPic *picOff)
{
	PicFromPicPaletted(pic, picP);
	pic->offset = Vec2iNew(picOff->dx, picOff->dy);
}

void PicFree(Pic *pic)
{
	CFREE(pic->data);
}

int PicIsNotNone(Pic *pic)
{
	return pic->size.x > 0 && pic->size.y > 0 && pic->data != NULL;
}
