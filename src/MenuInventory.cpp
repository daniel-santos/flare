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
 * class MenuInventory
 */

#include "MenuInventory.h"
#include "SharedResources.h"
#include "WidgetLabel.h"

#include <sstream>
#include <iostream>

using namespace std;


MenuInventory::MenuInventory(ItemManager &_items, StatBlock &_stats, PowerManager &_powers)
		: items(_items)
		, stats(_stats)
		, powers(_powers) {
	visible = false;
	loadGraphics();

	window_area.w = 320;
	window_area.h = 416;
	window_area.x = VIEW_W - window_area.w;
	window_area.y = (VIEW_H - window_area.h)/2;

	equipped_area.x = window_area.x + 32;
	equipped_area.y = window_area.y + 48;
	equipped_area.w = 256;
	equipped_area.h = 64;

	carried_area.x = window_area.x + 32;
	carried_area.y = window_area.y + 128;
	carried_area.w = 256;
	carried_area.h = 256;

	inventory[EQUIPMENT].init(MAX_EQUIPPED, items, equipped_area, ICON_SIZE_64, 4);
	inventory[CARRIED].init(MAX_CARRIED, items, carried_area, ICON_SIZE_32, 8);

	gold = 0;

	drag_prev_src = -1;
	changed_equipment = true;
	changed_artifact = true;
	log_msg = "";

	closeButton = new WidgetButton(mods->locate("images/menus/buttons/button_x.png"));
	closeButton->pos.x = VIEW_W - 26;
	closeButton->pos.y = (VIEW_H - 480)/2 + 34;
}

void MenuInventory::loadGraphics() {

	background = IMG_Load(mods->locate("images/menus/inventory.png").c_str());
	if(!background) {
		fprintf(stderr, "Couldn't load image: %s\n", IMG_GetError());
		SDL_Quit();
	}

	// optimize
	SDL_Surface *cleanup = background;
	background = SDL_DisplayFormatAlpha(background);
	SDL_FreeSurface(cleanup);
}

void MenuInventory::logic() {

	// if the player has just died, the penalty is half his current gold.
	if (stats.death_penalty) {
		gold = gold/2;
		stats.death_penalty = false;
	}

	// a copy of gold is kept in stats, to help with various situations
	stats.gold = gold;

	// check close button
	if (visible) {
		if (closeButton->checkClick()) {
			visible = false;
		}
	}
}

void MenuInventory::render() {
	if (!visible) return;

	SDL_Rect src;

	// background
	src.x = 0;
	src.y = 0;
	src.w = window_area.w;
	src.h = window_area.h;
	SDL_BlitSurface(background, &src, screen, &window_area);

	// close button
	closeButton->render();

	// text overlay
	WidgetLabel label;
	label.set(window_area.x+160, window_area.y+8, JUSTIFY_CENTER, VALIGN_TOP, msg->get("Inventory"), FONT_WHITE);
	label.render();
	label.set(window_area.x+64, window_area.y+34, JUSTIFY_CENTER, VALIGN_TOP, msg->get("Main Hand"), FONT_WHITE);
	label.render();
	label.set(window_area.x+128, window_area.y+34, JUSTIFY_CENTER, VALIGN_TOP, msg->get("Body"), FONT_WHITE);
	label.render();
	label.set(window_area.x+192, window_area.y+34, JUSTIFY_CENTER, VALIGN_TOP, msg->get("Off Hand"), FONT_WHITE);
	label.render();
	label.set(window_area.x+256, window_area.y+34, JUSTIFY_CENTER, VALIGN_TOP, msg->get("Artifact"), FONT_WHITE);
	label.render();
	label.set(window_area.x+288, window_area.y+114, JUSTIFY_RIGHT, VALIGN_TOP, msg->get("%d Gold", gold), FONT_WHITE);
	label.render();

	inventory[EQUIPMENT].render();
	inventory[CARRIED].render();
}

int MenuInventory::areaOver(const Point &mouse) {
	if (isWithin(equipped_area, mouse)) {
		return EQUIPMENT;
	}
	else if (isWithin(carried_area, mouse)) {
		return CARRIED;
	}
	return -1;
}

/**
 * If mousing-over an item with a tooltip, return that tooltip data.
 *
 * @param mouse The x,y screen coordinates of the mouse cursor
 */
