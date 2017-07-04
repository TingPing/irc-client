/* irc-context.c
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
#include "irc-context.h"
#include "irc-context-action.h"
#include "irc-context-manager.h"
#include "irc-private.h"
#include "irc-marshal.h"

G_DEFINE_INTERFACE (IrcContext, irc_context, G_TYPE_OBJECT)

enum
{
	SIGNAL_PRINT,
	SIGNAL_COMMAND,
	N_SIGNALS
};

static guint signals[N_SIGNALS];

static void
create_words (const char *content, GStrv *words_in, GStrv *words_eol_in)
{
	char *p = (char*)content;
	char *spaces[512];
	char **word_eol;
	char **word;
	guint i;

	// Get each space
	for (i = 0; i < sizeof(spaces); ++i)
	{
		if (p != NULL)
		{
			spaces[i] = i ? ++p : p++;
			p = strchr (p, ' ');
		}
		else
			break;
	}
	spaces[i] = NULL;
	const guint len = i + 1; // Including nul

	// Allocate a pointer to these
	word_eol = g_new (char*, len);
	for (i = 0; spaces[i] != NULL; ++i)
		word_eol[i] = spaces[i];
	word_eol[i] = NULL;
	*words_eol_in = word_eol;

	// Split them up
	word = g_new (char*, len);
	for (i = 0; spaces[i] != NULL; ++i)
	{
		if (spaces[i + 1] != NULL)
			word[i] = g_strndup (spaces[i], (guintptr)(spaces[i + 1] - spaces[i] - 1));
		else
			word[i] = g_strdup (spaces[i]); // Last word
	}
	word[i] = NULL;
	*words_in = word;
}


void
irc_context_remove_child (IrcContext *self, IrcContext *child)
{
	IRC_CONTEXT_GET_IFACE(self)->remove_child(self, child);
}

const char *
irc_context_get_id (IrcContext *self)
{
	char *id = g_object_get_data (G_OBJECT(self), "id");

	if (G_UNLIKELY(id == NULL))
	{
		IrcContext *parent;

		if ((parent = irc_context_get_parent (self)))
			id = g_strdup_printf ("%s/%s", irc_context_get_name (parent),
											 irc_context_get_name (self));
		else
			id = g_strdup (irc_context_get_name (self));

		g_object_set_data_full (G_OBJECT(self), "id", id, g_free);
	}

	return id;
}

/**
 * irc_context_lookup_setting_boolean:
 * @self: Context to lookup in
 * @setting_name: Setting to lookup
 *
 * If the setting has never been set for a context it will lookup
 * the setting in the parent. If all else fails it uses global settings.
 *
 * Returns: Value of setting
 */
gboolean
irc_context_lookup_setting_boolean (IrcContext *self, const char *setting_name)
{
	GSettings *settings = g_object_get_data (G_OBJECT(self), "settings");

	if (G_UNLIKELY(settings == NULL))
	{
		const char *id = irc_context_get_id (self);
		g_autofree char *path = g_strconcat ("/se/tingping/IrcClient/", id, "/", NULL);
		settings = g_settings_new_with_path ("se.tingping.context", path);
		g_object_set_data_full (G_OBJECT(self), "settings", settings, g_object_unref);
	}

	g_autoptr (GVariant) value = g_settings_get_user_value (settings, setting_name);
	if (value != NULL)
		return g_variant_get_boolean (value);

	IrcContext *parent = irc_context_get_parent (self);
	if (parent)
		return irc_context_lookup_setting_boolean (parent, setting_name);


	static GSettings *global_settings;

	if (G_UNLIKELY(global_settings == NULL))
		global_settings = g_settings_new_with_path ("se.tingping.context", "/se/tingping/IrcClient/");

	return g_settings_get_boolean (global_settings, setting_name);
}

/**
 * irc_context_get_parent:
 *
 * Returns: (transfer none): Parent context
 */
IrcContext *
irc_context_get_parent (IrcContext *self)
{
	return IRC_CONTEXT_GET_IFACE(self)->get_parent(self);
}

/**
 * irc_context_get_name:
 *
 * Gets name of context
 */
const char *
irc_context_get_name (IrcContext *self)
{
	return IRC_CONTEXT_GET_IFACE(self)->get_name(self);
}

/**
 * irc_context_print_with_time:
 *
 * Prints to context
 */
void
irc_context_print_with_time (IrcContext *self, const char *message, time_t stamp)
{
	g_signal_emit (self, signals[SIGNAL_PRINT], 0, message, stamp);
}

