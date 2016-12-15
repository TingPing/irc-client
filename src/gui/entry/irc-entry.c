/* irc-entry.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc-entry.h"
#include "irc-entrybuffer.h"
#include "irc-context-manager.h"

typedef struct
{
	GtkTreeModel *comp_model;
} IrcEntryPrivate;

struct _IrcEntry
{
	GtkEntry parent_instance;
};

G_DEFINE_TYPE_WITH_PRIVATE (IrcEntry, irc_entry, GTK_TYPE_ENTRY)

static void
irc_entry_push_into_history (GSimpleAction *action, GVariant *param, gpointer data)
{
	GtkEntry *self = GTK_ENTRY(data);
	IrcEntrybuffer *buf = IRC_ENTRYBUFFER(gtk_entry_get_buffer (self));

	irc_entrybuffer_push_into_history (buf);
}

static void
irc_entry_history_up (GSimpleAction *action, GVariant *param, gpointer data)
{
	GtkEntry *self = GTK_ENTRY(data);
	IrcEntrybuffer *buf = IRC_ENTRYBUFFER(gtk_entry_get_buffer (self));

	irc_entrybuffer_history_up (buf);
}

static void
irc_entry_history_down (GSimpleAction *action, GVariant *param, gpointer data)
{
	GtkEntry *self = GTK_ENTRY(data);
	IrcEntrybuffer *buf = IRC_ENTRYBUFFER(gtk_entry_get_buffer (self));

	irc_entrybuffer_history_down (buf);
}

static void
irc_entry_tab_complete (GSimpleAction *action, GVariant *param, gpointer data)
{
	IrcEntry *self = IRC_ENTRY(data);
	IrcEntryPrivate *priv = irc_entry_get_instance_private (self);

	if (priv->comp_model == NULL)
		return;
}

void
irc_entry_set_completion_model (IrcEntry *self, GtkTreeModel *model)
{
  	IrcEntryPrivate *priv = irc_entry_get_instance_private (self);

	priv->comp_model = model;

	// TODO: This is just temp, need to manually handle completion
	// as this is inflexible
	GtkEntryCompletion *comp = gtk_entry_completion_new ();
	gtk_entry_completion_set_model (comp, model);
	gtk_entry_completion_set_text_column (comp, 0);
	gtk_entry_completion_set_popup_completion (comp, FALSE);
	gtk_entry_completion_set_inline_completion (comp, TRUE);
	gtk_entry_set_completion (GTK_ENTRY(self), comp);
}

static void
irc_entry_activate (GtkEntry *entry)
{
	IrcContextManager *mgr = irc_context_manager_get_default ();
	IrcContext *ctx = irc_context_manager_get_front_context (mgr);

	const char *text = gtk_entry_get_text (entry);
	if (text != NULL && *text)
	{
		if (IRC_IS_CONTEXT(ctx))
		{
			if (*text == '/')
			{
				irc_context_run_command (ctx, text + 1);
			}
			else
			{
				g_autofree char *say_command = g_strconcat ("say ", text, NULL);
				irc_context_run_command (ctx, say_command);
			}
		}

		irc_entrybuffer_push_into_history (IRC_ENTRYBUFFER(gtk_entry_get_buffer (entry))); // Gets rid of text
	}

	GTK_ENTRY_CLASS (irc_entry_parent_class)->activate (entry);
}


IrcEntry *
irc_entry_new (void)
{
	IrcEntrybuffer *buf = irc_entrybuffer_new ();
	// This buffer should never really be used
	// but this class should *always* have a buffer of the correct type
	return g_object_new (IRC_TYPE_ENTRY, "buffer", buf, NULL);
}

static void
irc_entry_finalize (GObject *object)
{
	//IrcEntry *self = IRC_ENTRY(object);
	//IrcEntryPrivate *priv = irc_entry_get_instance_private (self);

	G_OBJECT_CLASS (irc_entry_parent_class)->finalize (object);
}

static void
irc_entry_class_init (IrcEntryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkEntryClass *entry_class = GTK_ENTRY_CLASS (klass);

	object_class->finalize = irc_entry_finalize;
	entry_class->activate = irc_entry_activate;
}

static void
irc_entry_init (IrcEntry *self)
{
	GSimpleActionGroup *group = g_simple_action_group_new ();
	const GActionEntry actions[] = {
		{ .name = "push_into_history", .activate = irc_entry_push_into_history },
		{ .name = "history_up", .activate = irc_entry_history_up },
		{ .name = "history_down", .activate = irc_entry_history_down },
		{ .name = "tab_complete", .activate = irc_entry_tab_complete, .parameter_type = "b" },
	};
	const char * const push_accels[] = { "<Primary>Up", NULL };
	const char * const up_accels[] = { "Up", NULL };
	const char * const down_accels[] = { "Down", NULL };
	const char * const tab_accels[] = { "Tab", NULL };
	const char * const tabreverse_accels[] = { "<Shift>ISO_Left_Tab", NULL };

	g_action_map_add_action_entries (G_ACTION_MAP (group), actions, G_N_ELEMENTS(actions), self);
	gtk_widget_insert_action_group (GTK_WIDGET(self), "entry", G_ACTION_GROUP(group));

	GtkApplication *app = GTK_APPLICATION(g_application_get_default ());
	gtk_application_set_accels_for_action (app, "entry.push_into_history", push_accels );
	gtk_application_set_accels_for_action (app, "entry.history_up", up_accels );
	gtk_application_set_accels_for_action (app, "entry.history_down", down_accels );
	gtk_application_set_accels_for_action (app, "entry.tab_complete(false)", tab_accels );
	gtk_application_set_accels_for_action (app, "entry.tab_complete(true)", tabreverse_accels );

	GtkStyleContext *style = gtk_widget_get_style_context (GTK_WIDGET(self));
	gtk_style_context_add_class (style, "irc-textview");
}
