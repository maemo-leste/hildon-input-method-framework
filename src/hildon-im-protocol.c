/**
   @file: hildon-im-protocol.c

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


#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "hildon-im-gtk-compat.h"
#include "hildon-im-protocol.h"


static const char *
ATOM_NAME[HILDON_IM_NUM_ATOMS] =
{
  HILDON_IM_WINDOW_NAME,
  HILDON_IM_ACTIVATE_NAME,
  HILDON_IM_INPUT_MODE_NAME,
  HILDON_IM_INSERT_UTF8_NAME,
  HILDON_IM_SURROUNDING_NAME,
  HILDON_IM_SURROUNDING_CONTENT_NAME,
  HILDON_IM_KEY_EVENT_NAME,
  HILDON_IM_COM_NAME,
  HILDON_IM_CLIPBOARD_COPIED_NAME,
  HILDON_IM_CLIPBOARD_SELECTION_QUERY_NAME,
  HILDON_IM_CLIPBOARD_SELECTION_REPLY_NAME,
  HILDON_IM_PREEDIT_COMMITTED_NAME,
  HILDON_IM_PREEDIT_COMMITTED_CONTENT_NAME,
  HILDON_IM_LONG_PRESS_SETTINGS_NAME
};

/**
 * hildon_im_protocol_get_atom:
 * @atom_name: a #HildonIMAtom
 * @Returns: the #Atom of the given #HildonIMAtom
 *
 * Convenience function for getting hildon-keyboard related atoms.
 */
Atom
hildon_im_protocol_get_atom(HildonIMAtom atom_name)
{
  static Atom atom_list[HILDON_IM_NUM_ATOMS] = { None, None, None, None };
  Atom result = None;
  gint i;

  /*initialise the atom on the first call of this function*/
  if (atom_list[0] == None)
  {
    for (i = 0; i < HILDON_IM_NUM_ATOMS; ++i)
    {
      atom_list[i] = XInternAtom(gdk_x11_get_default_xdisplay(), ATOM_NAME[i],
                                 False);
      g_return_val_if_fail(atom_list[i], result);
    }
  }

  /*now return the atom that we want*/
  if (atom_name >= HILDON_IM_WINDOW && atom_name <= HILDON_IM_NUM_ATOMS)
  {
    result = atom_list[atom_name];
  }
  else
  {
    g_warning("HildonIMProtocol :: Trying to get an invalid atom\n");
  }

  return result;
}
