/**
   @file: hildon-im-common.c

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


#include <string.h>
#include <glib.h>

#include "hildon-im-common.h"



gboolean
hildon_im_common_changes_case(const gchar *chr)
{
  gunichar uni;
  if (chr == NULL)
  {
    return TRUE;
  }

  uni = g_utf8_get_char(chr);

  return (uni == '.' || uni == '!' || uni == '?'
          || uni == 0x00a1 /* inverted exclamation mark */
          || uni == 0x00bf); /* inverted question mark */
}

gboolean
hildon_im_common_check_auto_cap (const gchar *content, gint offset)
{
  gchar *iter;
  gboolean space = FALSE;
  gunichar ch;

  if (content == NULL || content[0] == '\0')
    return TRUE;

  iter = g_utf8_offset_to_pointer (content, offset);
  while (TRUE)
  {
    iter = g_utf8_find_prev_char (content, iter);

    if (iter == NULL)
      break;

    ch = g_utf8_get_char (iter);

    if (g_unichar_isspace (ch))
    {
      space = TRUE;
    }
    else if (g_unichar_ispunct (ch))
    {
      if (space && hildon_im_common_changes_case (iter))
        return TRUE;
      else
        continue;
    }
    else
    {
      break;
    }

  }

  return FALSE;
}

gint
hildon_im_autocorrection_check_character (const gchar *text)
{
  gunichar uni;
  gint retval = 0;

  uni = g_utf8_get_char (text);
  switch (uni)
  {
    case '.':
    case ',':
    case '?':
    case '!':
      retval = 1;
      break;
    case 0x00A1:
    case 0x00BF:
      retval = 2;
      break;
    default:
      retval = 0;
      break;
  }
  return retval;
}

gboolean
hildon_im_common_should_be_appended_after_letter (const gchar *chr)
{
  gunichar uni;

  if (chr == NULL)
  {
    return FALSE;
  }

  uni = g_utf8_get_char(chr);
  return (uni == '.' || uni == ',' || uni == '?' || uni == '!' || uni == '"' ||
          uni == '\'');
}
