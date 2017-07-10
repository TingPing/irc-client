/* irc-entry.c
 *
 * Copyright (C) 2017 Patrick Griffis <tingping@tingping.se>
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

#include "irc-entry.h"
#include "irc-entrybuffer.h"
#include "irc-context-manager.h"

struct _IrcEntry
{
	IrcTextview parent_instance;
};

G_DEFINE_TYPE (IrcEntry, irc_entry, IRC_TYPE_TEXTVIEW)

static void
irc_entry_push_into_history (GSimpleAction *action, GVariant *param, gpointer data)
{
	GtkTextView *self = GTK_TEXT_VIEW(data);
	IrcEntrybuffer *buf = IRC_ENTRYBUFFER(gtk_text_view_get_buffer (self));

	irc_entrybuffer_push_into_history (buf);
}

static void
irc_entry_history_up (GSimpleAction *action, GVariant *param, gpointer data)
{
	GtkTextView *self = GTK_TEXT_VIEW(data);
	IrcEntrybuffer *buf = IRC_ENTRYBUFFER(gtk_text_view_get_buffer (self));

	irc_entrybuffer_history_up (buf);
}

static void
irc_entry_history_down (GSimpleAction *action, GVariant *param, gpointer data)
{
	GtkTextView *self = GTK_TEXT_VIEW(data);
	IrcEntrybuffer *buf = IRC_ENTRYBUFFER(gtk_text_view_get_buffer (self));

	irc_entrybuffer_history_down (buf);
}

static void
irc_entry_undo (GSimpleAction *action, GVariant *param, gpointer data)
{
	GtkTextView *self = GTK_TEXT_VIEW(data);
	GtkSourceBuffer *buf = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer (self));

	gtk_source_buffer_undo (buf);
}

static void
irc_entry_redo (GSimpleAction *action, GVariant *param, gpointer data)
{
	GtkTextView *self = GTK_TEXT_VIEW(data);
	GtkSourceBuffer *buf = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer (self));

	gtk_source_buffer_redo (buf);
}

static void
irc_entry_activate (GSimpleAction *action, GVariant *param, gpointer data)
{
	GtkTextView *self = GTK_TEXT_VIEW(data);
	GtkTextBuffer *buf = gtk_text_view_get_buffer (self);
	IrcContextManager *mgr = irc_context_manager_get_default ();
	IrcContext *ctx = irc_context_manager_get_front_context (mgr);

	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter (buf, &start);
	end = start;
	gboolean has_another_line = gtk_text_iter_forward_to_line_end (&end);

	while (TRUE)
	{
		g_autofree char *text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
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
		}

		if (!has_another_line || !gtk_text_iter_forward_line (&start))
		{
			irc_entrybuffer_push_into_history (IRC_ENTRYBUFFER(buf)); // Gets rid of text
			return;
		}

		end = start;
		has_another_line = gtk_text_iter_forward_to_line_end (&end);
	}
}

static void
irc_entry_tab_complete (GSimpleAction *action, GVariant *param, gpointer data)
{
	GtkTextView *self = GTK_TEXT_VIEW(data);
	IrcEntrybuffer *buf = IRC_ENTRYBUFFER(gtk_text_view_get_buffer (self));

	irc_entrybuffer_tab_complete (buf);
}

IrcEntry *
irc_entry_new (void)
{
	IrcEntrybuffer *buffer = irc_entrybuffer_new ();
	return g_object_new (IRC_TYPE_ENTRY, "buffer", buffer,
	                                     "editable", TRUE,
	                                     "border-width", 3,
	                                     "margin", 6,
	                                     "top-margin", 3,
	                                     "bottom-margin", 3,
	                                     "pixels-above-lines", 3,
	                                     "pixels-below-lines", 3,
	                                     "wrap-mode", GTK_WRAP_WORD_CHAR, NULL);
}

static void
irc_entry_class_init (IrcEntryClass *klass)
{
	GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

	// Hack to theme like GtkEntry. Seems to work...
	gtk_widget_class_set_css_name (wid_class, "entry");
}

static void
irc_entry_init (IrcEntry *self)
{
	GSimpleActionGroup *group = g_simple_action_group_new ();
	const GActionEntry actions[] = {
		{ .name = "push_into_history", .activate = irc_entry_push_into_history },
		{ .name = "history_up", .activate = irc_entry_history_up },
		{ .name = "history_down", .activate = irc_entry_history_down },
		{ .name = "activate", .activate = irc_entry_activate },
		{ .name = "undo", .activate = irc_entry_undo },
		{ .name = "redo", .activate = irc_entry_redo },
		{ .name = "tab_complete", .activate = irc_entry_tab_complete, .parameter_type = "b" },
	};
	const char * const push_accels[] = { "<Primary><Shift>Up", NULL };
	const char * const up_accels[] = { "<Primary>Up", NULL };
	const char * const down_accels[] = { "<Primary>Down", NULL };
	const char * const tab_accels[] = { "Tab", NULL };
	const char * const tabreverse_accels[] = { "<Shift>ISO_Left_Tab", NULL };
	const char * const activate_accels[] = { "Return", NULL };
	const char * const undo_accels[] = { "<Primary>z", NULL };
	const char * const redo_accels[] = { "<Primary><Shift>z", NULL };

	g_action_map_add_action_entries (G_ACTION_MAP (group), actions, G_N_ELEMENTS(actions), self);
	gtk_widget_insert_action_group (GTK_WIDGET(self), "entry", G_ACTION_GROUP(group));

	GtkApplication *app = GTK_APPLICATION(g_application_get_default ());
	gtk_application_set_accels_for_action (app, "entry.push_into_history", push_accels );
	gtk_application_set_accels_for_action (app, "entry.history_up", up_accels );
	gtk_application_set_accels_for_action (app, "entry.history_down", down_accels );
	gtk_application_set_accels_for_action (app, "entry.tab_complete(false)", tab_accels );
	gtk_application_set_accels_for_action (app, "entry.tab_complete(true)", tabreverse_accels );
	gtk_application_set_accels_for_action (app, "entry.undo", undo_accels );
	gtk_application_set_accels_for_action (app, "entry.redo", redo_accels );
	gtk_application_set_accels_for_action (app, "entry.activate", activate_accels );
}
