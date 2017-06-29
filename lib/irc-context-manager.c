/* irc-context-manager.c
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

#include <string.h>
#include "irc-context-manager.h"

struct _IrcContextManager
{
	GObject parent_instance;
};

typedef struct
{
	//GPtrArray *contexts;
	GNode *contexts;
	IrcContext *front;
} IrcContextManagerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (IrcContextManager, irc_context_manager, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_FRONT_CONTEXT,
	N_PROPS
};

enum {
	CONTEXT_ADDED,
	CONTEXT_REMOVED,
	FRONT_CHANGED,
	N_SIGNALS
};

//static GParamSpec *obj_props [N_PROPS];
static guint obj_signals[N_SIGNALS];

static void
ensure_removed (IrcContextManager *self, gpointer removed_data)
{
	IrcContextManagerPrivate *priv = irc_context_manager_get_instance_private (self);

	GNode *node = g_node_find (priv->contexts, G_POST_ORDER, G_TRAVERSE_ALL, removed_data);
	if (node)
		g_node_destroy (node);
}

static void
context_weak_notify (gpointer data, GObject *destroyed_object)
{
	ensure_removed (IRC_CONTEXT_MANAGER(data), destroyed_object);
}

/**
 * irc_context_manager_find:
 * @id: ID of an #IrcContext
 *
 * Returns: (transfer none): Found context or %NULL
 */
IrcContext *
irc_context_manager_find (IrcContextManager *self, const char *id)
{
  	IrcContextManagerPrivate *priv = irc_context_manager_get_instance_private (self);
	const char *p = strchr (id, '/');
  	const gboolean wants_child = p != NULL;
  	gsize split;
	guint i = 0;
	g_autofree char *parent_name = NULL;

	if (wants_child)
	{
		split = (gsize)(p - id);
		parent_name = g_strndup (id, split);
	}

	GNode *parent;
	while ((parent = g_node_nth_child (priv->contexts, i++)))
	{
		const char *name = irc_context_get_name (IRC_CONTEXT(parent->data));
		if (!wants_child)
		{
			if (irc_str_equal (id, name))
				return parent->data;
		}
		else
		{
			if (irc_str_equal (parent_name, name))
				break;
		}
	}

	if (parent == NULL)
		return NULL;

	GNode *child;
	i = 0;
	while ((child = g_node_nth_child (parent, i++)))
	{
		const char *name = irc_context_get_name (IRC_CONTEXT(child->data));
		if (irc_str_equal (name, id + split + 1))
		{
			return child->data;
		}
	}
	return NULL;
}

void
irc_context_manager_remove_by_id (IrcContextManager *self, const char *id)
{
	IrcContext *ctx = irc_context_manager_find (self, id);
	if (ctx != NULL)
	{
		irc_context_manager_remove (self, ctx);
	}
}

/**
 * irc_context_manager_get_front_context:
 *
 * Returns: (transfer none): The front context
 */
IrcContext *
irc_context_manager_get_front_context (IrcContextManager *self)
{
  	IrcContextManagerPrivate *priv = irc_context_manager_get_instance_private (self);
	return priv->front;
}

void
irc_context_manager_set_front_context (IrcContextManager *self, IrcContext *ctx)
{
	g_return_if_fail (ctx != NULL);

	IrcContextManagerPrivate *priv = irc_context_manager_get_instance_private (self);

	priv->front = ctx;
	g_signal_emit (self, obj_signals[FRONT_CHANGED], 0, ctx);
}

/**
 * irc_context_manager_add:
 * @ctx: Context to add
 *
 * Note: This should only be called externally if adding a top-level context
 */
void
irc_context_manager_add (IrcContextManager *self, IrcContext *ctx)
{
	IrcContextManagerPrivate *priv = irc_context_manager_get_instance_private (self);

	g_return_if_fail (ctx != NULL);
	g_return_if_fail (IRC_IS_CONTEXT(ctx));

	g_debug ("added %d", G_OBJECT(ctx)->ref_count);
	IrcContext *parent = irc_context_get_parent (ctx);
	if (parent == NULL)
	{
		g_node_append_data (priv->contexts, ctx);
		g_object_ref_sink (ctx);
	}
	else
	{
		GNode *parent_node = g_node_find_child (priv->contexts, G_TRAVERSE_ALL, parent);
		if (parent_node == NULL)
		{
			g_warning ("Parent node not found on add.");
			return;
		}
		else
		{
			g_node_append_data (parent_node, ctx);
			g_object_weak_ref (G_OBJECT(ctx), context_weak_notify, self);
		}
	}
	g_signal_emit (self, obj_signals[CONTEXT_ADDED], 0, ctx);
}

