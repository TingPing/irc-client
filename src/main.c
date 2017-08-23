/* main.c
 *
 * Copyright (C) 2015 Patrick Griffis <tingping@tingping.se>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

G_DEFINE_AUTOPTR_CLEANUP_FUNC(lua_State, lua_close);

int
main (int argc, char **argv)
{
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	g_autoptr(GBytes) data = g_resources_lookup_data ("/se/tingping/IrcClient/main.lua",
	                                                  G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
	g_assert (data != NULL);
	const char *str = g_bytes_get_data (data, NULL);
	g_assert (str != NULL);

	g_autoptr(lua_State) L = luaL_newstate ();
	if (L == NULL) g_critical ("Failed to allocate Lua state");
	luaL_openlibs (L);

	// Create arg global
	lua_newtable (L);
	for (int i = 0; i < argc; ++i)
	{
		lua_pushstring (L, argv[i]);
		lua_rawseti (L, -2, i + 1);
	}
	lua_setglobal (L, "arg");

	// Create config global
	lua_newtable (L);
	lua_pushstring (L, "LIBDIR"); lua_pushstring (L, LIBDIR); lua_settable (L, -3);
	lua_pushstring (L, "PACKAGE_VERSION"); lua_pushstring (L, PACKAGE_VERSION); lua_settable (L, -3);
	lua_pushstring (L, "PACKAGE_NAME"); lua_pushstring (L, PACKAGE_NAME); lua_settable (L, -3);
	lua_setglobal (L, "config");

	// Run main.lua
	int ret = luaL_loadstring (L, str);
	if (ret != 0)
	{
		g_print ("%s\n", luaL_optstring (L, -1, ""));
		return ret;
	}

	ret = lua_pcall (L, 0, 1, 0);
	if (ret != 0)
	{
		g_print ("%s\n", luaL_optstring (L, -1, ""));
		return ret;
	}

	// Get exit status
	return (int)luaL_optinteger (L, -1, 1);
}
