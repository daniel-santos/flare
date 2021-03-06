/*
Copyright © 2011-2012 Clint Bellanger

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

/**
 * class NPCManager
 *
 * NPCs which are not combatative enemies are handled by this Manager.
 * Most commonly this involves vendor and conversation townspeople.
 */

#ifndef NPC_MANAGER_H
#define NPC_MANAGER_H

#include "NPC.h"
#include "MapIso.h"
#include "WidgetTooltip.h"
#include "LootManager.h"

#include <SDL.h>

// max number of NPCs for a single map
const int MAX_NPC_COUNT = 32;

class NPCManager : private Uncopyable {
private:
	MapIso &map;
	LootManager &loot;
	ItemManager &items;
	WidgetTooltip tip;

public:
	NPCManager(MapIso &_map, LootManager &_loot, ItemManager &_items);
	~NPCManager();
	NPC *npcs[MAX_NPC_COUNT];
	void handleNewMap();
	void logic();
	int checkNPCClick(const Point &mouse, const Point &cam);
	void renderTooltips(const Point &cam, const Point &mouse);
	int npc_count;
	int tooltip_margin;
};

#endif