void MenuInventory::checkTooltip(WidgetTooltip &tip, const Point &mouse) {
	int area;

	area = areaOver(mouse);
	if (area > -1) {
		inventory[area].checkTooltip(tip, mouse, stats, false);
	}
	else if (mouse.x >= window_area.x + 224 && mouse.y >= window_area.y+96 && mouse.x < window_area.x+288 && mouse.y < window_area.y+128) {
		// TODO: I think we should add a little "?" icon in a corner, and show this title on it.
		tip.clear();
		tip.addLine(msg->get("Use SHIFT to move only one item."));
		tip.addLine(msg->get("CTRL-click a carried item to sell it."));
	}
}

/**
 * Click-start dragging in the inventory
 */
ItemStack MenuInventory::click(const InputState &input) {
	ItemStack item;
	item.item = NULL;
	item.quantity = 0;

	drag_prev_src = areaOver(input.mouse);
	if( drag_prev_src > -1) {
		item = inventory[drag_prev_src].click(input);
		// if dragging equipment, prepare to change stats/sprites
		if (drag_prev_src == EQUIPMENT) {
			updateEquipment( inventory[EQUIPMENT].drag_prev_slot);
		}
	}

	return item;
}

/**
 * Return dragged item to previous slot
 */
void MenuInventory::itemReturn(const ItemStack &stack) {
	inventory[drag_prev_src].itemReturn( stack);
	// if returning equipment, prepare to change stats/sprites
	if (drag_prev_src == EQUIPMENT) {
		updateEquipment( inventory[EQUIPMENT].drag_prev_slot);
	}
	drag_prev_src = -1;
}

/**
 * Dragging and dropping an item can be used to rearrange the inventory
 * and equip items
 */
void MenuInventory::drop(const Point &mouse, const ItemStack &stack) {
	int area;
	int slot;
	int drag_prev_slot;

	assert(stack.item);
	if (!stack.item)
		return;
	items.playSound(*stack.item);

	area = areaOver( mouse);
	slot = inventory[area].slotOver(mouse);
	drag_prev_slot = inventory[drag_prev_src].drag_prev_slot;

	if (area == EQUIPMENT) { // dropped onto equipped item

		// make sure the item is going to the correct slot
		// note: equipment slots 0-3 correspond with item types 0-3
		// also check to see if the hero meets the requirements
		if (drag_prev_src == CARRIED && slot == stack.item->type && requirementsMet(*stack.item)) {
			if( inventory[area][slot].item == stack.item) {
				// Merge the stacks
				add( stack, area, slot);
			}
			else if( inventory[drag_prev_src][drag_prev_slot].item == NULL) {
				// Swap the two stacks
				itemReturn( inventory[area][slot]);
				inventory[area][slot] = stack;
				updateEquipment( slot);
			} else {
				itemReturn( stack);
			}
		}
		else {
			// equippable items only belong to one slot, for the moment
			itemReturn( stack); // cancel
		}
	}
	else if (area == CARRIED) {
		// dropped onto carried item

		if (drag_prev_src == CARRIED) {
			if (slot != drag_prev_slot) {
				if( inventory[area][slot].item == stack.item) {
					// Merge the stacks
					add( stack, area, slot);
				}
				else if( inventory[area][slot].item == 0) {
					// Drop the stack
					inventory[area][slot] = stack;
				}
				else if( inventory[drag_prev_src][drag_prev_slot].item == 0) { // Check if the previous slot is free (could still be used if SHIFT was used).
					// Swap the two stacks
					itemReturn( inventory[area][slot]);
					inventory[area][slot] = stack;
				} else {
					itemReturn( stack);
				}
			}
			else {
				itemReturn( stack); // cancel
			}
		}
		else {
		    // note: equipment slots 0-3 correspond with item types 0-3
			// also check to see if the hero meets the requirements
			if (inventory[area][slot].item == stack.item) {
				// Merge the stacks
				add( stack, area, slot);
			}
			else if( inventory[area][slot].item == 0) {
				// Drop the stack
				inventory[area][slot] = stack;
			}
			else if(
				inventory[EQUIPMENT][drag_prev_slot].item == 0
				&& inventory[CARRIED][slot].item != stack.item
				&& inventory[CARRIED][slot].item->type == drag_prev_slot
				&& requirementsMet(*inventory[CARRIED][slot].item)
			) { // The whole equipped stack is dropped on an empty carried slot or on a wearable item
				// Swap the two stacks
				itemReturn( inventory[area][slot]);
				inventory[area][slot] = stack;
			}
			else {
				itemReturn( stack); // cancel
			}
		}
	}
	else {
		itemReturn( stack); // not dropped into a slot. Just return it to the previous slot.
	}

	drag_prev_src = -1;
}

