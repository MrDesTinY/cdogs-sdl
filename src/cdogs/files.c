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
#include "files.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <SDL.h>

#include "sys_specifics.h"
#include "utils.h"

#define MAX_STRING_LEN 1000

#define CAMPAIGN_MAGIC    690304
#define CAMPAIGN_VERSION  6

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
static const int bendian = 1;
#else
static const int bendian = 0;
#endif

void swap32 (void *d)
{
	if (bendian)
	{
		*((int32_t *)d) = SDL_Swap32(*((int32_t *)d));
	}
}

size_t f_read(FILE *f, void *buf, size_t size)
{
	return fread(buf, size, 1, f);
}

size_t f_read32(FILE *f, void *buf, size_t size)
{
	size_t ret = 0;
	if (buf) {
		ret = f_read(f, buf, size);
		swap32((int *)buf);
	}
	return ret;
}

void swap16 (void *d)
{
	if (bendian)
	{
		*((int16_t *)d) = SDL_Swap16(*((int16_t *)d));
	}
}

size_t f_read16(FILE *f, void *buf, size_t size)
{
	size_t ret = 0;
	if (buf)
	{
		ret = f_read(f, buf, size);
		swap16((int16_t *)buf);
	}
	return ret;
}

int fwrite32(FILE *f, void *buf)
{
	size_t ret = fwrite(buf, 4, 1, f);
	if (ret != 1)
	{
		return 0;
	}
	return 1;
}


int ScanCampaign(const char *filename, char *title, int *missions)
{
	FILE *f;
	int i;
	CampaignSetting setting;

	debug(D_NORMAL, "filename: %s\n", filename);

	f = fopen(filename, "rb");
	if (f != NULL)
	{
		f_read32(f, &i, sizeof(i));

		if (i != CAMPAIGN_MAGIC) {
			fclose(f);
			debug(D_NORMAL, "Filename: %s\n", filename);
			debug(D_NORMAL, "Magic: %d FileM: %d\n", CAMPAIGN_MAGIC, i);
			debug(D_NORMAL, "ScanCampaign - bad file!\n");
			return CAMPAIGN_BADFILE;
		}

		f_read32(f, &i, sizeof(i));
		if (i != CAMPAIGN_VERSION) {
			fclose(f);
			debug(
				D_NORMAL,
				"ScanCampaign - version mismatch (expected %d, read %d)\n",
				CAMPAIGN_VERSION, i);
			return CAMPAIGN_VERSIONMISMATCH;
		}

		f_read(f, setting.title, sizeof(setting.title));
		f_read(f, setting.author, sizeof(setting.author));
		f_read(f, setting.description, sizeof(setting.description));
		f_read32(f, &setting.missionCount, sizeof(setting.missionCount));
		strcpy(title, setting.title);
		*missions = setting.missionCount;

		fclose(f);

		return CAMPAIGN_OK;
	}
	perror("ScanCampaign - couldn't read file:");
	return CAMPAIGN_BADPATH;
}

void load_mission_objective(FILE *f, struct MissionObjective *o)
{
		int offset = 0;
		f_read(f, o->description, sizeof(o->description)); offset += sizeof(o->description);
		f_read32(f, &o->type, sizeof(o->type));
		f_read32(f, &o->index, sizeof(o->index));
		f_read32(f, &o->count, sizeof(o->count));
		f_read32(f, &o->required, sizeof(o->required));
		f_read32(f, &o->flags, sizeof(o->flags));
		debug(D_VERBOSE, " >> Objective: %s data: %d %d %d %d %d\n",
		o->description, o->type, o->index, o->count, o->required, o->flags);
}

#define R32(s,e)	f_read32(f, &(s)->e, sizeof((s)->e))