static void
child_parent_removed_foreach (GNode *node, gpointer data)
{
	// We know the parent will handle these
	g_object_weak_unref (node->data, context_weak_notify, data);

	IrcContext *ctx = IRC_CONTEXT(node->data);
	//IrcContext *parent = irc_context_get_parent (ctx);
	//irc_context_remove_child (parent, ctx);
	g_signal_emit (data, obj_signals[CONTEXT_REMOVED], 0, ctx);
}

/**
 * irc_context_manager_remove:
 * @ctx: Context to remove
 */
void
irc_context_manager_remove (IrcContextManager *self, IrcContext *ctx)
{
	IrcContextManagerPrivate *priv = irc_context_manager_get_instance_private (self);

	g_return_if_fail (ctx != NULL);
	g_return_if_fail (IRC_IS_CONTEXT(ctx));

	IrcContext *parent = irc_context_get_parent (ctx);
	GNode *context_node;
	if (parent == NULL)
	{
		context_node = g_node_find_child (priv->contexts, G_TRAVERSE_ALL, ctx);
		if (!context_node)
			g_warning ("Node not found on remove");
		else
		{
			g_node_children_foreach (context_node, G_TRAVERSE_ALL, child_parent_removed_foreach, self);
			g_node_destroy (context_node);
			g_signal_emit (self, obj_signals[CONTEXT_REMOVED], 0, ctx);
		}
		g_object_unref (ctx);
		g_debug ("removed %d", G_OBJECT(ctx)->ref_count);
	}
	else
	{
		GNode *parent_node = g_node_find_child (priv->contexts, G_TRAVERSE_ALL, parent);
		if (parent_node == NULL)
		{
			g_warning ("Parent node not found on remove.");
			context_node = NULL;
		}
		else
		{
			context_node = g_node_find_child (parent_node, G_TRAVERSE_ALL, ctx);
			if (!context_node)
				g_warning ("Node not found on remove");
			else
			{
				irc_context_remove_child (parent, ctx);
				g_node_destroy (context_node);
				g_signal_emit (self, obj_signals[CONTEXT_REMOVED], 0, ctx);
			}
		}
	}
}

void
irc_context_manager_foreach_parent (IrcContextManager *self, GNodeForeachFunc func, gpointer data)
{
	IrcContextManagerPrivate *priv = irc_context_manager_get_instance_private (self);
	g_node_children_foreach (priv->contexts, G_TRAVERSE_ALL, func, data);
}

/**
 * irc_context_manager_get_default:
 *
 * Returns: (transfer none): The default context manager
 */
IrcContextManager *
irc_context_manager_get_default (void)
{
	static IrcContextManager *default_manager;

	if (G_UNLIKELY(default_manager == NULL))
		default_manager = g_object_new (IRC_TYPE_CONTEXT_MANAGER, NULL);

	return default_manager;
}

static void
irc_context_manager_finalize (GObject *object)
{
	IrcContextManager *self = IRC_CONTEXT_MANAGER(object);
	IrcContextManagerPrivate *priv = irc_context_manager_get_instance_private (self);

	g_clear_pointer (&priv->contexts, g_node_destroy);

	G_OBJECT_CLASS (irc_context_manager_parent_class)->finalize (object);
}

static void
irc_context_manager_class_init (IrcContextManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = irc_context_manager_finalize;

	obj_signals[CONTEXT_ADDED] = g_signal_new ("context-added", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
												0, NULL, NULL, NULL, G_TYPE_NONE, 1, IRC_TYPE_CONTEXT);

	obj_signals[CONTEXT_REMOVED] = g_signal_new ("context-removed", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
												0, NULL, NULL, NULL, G_TYPE_NONE, 1, IRC_TYPE_CONTEXT);

	obj_signals[FRONT_CHANGED] = g_signal_new ("front-context-changed", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
												0, NULL, NULL, NULL, G_TYPE_NONE, 1, IRC_TYPE_CONTEXT);
}

static void
irc_context_manager_init (IrcContextManager *self)
{
  	IrcContextManagerPrivate *priv = irc_context_manager_get_instance_private (self);

	priv->contexts = g_node_new (NULL);
}
