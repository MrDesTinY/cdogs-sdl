/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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
#include "automap.h"

#include <stdio.h>
#include <string.h>

#include "actors.h"
#include "config.h"
#include "drawtools.h"
#include "map.h"
#include "mission.h"
#include "objs.h"
#include "pic_manager.h"
#include "text.h"


#define MAP_FACTOR 2
#define MASK_ALPHA 128;

color_t colorWall = { 72, 152, 72, 255 };
color_t colorFloor = { 12, 92, 12, 255 };
color_t colorDoor = { 172, 172, 172, 255 };
color_t colorYellowDoor = { 252, 224, 0, 255 };
color_t colorGreenDoor = { 0, 252, 0, 255 };
color_t colorBlueDoor = { 0, 252, 252, 255 };
color_t colorRedDoor = { 132, 0, 0, 255 };
color_t colorExit = { 255, 255, 255, 255 };



static void DisplayPlayer(TActor *player, Vec2i pos, int scale)
{
	Vec2i playerPos = Vec2iNew(
		player->tileItem.x / TILE_WIDTH,
		player->tileItem.y / TILE_HEIGHT);
	pos = Vec2iAdd(pos, Vec2iScale(playerPos, scale));
	if (scale >= 2)
	{
		Character *c = player->character;
		int picIdx = cHeadPic[c->looks.face][DIRECTION_DOWN][STATE_IDLE];
		PicPaletted *pic = PicManagerGetOldPic(&gPicManager, picIdx);
		pos.x -= pic->w / 2;
		pos.y -= pic->h / 2;
		DrawTTPic(pos.x, pos.y, pic, c->table);
	}
	else
	{
		Draw_Point(pos.x, pos.y, colorWhite);
	}
}

static void DisplayObjective(
	TTileItem *t, int objectiveIndex, Vec2i pos, int scale, int flags)
{
	Vec2i objectivePos = Vec2iNew(t->x / TILE_WIDTH, t->y / TILE_HEIGHT);
	color_t color = gMission.objectives[objectiveIndex].color;
	pos = Vec2iAdd(pos, Vec2iScale(objectivePos, scale));
	if (flags & AUTOMAP_FLAGS_MASK)
	{
		color.a = MASK_ALPHA;
	}
	if (scale >= 2)
	{
		DrawCross(&gGraphicsDevice, pos.x, pos.y, color);
	}
	else
	{
		Draw_Point(pos.x, pos.y, color);
	}
}

static void DisplayExit(Vec2i pos, int scale, int flags)
{
	Vec2i exitPos = Vec2iNew(
		gMission.exitLeft, gMission.exitTop);
	Vec2i exitSize = Vec2iAdd(
		Vec2iNew(gMission.exitRight, gMission.exitBottom),
		Vec2iScale(exitPos, -1));
	color_t color = colorExit;

	if (!CanCompleteMission(&gMission))
	{
		return;
	}
	if (gCampaign.Entry.mode == CAMPAIGN_MODE_DOGFIGHT)
	{
		return;
	}
	
	exitPos = Vec2iScale(exitPos, scale);
	exitSize = Vec2iScale(exitSize, scale);
	exitPos.x /= TILE_WIDTH;
	exitSize.x /= TILE_WIDTH;
	exitPos.y /= TILE_HEIGHT;
	exitSize.y /= TILE_HEIGHT;
	exitPos = Vec2iAdd(exitPos, pos);

	if (flags & AUTOMAP_FLAGS_MASK)
	{
		color.a = MASK_ALPHA;
	}
	DrawRectangle(&gGraphicsDevice, exitPos, exitSize, color, DRAW_FLAG_LINE);
}

