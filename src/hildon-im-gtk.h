/**
   @file: hildon-im-gtk.h

 */
/*
 * This file is part of hildon-input-method-framework
 *
 * Copyright (C) 2007 Nokia Corporation.
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


#ifndef __HILDON_IM_GTK_H__
#define __HILDON_IM_GTK_H__

#include <gtk/gtk.h>

/**
 * hildon_im_gtk_focus_next_text_widget:
 *
 * @current_focus: the widget currently focused
 * @direction: the direction in which to look for the next text widget
 *
 * Moves the focus to the next text input widget within the @current_focus
 * ancestor. If the given @direction is GTK_DIR_TAB_BACKWARD, it will look
 * for the widget backwards in the list of the widgets to look up.
 *
 */
void
hildon_im_gtk_focus_next_text_widget(GtkWidget *current_focus,
                                     GtkDirectionType direction);

#endif