void load_mission(FILE *f, struct Mission *m)
{
	int i;

	f_read(f, m->title, sizeof(m->title));
	f_read(f, m->description, sizeof(m->description));

	debug(D_NORMAL, "== MISSION ==\n");
	debug(D_NORMAL, "t: %s\n", m->title);
	debug(D_NORMAL, "d: %s\n", m->description);

	R32(m,  wallStyle);
	R32(m,  floorStyle);
	R32(m,  roomStyle);
	R32(m,  exitStyle);
	R32(m,  keyStyle);
	R32(m,  doorStyle);

	R32(m,  mapWidth); R32(m, mapHeight);
	R32(m,  wallCount); R32(m, wallLength);
	R32(m,  roomCount);
	R32(m,  squareCount);

	R32(m,  exitLeft); R32(m, exitTop); R32(m, exitRight); R32(m, exitBottom);

	R32(m, objectiveCount);

	debug(D_NORMAL, "number of objectives: %d\n", m->objectiveCount);
 	for (i = 0; i < OBJECTIVE_MAX; i++) {
		load_mission_objective(f, &m->objectives[i]);
	}

	R32(m, baddieCount);
	for (i = 0; i < BADDIE_MAX; i++) {
		f_read32(f, &m->baddies[i], sizeof(int));
	}

	R32(m, specialCount);
	for (i = 0; i < SPECIAL_MAX; i++) {
		f_read32(f, &m->specials[i], sizeof(int));
	}

	R32(m, itemCount);
	for (i = 0; i < ITEMS_MAX; i++) {
		f_read32(f, &m->items[i], sizeof(int));
	}
	for (i = 0; i < ITEMS_MAX; i++) {
		f_read32(f, &m->itemDensity[i], sizeof(int));
	}

	R32(m, baddieDensity);
	R32(m, weaponSelection);

	f_read(f, m->song, sizeof(m->song));
	f_read(f, m->map, sizeof(m->map));

	R32(m, wallRange);
	R32(m, floorRange);
	R32(m, roomRange);
	R32(m, altRange);

	debug(D_VERBOSE, "number of baddies: %d\n", m->baddieCount);
}



void load_character(FILE *f, TBadGuy *b)
{
	R32(b, armedBodyPic);
	R32(b, unarmedBodyPic);
	R32(b, facePic);
	R32(b, speed);
	R32(b, probabilityToMove);
	R32(b, probabilityToTrack);
	R32(b, probabilityToShoot);
	R32(b, actionDelay);
	R32(b, gun);
	R32(b, skinColor);
	R32(b, armColor);
	R32(b, bodyColor);
	R32(b, legColor);
	R32(b, hairColor);
	R32(b, health);
	R32(b, flags);
}
void ConvertCharacter(Character *c, TBadGuy *b)
{
	c->looks.armedBody = b->armedBodyPic;
	c->looks.unarmedBody = b->unarmedBodyPic;
	c->looks.face = b->facePic;
	c->speed = b->speed;
	c->bot.probabilityToMove = b->probabilityToMove;
	c->bot.probabilityToTrack = b->probabilityToTrack;
	c->bot.probabilityToShoot = b->probabilityToShoot;
	c->bot.actionDelay = b->actionDelay;
	c->gun = (gun_e)b->gun;
	c->looks.skin = b->skinColor;
	c->looks.arm = b->armColor;
	c->looks.body = b->bodyColor;
	c->looks.leg = b->legColor;
	c->looks.hair = b->hairColor;
	c->maxHealth = b->health;
	c->flags = b->flags;
}
TBadGuy ConvertTBadGuy(Character *e)
{
	TBadGuy b;
	b.armedBodyPic = e->looks.armedBody;
	b.unarmedBodyPic = e->looks.unarmedBody;
	b.facePic = e->looks.face;
	b.speed = e->speed;
	b.probabilityToMove = e->bot.probabilityToMove;
	b.probabilityToTrack = e->bot.probabilityToTrack;
	b.probabilityToShoot = e->bot.probabilityToShoot;
	b.actionDelay = e->bot.actionDelay;
	b.gun = e->gun;
	b.skinColor = e->looks.skin;
	b.armColor = e->looks.arm;
	b.bodyColor = e->looks.body;
	b.legColor = e->looks.leg;
	b.hairColor = e->looks.hair;
	b.health = e->maxHealth;
	b.flags = e->flags;
	return b;
}

void ConvertCampaignSetting(CampaignSettingNew *dest, CampaignSetting *src)
{
	int i;
	strcpy(dest->title, src->title);
	strcpy(dest->author, src->author);
	strcpy(dest->description, src->description);
	dest->missionCount = src->missionCount;
	CFREE(dest->missions);
	CMALLOC(dest->missions, sizeof *dest->missions * dest->missionCount);
	for (i = 0; i < dest->missionCount; i++)
	{
		memcpy(&dest->missions[i], &src->missions[i], sizeof dest->missions[i]);
	}
	CharacterStoreTerminate(&dest->characters);
	CharacterStoreInit(&dest->characters);
	for (i = 0; i < src->characterCount; i++)
	{
		Character *ch = CharacterStoreAddOther(&dest->characters);
		ConvertCharacter(ch, &src->characters[i]);
		CharacterSetLooks(ch, &ch->looks);
	}
}