/**
 * Right-clicking on a usable item in the inventory causes it to activate.
 * e.g. drink a potion
 * e.g. equip an item
 */
void MenuInventory::activate(const InputState &input) {
	int slot;
	int equip_slot;
	ItemStack stack;
	Point nullpt(0, 0);

	// clicked a carried item
	slot = inventory[CARRIED].slotOver(input.mouse);
	const Item &item = *inventory[CARRIED][slot].item;
	assert(&item);
	if (!&item)
		return;

	// use a consumable item
	if (item.type == ITEM_TYPE_CONSUMABLE) {

		// if this item requires targeting it can't be used this way
		if (!powers.powers[item.power].requires_targeting) {

			powers.activate(item.power, stats, nullpt);
			// intercept used_item flag.  We will destroy the item here.
			powers.used_item = -1;
			inventory[CARRIED].substract(slot);
		}
		else {
			// let player know this can only be used from the action bar
			log_msg = msg->get("This item can only be used from the action bar.");
		}

	}
	// equip an item
	else {
		equip_slot = item.type;
		if (equip_slot == ITEM_TYPE_MAIN ||
			 equip_slot == ITEM_TYPE_BODY ||
			 equip_slot == ITEM_TYPE_OFF ||
			 equip_slot == ITEM_TYPE_ARTIFACT) {
			if (requirementsMet(item)) {
				stack = click( input);	// don't use item reference after this call.
				const ItemStack &equip_stack = inventory[EQUIPMENT][equip_slot];
				const Item *equip_item = inventory[EQUIPMENT][equip_slot].item;
				if(!equip_item) {
					// Drop the stack
					inventory[EQUIPMENT][equip_slot] = stack;
				}
				else if (equip_item == stack.item) {
					// Merge the stacks
					add( stack, EQUIPMENT, equip_slot);
				}
				else {
					if(!inventory[CARRIED][slot].item) { // Don't forget this slot may have been emptied by the click()
						// Swap the two stacks
						itemReturn( inventory[EQUIPMENT][equip_slot]);
					}
					else {
						// Drop the equipped item anywhere
						add( inventory[EQUIPMENT][equip_slot]);
					}
					inventory[EQUIPMENT][equip_slot] = stack;
				}
				updateEquipment( equip_slot);
				items.playSound(*inventory[EQUIPMENT][equip_slot].item);
			}
		}
	}
}

/**
 * Insert item into first available carried slot, preferably in the optionnal specified slot
 *
 * @param ItemStack Stack of items
 * @param area Area number where it will try to store the item
 * @param slot Slot number where it will try to store the item
 */
void MenuInventory::add(const ItemStack &stack, int area, int slot) {
	int max_quantity;
	int quantity_added;
	int i;

	if (!stack.item) return;

	items.playSound(*stack.item);

	if( area < 0) {
		area = CARRIED;
	}
	max_quantity = stack.item->max_quantity;
	if( slot > -1 && inventory[area][slot].item && inventory[area][slot].item != stack.item) {
		// the proposed slot isn't available, search for another one
		slot = -1;
	}
	if( area == CARRIED) {
		// first search of stack to complete if the item is stackable
		i = 0;
		while( max_quantity > 1 && slot == -1 && i < MAX_CARRIED) {
			if (inventory[area][i].item == stack.item && inventory[area][i].quantity < max_quantity) {
				slot = i;
			}
			i++;
		}
		// then an empty slot
		i = 0;
		while( slot == -1 && i < MAX_CARRIED) {
			if (!inventory[area][i].item) {
				slot = i;
			}
			i++;
		}
	}
	if( slot != -1) {
		// Add
		ItemStack newStack(stack);

		quantity_added = min( stack.quantity, max_quantity - inventory[area][slot].quantity);
		inventory[area][slot].item = stack.item;
		inventory[area][slot].quantity += quantity_added;
		newStack.quantity -= quantity_added;
		// Add back the remaining
		if( newStack.quantity > 0) {
			if( drag_prev_src > -1) {
				itemReturn( newStack);
			} else {
				add( newStack);
			}
		}
	}
	else {
		// No available slot, drop
		// TODO: We should drop on the floor an item we can't store
	}
}

