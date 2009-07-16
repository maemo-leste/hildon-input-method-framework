/**
   @file: hildon-im-context.h

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


#ifndef __HILDON_IM_CONTEXT_H__
#define __HILDON_IM_CONTEXT_H__

#include <glib.h>
#include <gtk/gtkimcontext.h>
#include <gtk/gtkwidget.h>

#include "hildon-im-protocol.h"

#define HILDON_IS_IM_CONTEXT(obj) (GTK_CHECK_TYPE (obj, im_context_type))
#define HILDON_IM_CONTEXT(obj) \
        (GTK_CHECK_CAST (obj, im_context_type, HildonIMContext))

G_BEGIN_DECLS

typedef struct _HildonIMContextClass HildonIMContextClass;

struct _HildonIMContextClass
{
  GtkIMContextClass parent_class;
};

void hildon_im_context_register_type (GTypeModule *module);

/**
 * hildon_im_context_new:
 *
 * Creates new instance of #GtkIMContext.
 *
 * Returns: a pointer to a newly allocated #GtkIMContext object
 */
GtkIMContext* hildon_im_context_new(void);

G_END_DECLS

#endif
