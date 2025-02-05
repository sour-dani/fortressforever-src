
// ff_lualib_constants.h

/////////////////////////////////////////////////////////////////////////////
#ifndef FF_LUALIB_CONSTANTS_H
#define FF_LUALIB_CONSTANTS_H

/////////////////////////////////////////////////////////////////////////////
// Purpose: HUD item types in lua
/////////////////////////////////////////////////////////////////////////////

enum LuaHudItemTypes
{
	LUA_HUD_ITEM_ICON = 0,
	LUA_HUD_ITEM_TEXT,
	LUA_HUD_ITEM_TIMER,
	LUA_HUD_ITEM_REMOVE,

	LUA_HUD_ITEM_MAX
};

/////////////////////////////////////////////////////////////////////////////
// Purpose: Ammo types in lua
/////////////////////////////////////////////////////////////////////////////
enum LuaAmmoTypes
{
	LUA_AMMO_SHELLS = 0,
	LUA_AMMO_NAILS,
	LUA_AMMO_CELLS,
	LUA_AMMO_ROCKETS,
	LUA_AMMO_DETPACK,
	LUA_AMMO_MANCANNON,
	LUA_AMMO_GREN1,
	LUA_AMMO_GREN2,

	LUA_AMMO_INVALID,
};

//===========================================================================
// Purpose: Convert lua ammo type (int) to game ammo type (string)
//===========================================================================
const char *LookupLuaAmmo( int iLuaAmmoType );

//===========================================================================
// Purpose: Convert ammo to lua ammo
//===========================================================================
int LookupAmmoLua( int iAmmoType );

#endif
