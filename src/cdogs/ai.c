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
#include "ai.h"

#include <assert.h>
#include <stdlib.h>

#include "collision.h"
#include "config.h"
#include "defs.h"
#include "actors.h"
#include "gamedata.h"
#include "mission.h"
#include "sys_specifics.h"
#include "utils.h"

static int gBaddieCount = 0;
static int gAreGoodGuysPresent = 0;


static int IsFacing(TActor *a, TActor *a2, direction_e d)
{
	switch (d)
	{
	case DIRECTION_UP:
		return (a->y > a2->y);
	case DIRECTION_UPLEFT:
		return (a->y > a2->y && a->x > a2->x);
	case DIRECTION_LEFT:
		return a->x > a2->x;
	case DIRECTION_DOWNLEFT:
		return (a->y < a2->y && a->x > a2->x);
	case DIRECTION_DOWN:
		return a->y < a2->y;
	case DIRECTION_DOWNRIGHT:
		return (a->y < a2->y && a->x < a2->x);
	case DIRECTION_RIGHT:
		return a->x < a2->x;
	case DIRECTION_UPRIGHT:
		return (a->y > a2->y && a->x < a2->x);
	default:
		// should nver get here
		assert(0);
		break;
	}
	return NO;
}


static int IsFacingPlayer(TActor *actor, direction_e d)
{
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerAlive(i) && IsFacing(actor, gPlayers[i], d))
		{
			return 1;
		}
	}
	return 0;
}


#define Distance(a,b) CHEBYSHEV_DISTANCE(a->x, a->y, b->x, b->y)

TActor *GetClosestEnemy(Vec2i from, int flags, int isPlayer)
{
	// Search all the actors and find the closest one that is an enemy
	TActor *a;
	TActor *closestEnemy = NULL;
	int minDistance = -1;
	for (a = ActorList(); a; a = a->next)
	{
		int isEnemy = 0;
		int distance;
		// Never target invulnerables or victims
		if (a->flags & (FLAGS_INVULNERABLE | FLAGS_VICTIM))
		{
			continue;
		}
		if (a->pData || (a->flags & FLAGS_GOOD_GUY))
		{
			// target is good guy / player, check if we are bad
			if (!isPlayer && !(flags & FLAGS_GOOD_GUY))
			{
				isEnemy = 1;
			}
		}
		else
		{
			// target is bad guy, check if we are good
			if (isPlayer || (flags & FLAGS_GOOD_GUY))
			{
				isEnemy = 1;
			}
		}
		if (isEnemy)
		{
			distance = CHEBYSHEV_DISTANCE(from.x, from.y, a->x, a->y);
			if (!closestEnemy || distance < minDistance)
			{
				minDistance = distance;
				closestEnemy = a;
			}
		}
	}
	return closestEnemy;
}

static TActor *GetClosestPlayer(Vec2i pos)
{
	int i;
	int minDistance = -1;
	TActor *closestPlayer = NULL;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerAlive(i))
		{
			TActor *p = gPlayers[i];
			int distance = CHEBYSHEV_DISTANCE(pos.x, pos.y, p->x, p->y);
			if (!closestPlayer || distance < minDistance)
			{
				closestPlayer = p;
				minDistance = distance;
			}
		}
	}
	return closestPlayer;
}

static Vec2i GetClosestPlayerPos(Vec2i pos)
{
	TActor *closestPlayer = GetClosestPlayer(pos);
	if (closestPlayer)
	{
		return Vec2iNew(closestPlayer->x, closestPlayer->y);
	}
	else
	{
		return pos;
	}
}

static int IsCloseToPlayer(Vec2i pos)
{
	TActor *closestPlayer = GetClosestPlayer(pos);
	if (closestPlayer && CHEBYSHEV_DISTANCE(
			pos.x, pos.y, closestPlayer->x, closestPlayer->y) < (32 << 8))
	{
		return 1;
	}
	return 0;
}

