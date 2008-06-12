/**
   @file: hildon-im-common.h
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

#ifndef HILDON_IM_COMMON_H_
#define HILDON_IM_COMMON_H_

#include <glib.h>

G_BEGIN_DECLS

gboolean hildon_im_common_changes_case(const gchar *chr);
gint hildon_im_autocorrection_check_character (const gchar *);

G_END_DECLS

#endif /* ifndef HILDON_IM_COMMON_H_ */