/**
 * Remove one given item from the player's inventory.
 */
void MenuInventory::remove(const Item &item) {
	if( ! inventory[CARRIED].remove(item)) {
		inventory[EQUIPMENT].remove(item);
	}
}

/**
 * Add gold to the current total
 */
void MenuInventory::addGold(int count) {
	gold += count;
	items.playCoinsSound();
}

/**
 * Check if there is enough gold to buy the given stack, and if so remove it from the current total and add the stack.
 * (Handle the drop into the equipment area, but add() don't handle it well in all circonstances. MenuManager::logic() allow only into the carried area.)
 */
bool MenuInventory::buy(const ItemStack &stack, const Point &mouse) {
	int area;
	int slot = -1;
	int count = stack.item->price * stack.quantity;

	if( gold >= count) {
		gold -= count;

		area = areaOver( mouse);
		if( area > -1) {
			slot = inventory[area].slotOver( mouse);
		}
		if( slot > -1) {
			add( stack, area, slot);
		}
		else {
			add(stack);
		}
		items.playCoinsSound();
		return true;
	}
	else {
		return false;
	}
}

/**
 * Sell a specific stack of items
 */
bool MenuInventory::sell(const ItemStack &stack) {
	assert(stack.item);
	// items that have no price cannot be sold
	if (stack.item->price == 0) return false;

	int value_each = stack.item->getSellPrice();
	if (value_each == 0) value_each = 1;
	int value = value_each * stack.quantity;
	gold += value;
	items.playCoinsSound();
	return true;
}

/**
 * Cannot pick up new items if the inventory is full.
 * Full means no more carrying capacity (equipped capacity is ignored)
 */
bool MenuInventory::full() {
	return inventory[CARRIED].full();
}

/**
 * Get the number of the specified item carried (not equipped)
 */
int MenuInventory::getItemCountCarried(const Item &item) const {
	return inventory[CARRIED].count(item);
}

/**
 * Check to see if the given item is equipped
 */
bool MenuInventory::isItemEquipped(const Item &item) const {
	return inventory[EQUIPMENT].contain(item);
}

/**
 * Check requirements on an item
 */
bool MenuInventory::requirementsMet(const Item &item) const {
	if (item.req_stat == REQUIRES_PHYS) {
		return (stats.get_physical() >= item.req_val);
	}
	else if (item.req_stat == REQUIRES_MENT) {
		return (stats.get_mental() >= item.req_val);
	}
	else if (item.req_stat == REQUIRES_OFF) {
		return (stats.get_offense() >= item.req_val);
	}
	else if (item.req_stat == REQUIRES_DEF) {
		return (stats.get_defense() >= item.req_val);
	}
	// otherwise there is no requirement, so it is usable.
	return true;
}

void MenuInventory::updateEquipment(int slot) {
	if (slot < SLOT_ARTIFACT) {
		changed_equipment = true;
	}
	else {
		changed_artifact = true;
	}
}

/**
 * Given the equipped items, calculate the hero's stats
 */