/**
 * irc_context_print:
 *
 * Prints to context
 */
inline void
irc_context_print (IrcContext *self, const char *message)
{
	irc_context_print_with_time (self, message, 0);
}

/**
 * irc_context_run_command:
 * @self: Context to run on
 * @command: Command to run
 */
void
irc_context_run_command (IrcContext *self, const char *command)
{
	//g_return_if_fail (command && *command);
	g_return_if_fail (IRC_IS_CONTEXT(self));

	gboolean handled;
  	g_auto(GStrv) words;
	GStrv words_eol;

	// TODO: Parse commands similar to hexchat handling quotes
	create_words (command, &words, &words_eol);

	g_signal_emit (self, signals[SIGNAL_COMMAND], g_quark_from_string (words[0]), words, words_eol, &handled);
}

/**
 * irc_context_get_menu:
 *
 * Returns: (transfer full): New menu currently valid for context
 */
GMenuModel *
irc_context_get_menu (IrcContext *self)
{
	GMenuModel *menu = IRC_CONTEXT_GET_IFACE(self)->get_menu(self);

	// Always have a close entry
	GMenu *shared_menu = g_menu_new ();
	const char *id = irc_context_get_id (self);
  	g_autofree char *action = g_strdup_printf ("context.close('%s')", id);
	g_menu_append (shared_menu, _("Close"), action);

	g_menu_append_section (G_MENU(menu), NULL, G_MENU_MODEL(shared_menu));
	return menu;
}

static void
irc_context_close (IrcContext *ctx)
{
  	IrcContextManager *mgr = irc_context_manager_get_default ();
	irc_context_manager_remove (mgr, ctx);
}

static void
irc_context_focus (IrcContext *ctx)
{
	IrcContextManager *mgr = irc_context_manager_get_default ();
	irc_context_manager_set_front_context (mgr, ctx);
}

/**
 * irc_context_get_action_group:
 *
 * Returns: (transfer full): Valid actions for this context
 */
GActionGroup *
irc_context_get_action_group (void)
{
	GSimpleActionGroup *group = g_simple_action_group_new ();
	GAction *close_action = irc_context_action_new ("close", irc_context_close);
	GAction *focus_action = irc_context_action_new ("focus", irc_context_focus);
	g_action_map_add_action (G_ACTION_MAP (group), close_action);
	g_action_map_add_action (G_ACTION_MAP (group), focus_action);
	return G_ACTION_GROUP (group);
}

static IrcContext *
irc_context_default_get_parent (IrcContext *self)
{
	return NULL;
}

static GMenuModel *
irc_context_default_get_menu (IrcContext *self)
{
	return G_MENU_MODEL(g_menu_new ());
}

static void
irc_context_default_init (IrcContextInterface *iface)
{
	iface->get_parent = irc_context_default_get_parent;
	iface->get_menu = irc_context_default_get_menu;

	g_object_interface_install_property (iface,
                   g_param_spec_string ("name", _("Name"), _("Name of context"),
                                NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

  	g_object_interface_install_property (iface,
                   g_param_spec_object ("parent", _("Parent"), _("Parent context"),
                               IRC_TYPE_CONTEXT, G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

  	g_object_interface_install_property (iface,
                   g_param_spec_boolean ("active", _("Active"), _("The context is considered active"),
                               TRUE, G_PARAM_READABLE));

	signals[SIGNAL_PRINT] = g_signal_new_class_handler ("print", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
													NULL, NULL, NULL,
													irc_marshal_VOID__STRING_INT64,
													G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_INT64);

	g_signal_new_class_handler ("activity", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION,
											NULL, NULL, NULL, irc_marshal_VOID__BOOLEAN,
											G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

	/**
	 * IrcContext::command:
	 * @object: The #IrcContext for the event
	 * @words: Words split at spaces for event
	 * @words_eol: Strings starting at each space for the event
	 *
	 * Returns: %TRUE to stop signal, %FALSE to continue
	 */
	signals[SIGNAL_COMMAND] = g_signal_new_class_handler ("command", G_TYPE_FROM_INTERFACE(iface), G_SIGNAL_RUN_LAST|G_SIGNAL_DETAILED|G_SIGNAL_NO_RECURSE,
										G_CALLBACK(handle_command), g_signal_accumulator_true_handled, NULL,
										irc_marshal_BOOLEAN__BOXED_BOXED,
										G_TYPE_BOOLEAN, 2, G_TYPE_STRV, G_TYPE_STRV);
}