static int Follow(TActor * actor)
{
	int cmd = 0;
	Vec2i p = GetClosestPlayerPos(Vec2iNew(actor->x, actor->y));
	p.x >>= 8;
	p.y >>= 8;

	if ((actor->x >> 8) < p.x - 1)		cmd |= CMD_RIGHT;
	else if ((actor->x >> 8) > p.x + 1)	cmd |= CMD_LEFT;

	if ((actor->y >> 8) < p.y - 1)		cmd |= CMD_DOWN;
	else if ((actor->y >> 8) > p.y + 1)	cmd |= CMD_UP;

	return cmd;
}


static int ReverseDirection(int cmd)
{
	if (cmd & (CMD_LEFT | CMD_RIGHT))
	{
		cmd ^= CMD_LEFT | CMD_RIGHT;
	}
	if (cmd & (CMD_UP | CMD_DOWN))
	{
		cmd ^= CMD_UP | CMD_DOWN;
	}
	return cmd;
}

static int Hunt(TActor * actor)
{
	int cmd = 0;
	int dx, dy;
	Vec2i targetPos = Vec2iNew(actor->x, actor->y);
	if (!(actor->pData || (actor->flags & FLAGS_GOOD_GUY)))
	{
		targetPos = GetClosestPlayerPos(Vec2iNew(actor->x, actor->y));
	}

	if (actor->flags & FLAGS_VISIBLE)
	{
		TActor *a = GetClosestEnemy(
			Vec2iNew(actor->x, actor->y), actor->flags, !!actor->pData);
		if (a)
		{
			targetPos.x = a->x;
			targetPos.y = a->y;
		}
	}

	dx = abs(targetPos.x - actor->x);
	dy = abs(targetPos.y - actor->y);

	if (2 * dx > dy)
	{
		if (actor->x < targetPos.x)			cmd |= CMD_RIGHT;
		else if (actor->x > targetPos.x)	cmd |= CMD_LEFT;
	}
	if (2 * dy > dx)
	{
		if (actor->y < targetPos.y)			cmd |= CMD_DOWN;
		else if (actor->y > targetPos.y)	cmd |= CMD_UP;
	}
	// If it's a coward, reverse directions...
	if (actor->flags & FLAGS_RUNS_AWAY)
	{
		cmd = ReverseDirection(cmd);
	}

	return cmd;
}


static int PositionOK(TActor * actor, int x, int y)
{
	Vec2i realPos = Vec2iScaleDiv(Vec2iNew(x, y), 256);
	Vec2i size = Vec2iNew(actor->tileItem.w, actor->tileItem.h);
	if (IsCollisionWithWall(realPos, size))
	{
		return 0;
	}
	if (GetItemOnTileInCollision(
		&actor->tileItem, realPos, TILEITEM_IMPASSABLE,
		CalcCollisionTeam(1, actor)))
	{
		return 0;
	}
	return 1;
}

#define STEPSIZE    1024

static int DirectionOK(TActor * actor, int dir)
{
	switch (dir) {
	case DIRECTION_UP:
		return PositionOK(actor, actor->x, actor->y - STEPSIZE);
	case DIRECTION_UPLEFT:
		return PositionOK(actor, actor->x - STEPSIZE,
				  actor->y - STEPSIZE)
		    || PositionOK(actor, actor->x - STEPSIZE, actor->y)
		    || PositionOK(actor, actor->x, actor->y - STEPSIZE);
	case DIRECTION_LEFT:
		return PositionOK(actor, actor->x - STEPSIZE, actor->y);
	case DIRECTION_DOWNLEFT:
		return PositionOK(actor, actor->x - STEPSIZE,
				  actor->y + STEPSIZE)
		    || PositionOK(actor, actor->x - STEPSIZE, actor->y)
		    || PositionOK(actor, actor->x, actor->y + STEPSIZE);
	case DIRECTION_DOWN:
		return PositionOK(actor, actor->x, actor->y + STEPSIZE);
	case DIRECTION_DOWNRIGHT:
		return PositionOK(actor, actor->x + STEPSIZE,
				  actor->y + STEPSIZE)
		    || PositionOK(actor, actor->x + STEPSIZE, actor->y)
		    || PositionOK(actor, actor->x, actor->y + STEPSIZE);
	case DIRECTION_RIGHT:
		return PositionOK(actor, actor->x + STEPSIZE, actor->y);
	case DIRECTION_UPRIGHT:
		return PositionOK(actor, actor->x + STEPSIZE,
				  actor->y - STEPSIZE)
		    || PositionOK(actor, actor->x + STEPSIZE, actor->y)
		    || PositionOK(actor, actor->x, actor->y - STEPSIZE);
	}
	return NO;
}


