/**
   @file: hildon-im-module.c

 */
/*
 * This file is part of hildon-input-method-framework
 *
 * Copyright (C) 2005-2007 Nokia Corporation.
 *
 * Contact: Mohammad Anwari <Mohammad.Anwari@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */


#include <gtk/gtkimmodule.h>
#include "hildon-im-gtk-compat.h"
#include "hildon-im-context.h"
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define N_(String) (String)

/*GType hildon_im_type = 0;*/

/*the following need to be exported for use by the GTK.*/
void im_module_list(const GtkIMContextInfo ***contexts, guint *n_contexts);
void im_module_init(GTypeModule *module);
void im_module_exit(void);
GtkIMContext *im_module_create(const gchar *context_id);

/*our context info*/
static const
GtkIMContextInfo hildon_im_info =
{
  HILDON_IM_CONTEXT_ID, /* ID */
  N_("Hildon Input Method"),      /* Human readable name */
  PACKAGE,                      /* Translation domain */
  LOCALEDIR,                    /* Dir for bindtextdomain */
  "*"                   /* Languages for which this module is the default */
};

static const
GtkIMContextInfo *info_list[] = { &hildon_im_info };

/* sets the arguments to the context values */
void
im_module_list(const GtkIMContextInfo ***contexts, guint *n_contexts )
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

/* registers the given module*/
void
im_module_init(GTypeModule *module)
{
  hildon_im_context_register_type(module);
}

void
im_module_exit(void)
{
  /*purposely empty*/
  /* TODO: find out wether we could use this for
     something useful, when we have several IMContexts */
}

/* returns a new GtkIMContext with the id HILDON_IM_CONTEXT_ID or NULL
 * if given id does not match HILDON_IM_CONTEXT_ID
 */
GtkIMContext *
im_module_create(const gchar *context_id)
{
  if (g_ascii_strcasecmp(context_id, HILDON_IM_CONTEXT_ID) == 0)
  {
    return hildon_im_context_new();
  }

  return NULL;
}