int LoadCampaign(const char *filename, CampaignSettingNew *setting)
{
	FILE *f = NULL;
	int32_t i;
	int err = CAMPAIGN_OK;
	int numMissions;
	int numCharacters;

	debug(D_NORMAL, "f: %s\n", filename);
	f = fopen(filename, "rb");
	if (f == NULL)
	{
		err = CAMPAIGN_BADPATH;
		goto bail;
	}

	f_read32(f, &i, sizeof(i));
	if (i != CAMPAIGN_MAGIC)
	{
		debug(D_NORMAL, "LoadCampaign - bad file!\n");
		err = CAMPAIGN_BADFILE;
		goto bail;
	}

	f_read32(f, &i, sizeof(i));
	if (i != CAMPAIGN_VERSION)
	{
		debug(D_NORMAL, "LoadCampaign - version mismatch!\n");
		err = CAMPAIGN_VERSIONMISMATCH;
		goto bail;
	}

	f_read(f, setting->title, sizeof(setting->title));
	f_read(f, setting->author, sizeof(setting->author));
	f_read(f, setting->description, sizeof(setting->description));

	f_read32(f, &setting->missionCount, sizeof(int32_t));
	CCALLOC(
		setting->missions,
		setting->missionCount * sizeof *setting->missions);
	numMissions = setting->missionCount;
	debug(D_NORMAL, "No. missions: %d\n", numMissions);
	for (i = 0; i < numMissions; i++)
	{
		load_mission(f, &setting->missions[i]);
	}

	f_read32(f, &numCharacters, sizeof(int32_t));
	debug(D_NORMAL, "No. characters: %d\n", numCharacters);
	for (i = 0; i < numCharacters; i++)
	{
		TBadGuy b;
		Character *ch;
		load_character(f, &b);
		ch = CharacterStoreAddOther(&setting->characters);
		ConvertCharacter(ch, &b);
		CharacterSetLooks(ch, &ch->looks);
	}

bail:
	if (f != NULL)
	{
		fclose(f);
	}
	return err;
}

int SaveCampaign(const char *filename, CampaignSettingNew *setting)
{
	FILE *f;
	int32_t i;
	char buf[CDOGS_FILENAME_MAX];

	if (SDL_strcasecmp(StrGetFileExt(filename), "cpn") == 0)
	{
		strcpy(buf, filename);
	}
	else
	{
		sprintf(buf, "%s.cpn", filename);
	}
	f = fopen(buf, "wb");
	if (f == NULL)
	{
		perror("SaveCampaign - couldn't write to file: ");
		return CAMPAIGN_BADFILE;
	}
#define CHECK_WRITE(res)\
	if (!(res))\
	{\
		perror("SaveCampaign - couldn't write to file: ");\
		fclose(f);\
		return CAMPAIGN_BADFILE;\
	}
	i = CAMPAIGN_MAGIC;
	CHECK_WRITE(fwrite32(f, &i))

	i = CAMPAIGN_VERSION;
	CHECK_WRITE(fwrite32(f, &i))

	CHECK_WRITE(fwrite(setting->title, sizeof setting->title, 1, f) == 1)
	CHECK_WRITE(fwrite(setting->author, sizeof setting->author, 1, f) == 1)
	CHECK_WRITE(fwrite(setting->description, sizeof setting->description, 1, f) == 1)

	i = setting->missionCount;
	CHECK_WRITE(fwrite32(f, &i))
	for (i = 0; i < setting->missionCount; i++)
	{
		CHECK_WRITE(fwrite(&setting->missions[i], sizeof(struct Mission), 1, f) == 1)
	}

	i = setting->characters.otherCount;
	CHECK_WRITE(fwrite32(f, &i))
	for (i = 0; i < setting->characters.otherCount; i++)
	{
		TBadGuy b = ConvertTBadGuy(&setting->characters.others[i]);
		CHECK_WRITE(fwrite(&b, sizeof(TBadGuy), 1, f) == 1)
	}

#undef CHECK_WRITE
	printf("Saved to %s\n", filename);
	fclose(f);
	return CAMPAIGN_OK;
}

static void OutputCString(FILE * f, const char *s, int indentLevel)
{
	int length;

	for (length = 0; length < indentLevel; length++)
		fputc(' ', f);

	fputc('\"', f);
	while (*s) {
		switch (*s) {
		case '\"':
		case '\\':
			fputc('\\', f);
		default:
			fputc(*s, f);
		}
		s++;
		length++;
		if (length > 75) {
			fputs("\"\n", f);
			length = 0;
			for (length = 0; length < indentLevel; length++)
				fputc(' ', f);
			fputc('\"', f);
		}
	}
	fputc('\"', f);
}

