/*
 * hildon-im-gtk-compat.h
 *
 * Copyright (C) 2024 Ivaylo Dimitrov <ivo.g.dimitrov.75@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef __HILDON_IM_GTK_COMPAT_H__
#define __HILDON_IM_GTK_COMPAT_H__

#if !defined(MAEMO_CHANGES)
/* remove from here once we have gtk3 hildon */
typedef enum
{
  HILDON_GTK_INPUT_MODE_ALPHA             = 1 << 0,
  HILDON_GTK_INPUT_MODE_NUMERIC           = 1 << 1,
  HILDON_GTK_INPUT_MODE_SPECIAL           = 1 << 2,
  HILDON_GTK_INPUT_MODE_HEXA              = 1 << 3,
  HILDON_GTK_INPUT_MODE_TELE              = 1 << 4,
  HILDON_GTK_INPUT_MODE_FULL              = (HILDON_GTK_INPUT_MODE_ALPHA | HILDON_GTK_INPUT_MODE_NUMERIC | HILDON_GTK_INPUT_MODE_SPECIAL),
  HILDON_GTK_INPUT_MODE_NO_SCREEN_PLUGINS = 1 << 27,
  HILDON_GTK_INPUT_MODE_MULTILINE         = 1 << 28,
  HILDON_GTK_INPUT_MODE_INVISIBLE         = 1 << 29,
  HILDON_GTK_INPUT_MODE_AUTOCAP           = 1 << 30,
  HILDON_GTK_INPUT_MODE_DICTIONARY        = 1 << 31
} HildonGtkInputMode;
#endif

#endif /* __HILDON_IM_GTK_COMPAT_H__ */
