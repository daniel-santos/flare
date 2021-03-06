/*
Copyright © 2011-2012 kitano

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
 * class Menu
 *
 * The base class for Menu objects
 */

#ifndef MENU_H
#define MENU_H

#include <SDL_image.h>
#include "FontEngine.h"
#include "InputState.h"

class Menu : private Uncopyable {
protected:

	SDL_Surface *background;

public:
	Menu();
	virtual ~Menu();

	bool visible;
	SDL_Rect window_area;

	virtual void render() = 0;
};

#endif