void SaveCampaignAsC(
	const char *filename, const char *name,
	CampaignSettingNew* setting)
{
	FILE *f;
	int i, j;

	f = fopen(filename, "w");
	if (!f)
	{
		printf("Failed to save to C\n");
		return;
	}
	fprintf(f, "TBadGuy %s_badguys[ %d] =\n{\n", name,
		setting->characters.otherCount);
	for (i = 0; i < setting->characters.otherCount; i++)
	{
		TBadGuy b = ConvertTBadGuy(&setting->characters.others[i]);
		fprintf(f,
			"  {%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,0x%x}%s\n",
			b.armedBodyPic,
			b.unarmedBodyPic,
			b.facePic,
			b.speed,
			b.probabilityToMove,
			b.probabilityToTrack,
			b.probabilityToShoot,
			b.actionDelay,
			b.gun,
			b.skinColor,
			b.armColor,
			b.bodyColor,
			b.legColor,
			b.hairColor,
			b.health,
			b.flags,
			i < setting->characters.otherCount - 1 ? "," : "");
	}
	fprintf(f, "};\n\n");

	fprintf(f, "struct Mission %s_missions[ %d] =\n{\n", name,
		setting->missionCount);
	for (i = 0; i < setting->missionCount; i++) {
		fprintf(f, "  {\n");
		OutputCString(f, setting->missions[i].title, 4);
		fprintf(f, ",\n");
		OutputCString(f, setting->missions[i].description,
				    4);
		fprintf(f, ",\n");
		fprintf(f,
			"    %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,\n",
			setting->missions[i].wallStyle,
			setting->missions[i].floorStyle,
			setting->missions[i].roomStyle,
			setting->missions[i].exitStyle,
			setting->missions[i].keyStyle,
			setting->missions[i].doorStyle,
			setting->missions[i].mapWidth,
			setting->missions[i].mapHeight,
			setting->missions[i].wallCount,
			setting->missions[i].wallLength,
			setting->missions[i].roomCount,
			setting->missions[i].squareCount);
		fprintf(f, "    %d,%d,%d,%d,\n",
			setting->missions[i].exitLeft,
			setting->missions[i].exitTop,
			setting->missions[i].exitRight,
			setting->missions[i].exitBottom);
		fprintf(f, "    %d,\n",
			setting->missions[i].objectiveCount);
		fprintf(f, "    {\n");
		for (j = 0; j < OBJECTIVE_MAX; j++) {
			fprintf(f, "      {\n");
			OutputCString(f,
					    setting->missions[i].
					    objectives[j].description,
					    8);
			fprintf(f, ",\n");
			fprintf(f, "        %d,%d,%d,%d,0x%x\n",
				setting->missions[i].objectives[j].
				type,
				setting->missions[i].objectives[j].
				index,
				setting->missions[i].objectives[j].
				count,
				setting->missions[i].objectives[j].
				required,
				setting->missions[i].objectives[j].
				flags);
			fprintf(f, "      }%s\n",
				j < OBJECTIVE_MAX - 1 ? "," : "");
		}
		fprintf(f, "    },\n");

		fprintf(f, "    %d,\n",
			setting->missions[i].baddieCount);
		fprintf(f, "    {");
		for (j = 0; j < BADDIE_MAX; j++) {
			fprintf(f, "%d%s",
				setting->missions[i].baddies[j],
				j < BADDIE_MAX - 1 ? "," : "");
		}
		fprintf(f, "},\n");

		fprintf(f, "    %d,\n",
			setting->missions[i].specialCount);
		fprintf(f, "    {");
		for (j = 0; j < SPECIAL_MAX; j++) {
			fprintf(f, "%d%s",
				setting->missions[i].specials[j],
				j < SPECIAL_MAX - 1 ? "," : "");
		}
		fprintf(f, "},\n");

		fprintf(f, "    %d,\n",
			setting->missions[i].itemCount);
		fprintf(f, "    {");
		for (j = 0; j < ITEMS_MAX; j++) {
			fprintf(f, "%d%s",
				setting->missions[i].items[j],
				j < ITEMS_MAX - 1 ? "," : "");
		}
		fprintf(f, "},\n");

		fprintf(f, "    {");
		for (j = 0; j < ITEMS_MAX; j++) {
			fprintf(f, "%d%s",
				setting->missions[i].
				itemDensity[j],
				j < ITEMS_MAX - 1 ? "," : "");
		}
		fprintf(f, "},\n");

		fprintf(f, "    %d,0x%x,\n",
			setting->missions[i].baddieDensity,
			setting->missions[i].weaponSelection);
		OutputCString(f, setting->missions[i].song, 4);
		fprintf(f, ",\n");
		OutputCString(f, setting->missions[i].map, 4);
		fprintf(f, ",\n");
		fprintf(f, "    %d,%d,%d,%d\n",
			setting->missions[i].wallRange,
			setting->missions[i].floorRange,
			setting->missions[i].roomRange,
			setting->missions[i].altRange);
		fprintf(f, "  }%s\n",
			i < setting->missionCount ? "," : "");
	}
	fprintf(f, "};\n\n");

	fprintf(f, "struct CampaignSetting %s_campaign =\n{\n",
		name);
	OutputCString(f, setting->title, 2);
	fprintf(f, ",\n");
	OutputCString(f, setting->author, 2);
	fprintf(f, ",\n");
	OutputCString(f, setting->description, 2);
	fprintf(f, ",\n");
	fprintf(f, "  %d, %s_missions, %d, %s_badguys\n};\n",
		setting->missionCount, name,
		setting->characters.otherCount, name);

	printf("Saved to %s\n", filename);
	fclose(f);
};

