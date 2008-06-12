/**
   @file: hildon-im-gtk.c

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

#include "hildon-im-gtk.h"

/* Widget spatial comparison from GtkContainer */
static gint
tab_compare(gconstpointer a,
            gconstpointer b,
            gpointer      data)
{
  const GtkWidget *child1 = a;
  const GtkWidget *child2 = b;
  GtkTextDirection text_direction = GPOINTER_TO_INT(data);

  gint y1 = child1->allocation.y + child1->allocation.height / 2;
  gint y2 = child2->allocation.y + child2->allocation.height / 2;

  if (y1 == y2)
  {
    gint x1 = child1->allocation.x + child1->allocation.width / 2;
    gint x2 = child2->allocation.x + child2->allocation.width / 2;
      
    if (text_direction == GTK_TEXT_DIR_RTL) 
      return (x1 < x2) ? 1 : ((x1 == x2) ? 0 : -1);
    else
      return (x1 < x2) ? -1 : ((x1 == x2) ? 0 : 1);
  }
  else
    return (y1 < y2) ? -1 : 1;
}

static GList *
container_focus_sort(GtkContainer     *container,
                     GList            *children,
                     GtkDirectionType  direction)
{
  GtkTextDirection text_direction = gtk_widget_get_direction(GTK_WIDGET(container));
  children = g_list_sort_with_data(children, tab_compare, GINT_TO_POINTER(text_direction));

  if (direction == GTK_DIR_TAB_BACKWARD)
    children = g_list_reverse(children);

  return children;
}

static void
container_children_callback(GtkWidget *widget,
                            gpointer   client_data)
{
  GList **children;

  children = (GList**)client_data;
  *children = g_list_prepend(*children, widget);
}

/* same as gtk_container_get_children, except it includes internals */
static GList *
container_get_all_children(GtkContainer *container)
{
  GList *children = NULL;

  gtk_container_forall(container,
                       container_children_callback,
                       &children);
  return children;
}

static gboolean
move_focus(GtkContainer     *container,
           GtkDirectionType direction,
           GtkWidget        *old_focus_widget)
{
  GList *children, *child;
  GtkWidget *child_widget;
  gboolean found = FALSE;

  if (container->has_focus_chain)
  {
    children = g_list_copy(g_object_get_data(G_OBJECT(container),
                                             "gtk-container-focus-chain"));

    if (direction == GTK_DIR_TAB_BACKWARD)
      children = g_list_reverse(children);
  }
  else
  {
    GList *all_children, *visible_children = NULL;
    
    child = all_children = container_get_all_children(container);
    while (child)
    {
      if (GTK_WIDGET_DRAWABLE (child->data))
        visible_children = g_list_prepend(visible_children, child->data);
      child = child->next;
    }    
    children = container_focus_sort(container, visible_children, direction);
    g_list_free(all_children);
  }

  if (old_focus_widget)
  {
    GList *old_widget_location;

    old_widget_location = g_list_find(children, old_focus_widget);
    if (old_widget_location)
    {
      children = old_widget_location->next;
    }
  }

  for (child = children; child; child = child->next)
  {
    child_widget = child->data;

    if (!child_widget)
      continue;

    if (GTK_IS_ENTRY(child_widget) || GTK_IS_TEXT_VIEW(child_widget))
    {
      if (GTK_WIDGET_CAN_FOCUS(child_widget))
      {
        gtk_widget_child_focus(child_widget, direction);
        found = TRUE;
        break;
      }
    }

    if (GTK_IS_CONTAINER(child_widget))
    {
      found = move_focus(GTK_CONTAINER(child_widget),
                         direction,
                         child_widget);
      if (found)
        break;
    }
  }

  g_list_free(children);
  return found;
}

void
hildon_im_gtk_focus_next_text_widget(GtkWidget *current_focus,
                                     GtkDirectionType direction)
{
  GtkWidget *container;
  GtkWidget *toplevel;
  GtkWidget *child;

  toplevel = gtk_widget_get_toplevel(current_focus);
  if (!GTK_WIDGET_TOPLEVEL(toplevel))
    return;

  container = gtk_widget_get_ancestor(gtk_window_get_focus(GTK_WINDOW(toplevel)),
                                      GTK_TYPE_CONTAINER);

  child = current_focus;
  while (container != NULL &&
         move_focus(GTK_CONTAINER(container),
                    direction,
                    child) == FALSE)
  {
    GtkWidget *parent = gtk_widget_get_parent(container);

    child = container;
    if (parent)
      container = gtk_widget_get_ancestor(parent, GTK_TYPE_CONTAINER);
    else
      container = NULL;
  }
}
