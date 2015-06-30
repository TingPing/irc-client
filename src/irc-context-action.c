/* irc-context-action.c
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

#include "irc-context-action.h"
#include "irc-context-manager.h"

struct _IrcContextAction
{
	GObject parent_instance;
};

typedef struct
{
	IrcContextActionCallback callback;
	char *name;
} IrcContextActionPrivate;

static void irc_context_action_iface_init (GActionInterface *);

G_DEFINE_TYPE_WITH_CODE (IrcContextAction, irc_context_action, G_TYPE_OBJECT,
						G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, irc_context_action_iface_init)
						G_ADD_PRIVATE (IrcContextAction))

static const GVariantType *
irc_context_action_get_parameter_type (GAction *action)
{
	return G_VARIANT_TYPE_STRING;
}
static const GVariantType *
irc_context_action_get_state_type (GAction *action)
{
	return NULL;
}
static gboolean
irc_context_action_get_enabled (GAction *action)
{
	return TRUE;
}
static GVariant *
irc_context_action_get_state (GAction *action)
{
	return NULL;
};
static GVariant *
irc_context_action_get_state_hint (GAction *action)
{
	return NULL;
};
static void
irc_context_action_change_state (GAction *action, GVariant *value)
{
}

static void
irc_context_action_activate (GAction *action, GVariant *param)
{
	IrcContextAction *self = IRC_CONTEXT_ACTION(action);
  	IrcContextActionPrivate *priv = irc_context_action_get_instance_private (self);
	IrcContextManager *mgr = irc_context_manager_get_default ();
	IrcContext *ctx = irc_context_manager_find (mgr, g_variant_get_string (param, NULL));

	if (ctx == NULL)
	{
		g_warning ("Action called with invalid context");
		return;
	}

	priv->callback (ctx);
}

static const char *
irc_context_action_get_name (GAction *action)
{
  	IrcContextAction *self = IRC_CONTEXT_ACTION(action);
	IrcContextActionPrivate *priv = irc_context_action_get_instance_private (self);

	return priv->name;
}

/**
 * irc_context_action_new:
 * @name: Name of action
 * @callback: (scope notified): Function to be called on GAction::activate
 *
 * Custom implementation of #GAction intended to make it eaiser to add custom
 * actions to an #IrcContext.
 *
 * Returns: (transfer full): New #GAction
 */
GAction *
irc_context_action_new (const char *name, IrcContextActionCallback callback)
{
	IrcContextAction *self = g_object_new (IRC_TYPE_CONTEXT_ACTION, NULL);
	IrcContextActionPrivate *priv = irc_context_action_get_instance_private (self);

	priv->callback = callback;
	priv->name = g_strdup (name);

	return G_ACTION(self);
}

static void
irc_context_action_finalize (GObject *object)
{
	IrcContextAction *self = IRC_CONTEXT_ACTION(object);
	IrcContextActionPrivate *priv = irc_context_action_get_instance_private (self);

	g_free (priv->name);

	G_OBJECT_CLASS (irc_context_action_parent_class)->finalize (object);
}

enum
{
	PROP_0,
	PROP_NAME,
	PROP_ENABLED,
	PROP_PARAMETER_TYPE,
	PROP_STATE_TYPE,
	PROP_STATE,
	N_PROPS,
};

static void
irc_context_action_get_property (GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GAction *action = G_ACTION(obj);

	switch (prop_id)
	{
	case PROP_NAME:
		g_value_set_string (value, irc_context_action_get_name (action));
		break;
	case PROP_ENABLED:
		g_value_set_boolean (value, irc_context_action_get_enabled (action));
		break;
	case PROP_PARAMETER_TYPE:
		g_value_set_boxed (value, irc_context_action_get_parameter_type (action));
		break;
	case PROP_STATE:
		g_value_set_boxed (value, irc_context_action_get_state (action));
		break;
	case PROP_STATE_TYPE:
		g_value_set_boxed (value, irc_context_action_get_state_type (action));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
irc_context_action_class_init (IrcContextActionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = irc_context_action_finalize;
	object_class->get_property = irc_context_action_get_property;

	g_object_class_override_property (object_class, PROP_NAME, "name");
	g_object_class_override_property (object_class, PROP_PARAMETER_TYPE, "parameter-type");
	g_object_class_override_property (object_class, PROP_ENABLED, "enabled");
	g_object_class_override_property (object_class, PROP_STATE_TYPE, "state-type");
	g_object_class_override_property (object_class, PROP_STATE, "state");
}

static void
irc_context_action_iface_init (GActionInterface *iface)
{
	iface->get_enabled = irc_context_action_get_enabled;
	iface->activate = irc_context_action_activate;
	iface->change_state = irc_context_action_change_state;
	iface->get_parameter_type = irc_context_action_get_parameter_type;
	iface->get_state_type = irc_context_action_get_state_type;
	iface->get_state_hint = irc_context_action_get_state_hint;
	iface->get_state = irc_context_action_get_state;
	iface->get_name = irc_context_action_get_name;
}

static void
irc_context_action_init (IrcContextAction *self)
{
}