void MenuInventory::applyEquipment(ItemStack *equipped) {

	int prev_hp = stats.hp;
	int prev_mp = stats.mp;

	// calculate bonuses to basic stats and check that each equipped item fit requirements
	bool checkRequired = true;
	while(checkRequired)
	{
		checkRequired = false;
		stats.offense_additional = stats.defense_additional = stats.physical_additional = stats.mental_additional = 0;
		for (int i = 0; i < 4; i++) {
			if (!equipped[i].item) continue;
			const Item &item = *equipped[i].item;

			for(int j = 0; j != ITEM_MAX_BONUSES && !item.bonus_stat[j].empty(); ++j) {
				const string &stat = item.bonus_stat[j];
				int val = item.bonus_val[j];

				if (stat == "offense")
					stats.offense_additional += val;
				else if (stat == "defense")
					stats.defense_additional += val;
				else if (stat == "physical")
					stats.physical_additional += val;
				else if (stat == "mental")
					stats.mental_additional += val;
				else if (stat == "all basic stats") {
					stats.offense_additional += val;
					stats.defense_additional += val;
					stats.physical_additional += val;
					stats.mental_additional += val;
				}
			}
		}
		for (int i = 0; i < 4; i++) {
			if (!equipped[i].item) continue;
			if (!requirementsMet(*equipped[i].item)) {
				add(equipped[i]);
				equipped[i].item = 0;
				equipped[i].quantity = 0;
				checkRequired = true;
			}
		}
	}

	// defaults
	stats.recalc();
	stats.offense_additional = stats.defense_additional = stats.physical_additional = stats.mental_additional = 0;
	stats.dmg_melee_min = stats.dmg_ment_min = 1;
	stats.dmg_melee_max = stats.dmg_ment_max = 4;
	stats.dmg_ranged_min = stats.dmg_ranged_max = 0;
	stats.absorb_min = stats.absorb_max = 0;
	stats.speed = 14;
	stats.dspeed = 10;
	stats.attunement_fire = 100;
	stats.attunement_ice = 100;

	// reset wielding vars
	stats.wielding_physical = false;
	stats.wielding_mental = false;
	stats.wielding_offense = false;

	// main hand weapon
	if (equipped[SLOT_MAIN].item) {
		const Item &item = *equipped[SLOT_MAIN].item;
		if (item.req_stat == REQUIRES_PHYS) {
			stats.dmg_melee_min = item.dmg_min;
			stats.dmg_melee_max = item.dmg_max;
			stats.melee_weapon_power = item.power_mod;
			stats.wielding_physical = true;
		}
		else if (item.req_stat == REQUIRES_MENT) {
			stats.dmg_ment_min = item.dmg_min;
			stats.dmg_ment_max = item.dmg_max;
			stats.mental_weapon_power = item.power_mod;
			stats.wielding_mental = true;
		}
	}
	// off hand item
	if (equipped[SLOT_OFF].item) {
		const Item &item = *equipped[SLOT_OFF].item;
		if (item.req_stat == REQUIRES_OFF) {
			stats.dmg_ranged_min = item.dmg_min;
			stats.dmg_ranged_max = item.dmg_max;
			stats.ranged_weapon_power = item.power_mod;
			stats.wielding_offense = true;
		}
		else if (item.req_stat == REQUIRES_DEF) {
			stats.absorb_min += item.abs_min;
			stats.absorb_max += item.abs_max;
		}
	}
	// body item
	if (equipped[SLOT_BODY].item) {
		const Item &item = *equipped[SLOT_BODY].item;
		stats.absorb_min += item.abs_min;
		stats.absorb_max += item.abs_max;
	}



	// apply bonuses from all items
	for (int i=0; i<4; i++) {
		if (!equipped[i].item) continue;
		const Item &item = *equipped[i].item;

		for(int j = 0; j != ITEM_MAX_BONUSES && !item.bonus_stat[j].empty(); ++j) {
			const string &stat = item.bonus_stat[j];
			int val = item.bonus_val[j];

			if (stat == "HP")				stats.maxhp += val;
			else if (stat == "HP regen")	stats.hp_per_minute += val;
			else if (stat == "MP")			stats.maxmp += val;
			else if (stat == "MP regen")	stats.mp_per_minute += val;
			else if (stat == "accuracy")	stats.accuracy += val;
			else if (stat == "avoidance")	stats.avoidance += val;
			else if (stat == "crit")		stats.crit += val;
			else if (stat == "speed") {
				stats.speed += val;
				// speed bonuses are in multiples of 3
				// 3 ordinal, 2 diagonal is rounding pythagorus
				stats.dspeed += ((val) * 2) /3;
			}
			else if (stat == "fire resist")	stats.attunement_fire -= val;
			else if (stat == "ice resist")	stats.attunement_ice -= val;
			else if (stat == "offense")		stats.offense_additional += val;
			else if (stat == "defense")		stats.defense_additional += val;
			else if (stat == "physical")	stats.physical_additional += val;
			else if (stat == "mental")		stats.mental_additional += val;
			else if (stat == "all basic stats") {
				stats.offense_additional += val;
				stats.defense_additional += val;
				stats.physical_additional += val;
				stats.mental_additional += val;
			}
			else {
				std::cout << "WARNING: unknown stat '" << stat << "' on item '"
						  << item.name << "'." << std::endl;
			}
		}
	}

	// apply previous hp/mp
	if (prev_hp < stats.maxhp)
		stats.hp = prev_hp;
	else
		stats.hp = stats.maxhp;

	if (prev_mp < stats.maxmp)
		stats.mp = prev_mp;
	else
		stats.mp = stats.maxmp;

}

MenuInventory::~MenuInventory() {
	SDL_FreeSurface(background);
	delete closeButton;
}