static int BrightWalk(TActor * actor, int roll)
{
	if (!!(actor->flags & FLAGS_VISIBLE) &&
		roll < actor->character->bot.probabilityToTrack)
	{
		actor->flags &= ~FLAGS_DETOURING;
		return Hunt(actor);
	}

	if (actor->flags & FLAGS_TRYRIGHT) {
		if (DirectionOK(actor, (actor->direction + 7) % 8)) {
			actor->direction = (actor->direction + 7) % 8;
			actor->turns--;
			if (actor->turns == 0)
				actor->flags &= ~FLAGS_DETOURING;
		} else if (!DirectionOK(actor, actor->direction)) {
			actor->direction = (actor->direction + 1) % 8;
			actor->turns++;
			if (actor->turns == 4) {
				actor->flags &=
				    ~(FLAGS_DETOURING | FLAGS_TRYRIGHT);
				actor->turns = 0;
			}
		}
	} else {
		if (DirectionOK(actor, (actor->direction + 1) % 8)) {
			actor->direction = (actor->direction + 1) % 8;
			actor->turns--;
			if (actor->turns == 0)
				actor->flags &= ~FLAGS_DETOURING;
		} else if (!DirectionOK(actor, actor->direction)) {
			actor->direction = (actor->direction + 7) % 8;
			actor->turns++;
			if (actor->turns == 4) {
				actor->flags &=
				    ~(FLAGS_DETOURING | FLAGS_TRYRIGHT);
				actor->turns = 0;
			}
		}
	}
	return DirectionToCmd(actor->direction);
}

static int WillFire(TActor * actor, int roll)
{
	if ((actor->flags & FLAGS_VISIBLE) != 0 &&
		WeaponCanFire(&actor->weapon) &&
		roll < actor->character->bot.probabilityToShoot)
	{
		if ((actor->flags & FLAGS_GOOD_GUY) != 0)
			return 1;	//!FacingPlayer( actor);
		else if (gAreGoodGuysPresent)
		{
			return 1;
		}
		else
		{
			return IsFacingPlayer(actor, actor->direction);
		}
	}
	return 0;
}

void Detour(TActor * actor)
{
	actor->flags |= FLAGS_DETOURING;
	actor->turns = 1;
	if (actor->flags & FLAGS_TRYRIGHT)
		actor->direction =
		    (CmdToDirection(actor->lastCmd) + 1) % 8;
	else
		actor->direction =
		    (CmdToDirection(actor->lastCmd) + 7) % 8;
}

static int IsActorPositionValid(TActor *actor)
{
	Vec2i pos = Vec2iNew(actor->x, actor->y);
	actor->x = actor->y = 0;
	return MoveActor(actor, pos.x, pos.y);
}