static void DisplaySummary(void)
{
	int i, y, x, x2;
	char sScore[20];

	y = gGraphicsDevice.cachedConfig.ResolutionHeight - 5 - CDogsTextHeight(); // 10 pixels from bottom

	for (i = 0; i < gMission.missionData->objectiveCount; i++) {
		if (gMission.objectives[i].required > 0 ||
			gMission.objectives[i].done > 0)
		{
			x = 5;
			// Objective color dot
			Draw_Rect(x, (y + 3), 2, 2, gMission.objectives[i].color);

			x += 5;
			x2 = x + TextGetStringWidth(gMission.missionData->objectives[i].description) + 5;

			sprintf(sScore, "(%d)", gMission.objectives[i].done);

			if (gMission.objectives[i].required <= 0) {
				CDogsTextStringWithTableAt(x, y,
						      gMission.missionData->objectives[i].description,
						      &tablePurple);
				CDogsTextStringWithTableAt(x2, y, sScore, &tablePurple);
			} else if (gMission.objectives[i].done >= gMission.objectives[i].required) {
				CDogsTextStringWithTableAt(x, y,
						      gMission.missionData->objectives[i].description,
						      &tableFlamed);
				CDogsTextStringWithTableAt(x2, y, sScore, &tableFlamed);
			} else {
				CDogsTextStringAt(x, y, gMission.missionData->objectives[i].description);
				CDogsTextStringAt(x2, y, sScore);
			}
			y -= (CDogsTextHeight() + 1);
		}
	}
}

static int MapLevel(int x, int y)
{
	int l;

	l = MapAccessLevel(x - 1, y);
	if (l)
		return l;
	l = MapAccessLevel(x + 1, y);
	if (l)
		return l;
	l = MapAccessLevel(x, y - 1);
	if (l)
		return l;
	return MapAccessLevel(x, y + 1);
}

color_t DoorColor(int x, int y)
{
	int l = MapLevel(x, y);

	switch (l) {
	case FLAGS_KEYCARD_YELLOW:
		return colorYellowDoor;
	case FLAGS_KEYCARD_GREEN:
		return colorGreenDoor;
	case FLAGS_KEYCARD_BLUE:
		return colorBlueDoor;
	case FLAGS_KEYCARD_RED:
		return colorRedDoor;
	default:
		return colorDoor;
	}
}

void DrawDot(TTileItem *t, color_t color, Vec2i pos, int scale)
{
	Vec2i dotPos = Vec2iNew(t->x / TILE_WIDTH, t->y / TILE_HEIGHT);
	pos = Vec2iAdd(pos, Vec2iScale(dotPos, scale));
	Draw_Rect(pos.x, pos.y, scale, scale, color);
}

static void DrawMap(
	Tile map[YMAX][XMAX],
	Vec2i center, Vec2i centerOn, Vec2i size,
	int scale, int flags)
{
	int x, y;
	Vec2i mapPos = Vec2iAdd(center, Vec2iScale(centerOn, -scale));
	for (y = 0; y < YMAX; y++)
	{
		int i;
		for (i = 0; i < scale; i++)
		{
			for (x = 0; x < XMAX; x++)
			{
				Tile *tile = &map[y][x];
				if (!(tile->flags & MAPTILE_IS_NOTHING) &&
					(tile->isVisited || (flags & AUTOMAP_FLAGS_SHOWALL)))
				{
					int j;
					for (j = 0; j < scale; j++)
					{
						Vec2i drawPos = Vec2iNew(
							mapPos.x + x*scale + j,
							mapPos.y + y*scale + i);
						color_t color = colorBlack;
						if (tile->flags & MAPTILE_IS_WALL)
						{
							color = colorWall;
						}
						else if (tile->flags & MAPTILE_NO_WALK)
						{
							color = DoorColor(x, y);
						}
						else
						{
							color = colorFloor;
						}
						if (!ColorEquals(color, colorBlack))
						{
							if (flags & AUTOMAP_FLAGS_MASK)
							{
								color.a = MASK_ALPHA;
							}
							Draw_Point(drawPos.x, drawPos.y, color);
						}
					}
				}
			}
		}
	}
	if (flags & AUTOMAP_FLAGS_MASK)
	{
		color_t color = { 255, 255, 255, 128 };
		Draw_Rect(
			center.x - size.x / 2,
			center.y - size.y / 2,
			size.x, size.y,
			color);
	}
}

