/*
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
#include "mainmenu.h"

#include <stdio.h>
#include <string.h>

#include <cdogs/config.h>
#include <cdogs/text.h>

#include "autosave.h"
#include "menu.h"


MenuSystem *MenuCreateAll(
	custom_campaigns_t *campaigns,
	InputDevices *input,
	GraphicsDevice *graphics);

int MainMenu(
	GraphicsDevice *graphics,
	credits_displayer_t *creditsDisplayer,
	custom_campaigns_t *campaigns)
{
	int doPlay = 0;
	MenuSystem *menu = MenuCreateAll(campaigns, &gInputDevices, graphics);
	MenuSetCreditsDisplayer(menu, creditsDisplayer);
	MenuLoop(menu);
	doPlay = menu->current->type == MENU_TYPE_CAMPAIGN_ITEM;

	MenuDestroy(menu);
	return doPlay;
}

menu_t *MenuCreateContinue(const char *name, campaign_entry_t *entry);
menu_t *MenuCreateQuickPlay(const char *name, campaign_entry_t *entry);
menu_t *MenuCreateCampaigns(
	const char *name,
	const char *title,
	campaign_list_t *list);
menu_t *MenuCreateOptions(const char *name);
menu_t *MenuCreateQuit(const char *name);

MenuSystem *MenuCreateAll(
	custom_campaigns_t *campaigns,
	InputDevices *input,
	GraphicsDevice *graphics)
{
	MenuSystem *ms;
	int menuContinueIndex;
	CCALLOC(ms, sizeof *ms);
	MenuSystemInit(
		ms,
		input, graphics,
		Vec2iZero(),
		Vec2iNew(
			graphics->cachedConfig.ResolutionWidth,
			graphics->cachedConfig.ResolutionHeight));
	ms->root = ms->current = MenuCreateNormal(
		"",
		"",
		MENU_TYPE_NORMAL,
		MENU_DISPLAY_ITEMS_CREDITS | MENU_DISPLAY_ITEMS_AUTHORS);
	MenuAddSubmenu(
		ms->root,
		MenuCreateContinue("Continue", &gAutosave.LastMission.Campaign));
	menuContinueIndex = ms->root->u.normal.numSubMenus - 1;
	MenuAddSubmenu(
		ms->root,
		MenuCreateQuickPlay("Quick Play", &campaigns->quickPlayEntry));
	MenuAddSubmenu(
		ms->root,
		MenuCreateCampaigns(
			"Campaign",
			"Select a campaign:",
			&campaigns->campaignList));
	MenuAddSubmenu(
		ms->root,
		MenuCreateCampaigns(
			"Dogfight",
			"Select a dogfight scenario:",
			&campaigns->dogfightList));
	MenuAddSubmenu(ms->root, MenuCreateOptions("Options..."));
	MenuAddSubmenu(ms->root, MenuCreateQuit("Quit"));
	MenuAddExitType(ms, MENU_TYPE_QUIT);
	MenuAddExitType(ms, MENU_TYPE_CAMPAIGN_ITEM);

	if (strlen(gAutosave.LastMission.Password) == 0 ||
		!gAutosave.LastMission.IsValid)
	{
		MenuDisableSubmenu(ms->root, menuContinueIndex);
	}

	return ms;
}


menu_t *MenuCreateContinue(const char *name, campaign_entry_t *entry)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_CAMPAIGN_ITEM);
	menu->u.campaign = *entry;
	return menu;
}

menu_t *MenuCreateQuickPlay(const char *name, campaign_entry_t *entry)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_CAMPAIGN_ITEM);
	memcpy(&menu->u.campaign, entry, sizeof(menu->u.campaign));
	return menu;
}

menu_t *MenuCreateCampaignItem(campaign_entry_t *entry);

menu_t *MenuCreateCampaigns(
	const char *name,
	const char *title,
	campaign_list_t *list)
{
	menu_t *menu = MenuCreateNormal(
		name,
		title,
		MENU_TYPE_CAMPAIGNS,
		0);
	int i;
	for (i = 0; i < list->numSubFolders; i++)
	{
		char folderName[CDOGS_FILENAME_MAX];
		sprintf(folderName, "%s/", list->subFolders[i].name);
		MenuAddSubmenu(
			menu,
			MenuCreateCampaigns(
				folderName,
				title,
				&list->subFolders[i]));
	}
	for (i = 0; i < list->num; i++)
	{
		MenuAddSubmenu(menu, MenuCreateCampaignItem(&list->list[i]));
	}
	return menu;
}

menu_t *MenuCreateCampaignItem(campaign_entry_t *entry)
{
	menu_t *menu = MenuCreate(entry->info, MENU_TYPE_CAMPAIGN_ITEM);
	memcpy(&menu->u.campaign, entry, sizeof(menu->u.campaign));
	return menu;
}

menu_t *MenuCreateOptionsGame(const char *name);
menu_t *MenuCreateOptionsGraphics(const char *name);
menu_t *MenuCreateOptionsControls(const char *name);
menu_t *MenuCreateOptionsSound(const char *name);
menu_t *MenuCreateOptionsQuickPlay(const char *name);

menu_t *MenuCreateOptions(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Options:",
		MENU_TYPE_NORMAL,
		0);
	MenuAddSubmenu(menu, MenuCreateOptionsGame("Game..."));
	MenuAddSubmenu(menu, MenuCreateOptionsGraphics("Graphics..."));
	MenuAddSubmenu(menu, MenuCreateOptionsControls("Controls..."));
	MenuAddSubmenu(menu, MenuCreateOptionsSound("Sound..."));
	MenuAddSubmenu(menu, MenuCreateOptionsQuickPlay("Quick Play..."));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));
	return menu;
}

menu_t *MenuCreateOptionsGame(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Game Options:",
		MENU_TYPE_OPTIONS,
		0);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Friendly fire",
			&gConfig.Game.FriendlyFire,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"FPS monitor",
			&gConfig.Interface.ShowFPS,
			MENU_OPTION_DISPLAY_STYLE_ON_OFF));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Display time",
			&gConfig.Interface.ShowTime,
			MENU_OPTION_DISPLAY_STYLE_ON_OFF));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Show HUD map",
			&gConfig.Interface.ShowHUDMap,
			MENU_OPTION_DISPLAY_STYLE_ON_OFF));
	MenuAddSubmenu(
		menu, MenuCreateOptionSeed("Random seed", &gConfig.Game.RandomSeed));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Difficulty", (int *)&gConfig.Game.Difficulty,
			DIFFICULTY_VERYEASY, DIFFICULTY_VERYHARD, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))DifficultyStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Slowmotion",
			&gConfig.Game.SlowMotion,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Enemy density",
			&gConfig.Game.EnemyDensity,
			25, 200, 25,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))PercentStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Non-player HP",
			&gConfig.Game.NonPlayerHP,
			25, 200, 25,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))PercentStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Player HP",
			&gConfig.Game.PlayerHP,
			25, 200, 25,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))PercentStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Fog",
			&gConfig.Game.Fog,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Sight range",
			&gConfig.Game.SightRange,
			8, 40, 1,
			MENU_OPTION_DISPLAY_STYLE_INT, NULL));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Shadows",
			&gConfig.Game.Shadows,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Move when shooting",
			&gConfig.Game.MoveWhenShooting,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Switch move style", (int *)&gConfig.Game.SwitchMoveStyle,
			SWITCHMOVE_SLIDE, SWITCHMOVE_NONE, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))SwitchMoveStyleStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Shots push back",
			&gConfig.Game.ShotsPushback,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Ally collision", (int *)&gConfig.Game.AllyCollision,
			ALLYCOLLISION_NORMAL, ALLYCOLLISION_NONE, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))AllyCollisionStr));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateOptionsGraphics(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Graphics Options:",
		MENU_TYPE_OPTIONS,
		0);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Brightness", (int *)&gConfig.Graphics.Brightness,
			BLIT_BRIGHTNESS_MIN, BLIT_BRIGHTNESS_MAX, 1,
			MENU_OPTION_DISPLAY_STYLE_INT, NULL));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Splitscreen", (int *)&gConfig.Interface.Splitscreen,
			SPLITSCREEN_NORMAL, SPLITSCREEN_NEVER, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))SplitscreenStyleStr));
#ifndef __ANDROID__
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Fullscreen",
			&gConfig.Graphics.Fullscreen,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
#endif
	MenuAddSubmenu(
		menu,
		MenuCreateOptionUpDownFunc(
			"Video mode",
			Gfx_ModePrev,
			Gfx_ModeNext,
			MENU_OPTION_DISPLAY_STYLE_STR_FUNC,
			GrafxGetModeStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Scale mode", (int *)&gConfig.Graphics.ScaleMode,
			SCALE_MODE_NN, SCALE_MODE_HQX, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))ScaleModeStr));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateKeys(const char *name);

menu_t *MenuCreateOptionsControls(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Configure Controls:",
		MENU_TYPE_OPTIONS,
		0);
#ifndef __ANDROID__
	MenuAddSubmenu(menu, MenuCreateKeys("Redefine keys..."));
#endif
	MenuAddSubmenu(
		menu,
		MenuCreateOptionFunc(
			"Reset joysticks",
			GJoyReset,
			NULL, MENU_OPTION_DISPLAY_STYLE_NONE));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateOptionsSound(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Configure Sound:",
		MENU_TYPE_OPTIONS,
		0);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Sound effects",
			&gConfig.Sound.SoundVolume,
			8, 64, 8,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))Div8Str));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Music",
			&gConfig.Sound.MusicVolume,
			0, 64, 8,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))Div8Str));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"FX channels",
			&gConfig.Sound.SoundChannels,
			2, 8, 1,
			MENU_OPTION_DISPLAY_STYLE_INT, NULL));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateOptionsQuickPlay(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Quick Play Options:",
		MENU_TYPE_OPTIONS,
		0);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Map size",
			(int *)&gConfig.QuickPlay.MapSize,
			QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))QuickPlayQuantityStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Wall count",
			(int *)&gConfig.QuickPlay.WallCount,
			QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))QuickPlayQuantityStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Wall length",
			(int *)&gConfig.QuickPlay.WallLength,
			QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))QuickPlayQuantityStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Room count",
			(int *)&gConfig.QuickPlay.RoomCount,
			QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))QuickPlayQuantityStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Square count",
			(int *)&gConfig.QuickPlay.SquareCount,
			QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))QuickPlayQuantityStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Enemy count",
			(int *)&gConfig.QuickPlay.EnemyCount,
			QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))QuickPlayQuantityStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Enemy speed",
			(int *)&gConfig.QuickPlay.EnemySpeed,
			QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))QuickPlayQuantityStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Enemy health",
			(int *)&gConfig.QuickPlay.EnemyHealth,
			QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))QuickPlayQuantityStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Enemies with explosives",
			&gConfig.QuickPlay.EnemiesWithExplosives,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Item count",
			(int *)&gConfig.QuickPlay.ItemCount,
			QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))QuickPlayQuantityStr));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateQuit(const char *name)
{
	return MenuCreate(name, MENU_TYPE_QUIT);
}


menu_t *MenuCreateOptionToggle(
	const char *name, int *config, menu_option_display_style_e style)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_TOGGLE);
	menu->u.option.uHook.optionToggle = config;
	menu->u.option.displayStyle = style;
	return menu;
}

void MenuCreateKeysSingleSection(
	menu_t *menu, const char *sectionName,
	input_keys_t *keys, input_keys_t *keysOther);
menu_t *MenuCreateOptionChangeKey(
	const char *name, key_code_e code,
	input_keys_t *keys, input_keys_t *keysOther);

menu_t *MenuCreateKeys(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"",
		MENU_TYPE_KEYS,
		0);
	MenuCreateKeysSingleSection(
		menu, "Keyboard 1",
		&gConfig.Input.PlayerKeys[0].Keys, &gConfig.Input.PlayerKeys[1].Keys);
	MenuCreateKeysSingleSection(
		menu, "Keyboard 2",
		&gConfig.Input.PlayerKeys[1].Keys, &gConfig.Input.PlayerKeys[0].Keys);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Map", KEY_CODE_MAP,
			&gConfig.Input.PlayerKeys[0].Keys, &gConfig.Input.PlayerKeys[1].Keys));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

void MenuCreateKeysSingleSection(
	menu_t *menu, const char *sectionName,
	input_keys_t *keys, input_keys_t *keysOther)
{
	MenuAddSubmenu(menu, MenuCreateSeparator(sectionName));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey("Left", KEY_CODE_LEFT, keys, keysOther));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Right", KEY_CODE_RIGHT, keys, keysOther));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Up", KEY_CODE_UP, keys, keysOther));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Down", KEY_CODE_DOWN, keys, keysOther));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Fire", KEY_CODE_BUTTON1, keys, keysOther));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Switch/slide", KEY_CODE_BUTTON2, keys, keysOther));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
}

menu_t *MenuCreateOptionChangeKey(
	const char *name, key_code_e code,
	input_keys_t *keys, input_keys_t *keysOther)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_CHANGE_KEY);
	menu->u.changeKey.code = code;
	menu->u.changeKey.keys = keys;
	menu->u.changeKey.keysOther = keysOther;
	return menu;
}