static void PlaceBaddie(TActor *actor)
{
	int hasPlaced = 0;
	int i;
	for (i = 0; i < 100; i++)	// Don't try forever trying to place baddie
	{
		// Try spawning out of players' sights
		TActor *closestPlayer = NULL;
		do
		{
			actor->x = (rand() % (XMAX * TILE_WIDTH)) << 8;
			actor->y = (rand() % (YMAX * TILE_HEIGHT)) << 8;
			closestPlayer = GetClosestPlayer(Vec2iNew(actor->x, actor->y));
		}
		while (closestPlayer && CHEBYSHEV_DISTANCE(
			actor->x, actor->y, closestPlayer->x, closestPlayer->y) <
			256 * 150);
		if (IsActorPositionValid(actor))
		{
			hasPlaced = 1;
			break;
		}
	}
	// Keep trying, but this time try spawning anywhere, even close to player
	while (!hasPlaced)
	{
		actor->x = (rand() % (XMAX * TILE_WIDTH)) << 8;
		actor->y = (rand() % (YMAX * TILE_HEIGHT)) << 8;
		if (IsActorPositionValid(actor))
		{
			hasPlaced = 1;
			break;
		}
	}

	actor->direction = rand() % DIRECTION_COUNT;

	actor->health = (actor->health * gConfig.Game.NonPlayerHP) / 100;
	if (actor->health <= 0)
	{
		actor->health = 1;
	}
	if (actor->flags & FLAGS_AWAKEALWAYS)
	{
		actor->flags &= ~FLAGS_SLEEPING;
	}
	else if (!(actor->flags & FLAGS_SLEEPALWAYS) &&
		rand() % 100 < gBaddieCount)
	{
		actor->flags &= ~FLAGS_SLEEPING;
	}
}

static void PlacePrisoner(TActor * actor)
{
	int x, y;

	do {
		do {
			actor->x = ((rand() % (XMAX * TILE_WIDTH)) << 8);
			actor->y = ((rand() % (YMAX * TILE_HEIGHT)) << 8);
		}
		while (!IsHighAccess(actor->x >> 8, actor->y >> 8));
		x = actor->x;
		y = actor->y;
		actor->x = actor->y = 0;
	}
	while (!MoveActor(actor, x, y));
}


static int DidPlayerShoot(void)
{
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (gPlayers[i] && !!(gPlayers[i]->lastCmd & CMD_BUTTON1))
		{
			return 1;
		}
	}
	return 0;
}

void CommandBadGuys(int ticks)
{
	TActor *actor;
	int roll, cmd;
	int count = 0;
	int bypass;
	int delayModifier;
	int rollLimit;

	switch (gConfig.Game.Difficulty)
	{
	case DIFFICULTY_VERYEASY:
		delayModifier = 4;
		rollLimit = 300;
		break;
	case DIFFICULTY_EASY:
		delayModifier = 2;
		rollLimit = 200;
		break;
	case DIFFICULTY_HARD:
		delayModifier = 1;
		rollLimit = 75;
		break;
	case DIFFICULTY_VERYHARD:
		delayModifier = 1;
		rollLimit = 50;
		break;
	default:
		delayModifier = 1;
		rollLimit = 100;
		break;
	}

	actor = ActorList();
	while (actor != NULL)
	{
		if (!(actor->pData || (actor->flags & FLAGS_PRISONER)))
		{
			if ((actor->flags & (FLAGS_VICTIM | FLAGS_GOOD_GUY)) != 0)
			{
				gAreGoodGuysPresent = 1;
			}

			count++;
			cmd = 0;
			if (!actor->dead &&
			    (actor->flags & FLAGS_SLEEPING) == 0) {
				bypass = 0;
				roll = rand() % rollLimit;
				if (!!(actor->flags & FLAGS_FOLLOWER))
				{
					if (IsCloseToPlayer(Vec2iNew(actor->x, actor->y)))
					{
						cmd = 0;
					}
					else
					{
						cmd = Follow(actor);
					}
					actor->delay = actor->character->bot.actionDelay;
				}
				else if (!!(actor->flags & FLAGS_SNEAKY) &&
					!!(actor->flags & FLAGS_VISIBLE) &&
					DidPlayerShoot())
				{
					cmd = Hunt(actor) | CMD_BUTTON1;
					bypass = 1;
				}
				else if (actor->flags & FLAGS_DETOURING)
				{
					cmd = BrightWalk(actor, roll);
				}
				else if (actor->delay > 0)
				{
					actor->delay = MAX(0, actor->delay - ticks);
					cmd = actor->lastCmd & ~CMD_BUTTON1;
				}
				else
				{
					if (roll < actor->character->bot.probabilityToTrack)
					{
						cmd = Hunt(actor);
					}
					else if (roll < actor->character->bot.probabilityToMove)
					{
						cmd = DirectionToCmd(rand() & 7);
					}
					else
					{
						cmd = 0;
					}
					actor->delay = actor->character->bot.actionDelay * delayModifier;
				}
				if (!bypass)
				{
					if (WillFire(actor, roll))
					{
						cmd |= CMD_BUTTON1;
						if (!!(actor->flags & FLAGS_FOLLOWER) &&
							(actor->flags & FLAGS_GOOD_GUY))
						{
							// Shoot in a random direction away
							int i;
							for (i = 0; i < 10; i++)
							{
								direction_e d =
									(direction_e)(rand() % DIRECTION_COUNT);
								if (!IsFacingPlayer(actor, d))
								{
									cmd = DirectionToCmd(d) | CMD_BUTTON1;
									break;
								}
							}
						}
						if (actor->flags & FLAGS_RUNS_AWAY)
						{
							// Turn back and shoot for running away characters
							cmd |= ReverseDirection(Hunt(actor));
						}
					}
					else
					{
						if ((actor->flags & FLAGS_VISIBLE) == 0)
						{
							// I think this is some hack to make sure invisible enemies don't fire so much
							actor->weapon.lock = 40;
						}
						if (cmd && !DirectionOK(actor, CmdToDirection(cmd)) &&
							(actor->flags & FLAGS_DETOURING) == 0)
						{
							Detour(actor);
							cmd = 0;
						}
					}
				}
			}
			CommandActor(actor, cmd, ticks);
			actor->flags &= ~FLAGS_VISIBLE;
		}
		else if ((actor->flags & FLAGS_PRISONER) != 0)
		{
			CommandActor(actor, 0, ticks);
		}

		actor = actor->next;
	}
	if (gMission.missionData->baddieCount > 0 &&
		gMission.missionData->baddieDensity > 0 &&
		count < MAX(1, (gMission.missionData->baddieDensity * gConfig.Game.EnemyDensity) / 100))
	{
		Character *character = CharacterStoreGetRandomBaddie(
			&gCampaign.Setting.characters);
		TActor *baddie = AddActor(character, NULL);
		PlaceBaddie(baddie);
		gBaddieCount++;
	}
}