static void DrawObjectivesAndKeys(
	Tile map[YMAX][XMAX], Vec2i pos, int scale, int flags)
{
	int y;
	for (y = 0; y < YMAX; y++)
	{
		int x;
		for (x = 0; x < XMAX; x++)
		{
			TTileItem *t = map[y][x].things;
			while (t)
			{
				if ((t->flags & TILEITEM_OBJECTIVE) != 0)
				{
					int obj = ObjectiveFromTileItem(t->flags);
					int objFlags = gMission.missionData->objectives[obj].flags;
					if (!(objFlags & OBJECTIVE_HIDDEN) ||
						(flags & AUTOMAP_FLAGS_SHOWALL))
					{
						if ((objFlags & OBJECTIVE_POSKNOWN) ||
							map[y][x].isVisited ||
							(flags & AUTOMAP_FLAGS_SHOWALL))
						{
							DisplayObjective(t, obj, pos, scale, flags);
						}
					}
				}
				else if (t->kind == KIND_OBJECT &&
					t->data &&
					map[y][x].isVisited)
				{
					color_t dotColor = colorBlack;
					switch (((TObject *)t->data)->objectIndex)
					{
					case OBJ_KEYCARD_RED:
						dotColor = colorRedDoor;
						break;
					case OBJ_KEYCARD_BLUE:
						dotColor = colorBlueDoor;
						break;
					case OBJ_KEYCARD_GREEN:
						dotColor = colorGreenDoor;
						break;
					case OBJ_KEYCARD_YELLOW:
						dotColor = colorYellowDoor;
						break;
					default:
						break;
					}
					if (!ColorEquals(dotColor, colorBlack))
					{
						DrawDot(t, dotColor, pos, scale);
					}
				}

				t = t->next;
			}
		}
	}
}

void AutomapDraw(int flags)
{
	int x, y;
	int i;
	color_t mask = { 0, 128, 0, 255 };
	Vec2i mapCenter = Vec2iNew(
		gGraphicsDevice.cachedConfig.ResolutionWidth / 2,
		gGraphicsDevice.cachedConfig.ResolutionHeight / 2);
	Vec2i centerOn = Vec2iNew(XMAX / 2, YMAX / 2);
	Vec2i pos = Vec2iAdd(mapCenter, Vec2iScale(centerOn, -MAP_FACTOR));

	// Draw faded green overlay
	for (y = 0; y < gGraphicsDevice.cachedConfig.ResolutionHeight; y++)
	{
		for (x = 0; x < gGraphicsDevice.cachedConfig.ResolutionWidth; x++)
		{
			DrawPointMask(&gGraphicsDevice, Vec2iNew(x, y), mask);
		}
	}

	DrawMap(
		gMap,
		mapCenter,
		centerOn,
		Vec2iNew(XMAX, YMAX),
		MAP_FACTOR,
		flags);

	DrawObjectivesAndKeys(gMap, pos, MAP_FACTOR, flags);

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (gPlayers[i])
		{
			DisplayPlayer(gPlayers[i], pos, MAP_FACTOR);
		}
	}

	DisplayExit(pos, MAP_FACTOR, flags);
	DisplaySummary();
}

void AutomapDrawRegion(
	Tile map[YMAX][XMAX],
	Vec2i pos, Vec2i size, Vec2i mapCenter,
	int scale, int flags)
{
	Vec2i centerOn;
	BlitClipping oldClip = gGraphicsDevice.clipping;
	int i;
	GraphicsSetBlitClip(
		&gGraphicsDevice,
		pos.x, pos.y, pos.x + size.x - 1, pos.y + size.y - 1);
	pos = Vec2iAdd(pos, Vec2iScaleDiv(size, 2));
	DrawMap(map, pos, mapCenter, size, scale, flags);
	centerOn = Vec2iAdd(pos, Vec2iScale(mapCenter, -scale));
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerAlive(i))
		{
			TActor *player = gPlayers[i];
			DisplayPlayer(player, centerOn, scale);
		}
	}
	DrawObjectivesAndKeys(gMap, centerOn, scale, flags);
	DisplayExit(centerOn, scale, flags);
	GraphicsSetBlitClip(
		&gGraphicsDevice,
		oldClip.left, oldClip.top, oldClip.right, oldClip.bottom);
}