/* GetHomeDirectory ()
 *
 * Uses environment variables to determine the users home directory.
 * returns some sort of path (C string)
 *
 * It's an ugly piece of sh*t... :/
 */
char *cdogs_homepath = NULL;
const char *GetHomeDirectory(void)
{
	const char *p;

	if (cdogs_homepath != NULL)
	{
		return cdogs_homepath;
	}

	p = getenv("CDOGS_CONFIG_DIR");
	if (p != NULL && strlen(p) != 0)
	{
		cdogs_homepath = strdup(p);
		return cdogs_homepath;
	}

	p = getenv(HOME_DIR_ENV);
	if (p != NULL && strlen(p) != 0)
	{
		cdogs_homepath = calloc(strlen(p) + 1, sizeof(char));
		strncpy(cdogs_homepath, p, strlen(p));
		strncat(cdogs_homepath, "/", 1);
		return cdogs_homepath;
	}

	fprintf(stderr,"%s%s%s%s",
	"##############################################################\n",
	"# You don't have the environment variables HOME or USER set. #\n",
	"# It is suggested you get a better shell. :D                 #\n",
	"##############################################################\n");
	return "";
}


char *GetDataFilePath(const char *path)
{
	static char buf[CDOGS_PATH_MAX];
	strcpy(buf, CDOGS_DATA_DIR);
	strcat(buf, path);
	return buf;
}


/* GetConfigFilePath()
 *
 * returns a full path to a data file...
 */
char cfpath[512];
const char *GetConfigFilePath(const char *name)
{
	const char *homedir = GetHomeDirectory();

	strcpy(cfpath, homedir);

	strcat(cfpath, CDOGS_CFG_DIR);
	strcat(cfpath, name);

	return cfpath;
}

int mkdir_deep(const char *path)
{
	int i;
	char part[255];

	debug(D_NORMAL, "mkdir_deep path: %s\n", path);

	for (i = 0; i < (int)strlen(path); i++)
	{
		if (path[i] == '\0') break;
		if (path[i] == '/') {
			strncpy(part, path, i + 1);
			part[i+1] = '\0';

			if (mkdir(part, MKDIR_MODE) == -1)
			{
				/* Mac OS X 10.4 returns EISDIR instead of EEXIST
				 * if a dir already exists... */
				if (errno == EEXIST || errno == EISDIR) continue;
				else return 1;
			}
		}
	}

	return 0;
}

void SetupConfigDir(void)
{
	const char *cfg_p = GetConfigFilePath("");

	printf("Creating Config dir... ");

	if (mkdir_deep(cfg_p) == 0)
	{
		if (errno != EEXIST)
			printf("Config dir created.\n");
		else
			printf("No need. Already exists!\n");
	} else {
		switch (errno) {
			case EACCES:
				printf("Permission denied!\n");
				break;
			default:
				perror("Error creating config directory:");
		}
	}

	return;
}

char dir_buf[512];
char * GetPWD(void)
{
	if (getcwd(dir_buf, 511) == NULL) {
		printf("Error getting PWD\n");
	}
	return dir_buf;
}