void InitializeBadGuys(void)
{
	int i, j;
	TActor *actor;

	if (gMission.missionData->specialCount > 0)
	{
		for (i = 0; i < gMission.missionData->objectiveCount; i++)
		{
			if (gMission.missionData->objectives[i].type == OBJECTIVE_KILL)
			{
				for (j = 0; j < gMission.objectives[i].count; j++)
				{
					actor = AddActor(CharacterStoreGetRandomSpecial(
						&gCampaign.Setting.characters), NULL);
					actor->tileItem.flags |= ObjectiveToTileItem(i);
					PlaceBaddie(actor);
				}
			}
		}
	}

	for (i = 0; i < gMission.missionData->objectiveCount; i++)
	{
		if (gMission.missionData->objectives[i].type == OBJECTIVE_RESCUE)
		{
			for (j = 0; j < gMission.objectives[i].count; j++)
			{
				actor = AddActor(CharacterStoreGetPrisoner(
					&gCampaign.Setting.characters, 0), NULL);
				actor->tileItem.flags |= ObjectiveToTileItem(i);
				if (HasLockedRooms())
				{
					PlacePrisoner(actor);
				}
				else
				{
					PlaceBaddie(actor);
				}
			}
		}
	}

	gBaddieCount = gMission.index * 4;
	gAreGoodGuysPresent = 0;
}

void CreateEnemies(void)
{
	int i;
	if (gMission.missionData->baddieCount <= 0)
		return;

	for (i = 0;
		i < MAX(1, (gMission.missionData->baddieDensity * gConfig.Game.EnemyDensity) / 100);
		i++)
	{
		TActor *enemy = AddActor(CharacterStoreGetRandomBaddie(
			&gCampaign.Setting.characters), NULL);
		PlaceBaddie(enemy);
		gBaddieCount++;
	}
}
