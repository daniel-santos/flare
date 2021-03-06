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
 * class MenuItemStorage
 */

#ifndef MENU_ITEM_STORAGE_H
#define MENU_ITEM_STORAGE_H

#include "ItemStorage.h"
#include <SDL.h>

class MenuItemStorage : public ItemStorage {
protected:
	SDL_Rect area;
	int icon_size;
	int nb_cols;

public:
	void init(int slot_number, ItemManager &items, SDL_Rect _area, int icon_size, int nb_cols);

	// rendering
	void render();
	int slotOver(const Point &mouse) const;
	void checkTooltip(WidgetTooltip &tip, const Point &mouse, const StatBlock &stats, bool vendor_view);
	ItemStack click(const InputState &input);
	void itemReturn(const ItemStack &stack);

	int drag_prev_slot;
};

#endif


