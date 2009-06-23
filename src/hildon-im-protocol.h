/**
   @file: hildon-im-protocol.h
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

#ifndef __HILDON_IM_PROTOCOL_H__
#define __HILDON_IM_PROTOCOL_H__

#include <X11/X.h>
#include <gtk/gtkenums.h>

G_BEGIN_DECLS

/* IM atoms for each message type */
typedef enum
{
  HILDON_IM_WINDOW,
  HILDON_IM_ACTIVATE,
  HILDON_IM_INPUT_MODE,
  HILDON_IM_INSERT_UTF8,
  HILDON_IM_SURROUNDING,
  HILDON_IM_SURROUNDING_CONTENT,
  HILDON_IM_KEY_EVENT,
  HILDON_IM_COM,
  HILDON_IM_CLIPBOARD_COPIED,
  HILDON_IM_CLIPBOARD_SELECTION_QUERY,
  HILDON_IM_CLIPBOARD_SELECTION_REPLY,
  HILDON_IM_PREEDIT_COMMITTED,
  HILDON_IM_PREEDIT_COMMITTED_CONTENT,

  /* always last */
  HILDON_IM_NUM_ATOMS
} HildonIMAtom;

/* Returns the Atom of a given HildonIMAtom */
Atom hildon_im_protocol_get_atom(HildonIMAtom atom_name);

/* IM atom names */
#define HILDON_IM_WINDOW_NAME                    "_HILDON_IM_WINDOW"
#define HILDON_IM_ACTIVATE_NAME                  "_HILDON_IM_ACTIVATE"
#define HILDON_IM_INPUT_MODE_NAME                "_HILDON_IM_INPUT_MODE"
#define HILDON_IM_SURROUNDING_NAME               "_HILDON_IM_SURROUNDING"
#define HILDON_IM_SURROUNDING_CONTENT_NAME       "_HILDON_IM_SURROUNDING_CONTENT"
#define HILDON_IM_KEY_EVENT_NAME                 "_HILDON_IM_KEY_EVENT"
#define HILDON_IM_INSERT_UTF8_NAME               "_HILDON_IM_INSERT_UTF8"
#define HILDON_IM_COM_NAME                       "_HILDON_IM_COM"
#define HILDON_IM_CLIPBOARD_COPIED_NAME          "_HILDON_IM_CLIPBOARD_COPIED"
#define HILDON_IM_CLIPBOARD_SELECTION_QUERY_NAME "_HILDON_IM_CLIPBOARD_SELECTION_QUERY"
#define HILDON_IM_CLIPBOARD_SELECTION_REPLY_NAME "_HILDON_IM_CLIPBOARD_SELECTION_REPLY"
#define HILDON_IM_PREEDIT_COMMITTED_NAME         "_HILDON_IM_PREEDIT_COMMITTED"
#define HILDON_IM_PREEDIT_COMMITTED_CONTENT_NAME "_HILDON_IM_PREEDIT_COMMITTED_CONTENT"

/* IM ClientMessage formats */
#define HILDON_IM_WINDOW_ID_FORMAT 32
#define HILDON_IM_ACTIVATE_FORMAT 8
#define HILDON_IM_INPUT_MODE_FORMAT 8
#define HILDON_IM_SURROUNDING_FORMAT 8
#define HILDON_IM_SURROUNDING_CONTENT_FORMAT 8
#define HILDON_IM_KEY_EVENT_FORMAT 8
#define HILDON_IM_INSERT_UTF8_FORMAT 8
#define HILDON_IM_COM_FORMAT 8
#define HILDON_IM_CLIPBOARD_FORMAT 32
#define HILDON_IM_CLIPBOARD_SELECTION_REPLY_FORMAT 32
#define HILDON_IM_PREEDIT_COMMITTED_FORMAT 8
#define HILDON_IM_PREEDIT_COMMITTED_CONTENT_FORMAT 8

/* IM commands and notifications, from context to IM process */
typedef enum
{
  HILDON_IM_MODE,        /* Update the hildon-input-mode property */
  HILDON_IM_SHOW,        /* Show the IM UI */
  HILDON_IM_HIDE,        /* Hide the IM UI */
  HILDON_IM_UPP,         /* Uppercase autocap state at cursor */
  HILDON_IM_LOW,         /* Lowercase autocap state at cursor */
  HILDON_IM_DESTROY,     /* DEPRECATED */
  HILDON_IM_CLEAR,       /* Clear the IM UI state */
  HILDON_IM_SETCLIENT,   /* Set the client window */
  HILDON_IM_SETNSHOW,    /* Set the client and show the IM window */
  HILDON_IM_SELECT_ALL,  /* Select the text in the plugin */

  HILDON_IM_SHIFT_LOCKED,
  HILDON_IM_SHIFT_UNLOCKED,
  HILDON_IM_MOD_LOCKED,
  HILDON_IM_MOD_UNLOCKED,

  /* always last */
  HILDON_IM_NUM_COMMANDS
} HildonIMCommand;

/* IM communications, from IM process to context */
typedef enum
{
  HILDON_IM_CONTEXT_HANDLE_ENTER,           /* Virtual enter activated */
  HILDON_IM_CONTEXT_HANDLE_TAB,             /* Virtual tab activated */
  HILDON_IM_CONTEXT_HANDLE_BACKSPACE,       /* Virtual backspace activated */
  HILDON_IM_CONTEXT_HANDLE_SPACE,           /* Virtual space activated */
  HILDON_IM_CONTEXT_CONFIRM_SENTENCE_START, /* Query the autocap state at cursor */
  HILDON_IM_CONTEXT_FLUSH_PREEDIT,          /* Finalize the preedit to the client widget */
  HILDON_IM_CONTEXT_CANCEL_PREEDIT,          /* Clean the preedit buffer */

  /* See HildonIMCommitMode for a description of the commit modes */
  HILDON_IM_CONTEXT_BUFFERED_MODE,
  HILDON_IM_CONTEXT_DIRECT_MODE,
  HILDON_IM_CONTEXT_REDIRECT_MODE,
  HILDON_IM_CONTEXT_SURROUNDING_MODE,
  HILDON_IM_CONTEXT_PREEDIT_MODE,

  HILDON_IM_CONTEXT_CLIPBOARD_COPY,            /* Copy client selection to clipboard */
  HILDON_IM_CONTEXT_CLIPBOARD_CUT,             /* Cut client selection to clipboard */
  HILDON_IM_CONTEXT_CLIPBOARD_PASTE,           /* Paste clipboard selection to client */
  HILDON_IM_CONTEXT_CLIPBOARD_SELECTION_QUERY, /* Query if the client has an active selection */
  HILDON_IM_CONTEXT_REQUEST_SURROUNDING,       /* Request the content surrounding the cursor */
  HILDON_IM_CONTEXT_REQUEST_SURROUNDING_FULL,  /* Request the contents of the text widget */
  HILDON_IM_CONTEXT_WIDGET_CHANGED,            /* IM detected that the client widget changed */
  HILDON_IM_CONTEXT_OPTION_CHANGED,            /* The OptionMask for the active context is updated */
  HILDON_IM_CONTEXT_CLEAR_STICKY,              /* Clear the sticky key state */
  HILDON_IM_CONTEXT_ENTER_ON_FOCUS,            /* Generate a virtual enter key event on focus in */
  
  HILDON_IM_CONTEXT_SPACE_AFTER_COMMIT,        /* Append a space when the preedit text is committed */
  HILDON_IM_CONTEXT_NO_SPACE_AFTER_COMMIT,     /* Do not append said space */

  /* always last */
  HILDON_IM_CONTEXT_NUM_COM
} HildonIMCommunication;

/* IM context toggle options */
typedef enum {
  HILDON_IM_AUTOCASE          = 1 << 0, /* Suggest case based on cursor's position in sentence */
  HILDON_IM_AUTOCORRECT       = 1 << 1, /* Limited automatic error correction of commits */
  HILDON_IM_AUTOLEVEL_NUMERIC = 1 << 2, /* Default to appropriate key-level in numeric-only clients */
  HILDON_IM_LOCK_LEVEL        = 1 << 3, /* Lock the effective key-level at pre-determined value */
} HildonIMOptionMask;

/* IM trigger types, i.e. what is causing the IM plugin to active */
typedef enum
{
  HILDON_IM_TRIGGER_NONE = -1, /* Reserved for non-UI plugins not requiring activation */
  HILDON_IM_TRIGGER_STYLUS,    /* The user poked the screen with the stylus; not used in Fremantle */
  HILDON_IM_TRIGGER_FINGER,    /* The user poked the screen with his finger */
  HILDON_IM_TRIGGER_KEYBOARD,   /* The user pressed a key */
  HILDON_IM_TRIGGER_UNKNOWN
} HildonIMTrigger;

/* Commit modes
   Determines how text is inserted into the client widget

   Buffered mode:  Each new commit replaces any previous commit to the
   client widget until FLUSH_PREEDIT is called.

   Direct mode (default): Each commit is immediately appended to the
   client widget at the cursor position.

   Redirect mode: Proxies input and cursor movement from one text widget
   into another (potentially off-screen) widget. Used when implementing
   fullscreen IM plugins for widgets that contain text formatting.

   Surrounding mode: Each commit replaces the current text surrounding
   the cursor position (see gtk_im_context_get_surrounding).
*/
typedef enum
{
  HILDON_IM_COMMIT_DIRECT,
  HILDON_IM_COMMIT_REDIRECT,
  HILDON_IM_COMMIT_SURROUNDING,
  HILDON_IM_COMMIT_BUFFERED,
  HILDON_IM_COMMIT_PREEDIT,
} HildonIMCommitMode;

/* Command activation message, from context to IM (see HildonIMCommand) */
typedef struct
{
  Window input_window;
  Window app_window;
  HildonIMCommand cmd;
  HildonIMTrigger trigger;
} HildonIMActivateMessage;

typedef struct
{
#ifdef MAEMO_CHANGES
  HildonGtkInputMode input_mode;
  HildonGtkInputMode default_input_mode;
#else
  gint input_mode;
  guint default_input_mode;
#endif
} HildonIMInputModeMessage;

/* event.xclient.data may not exceed 20 bytes -> HildonIMInsertUtf8Message
   may not exceed 20 bytes. So the maximum size of message buffer is
   20 - sizeof(msg_flags) = 16. */
#define HILDON_IM_CLIENT_MESSAGE_BUFFER_SIZE (20 - sizeof(int))

/* Text insertion message, from IM to context */
typedef struct
{
  int msg_flag;
  char utf8_str[HILDON_IM_CLIENT_MESSAGE_BUFFER_SIZE];
} HildonIMInsertUtf8Message;

/* Message carrying surrounding interpretation info, sent by both IM and context */
typedef struct
{
  HildonIMCommitMode commit_mode;
  int offset_is_relative;
  int cursor_offset;
} HildonIMSurroundingMessage;

/* The surrounding text, sent by both IM and context */
typedef struct
{
  int msg_flag;
  char surrounding[HILDON_IM_CLIENT_MESSAGE_BUFFER_SIZE];
} HildonIMSurroundingContentMessage;

/* Message carrying information about the committed preedit */
typedef struct
{
  int msg_flag;
  HildonIMCommitMode commit_mode;
} HildonIMPreeditCommittedMessage;

/* The committed preedit text, sent by context */
typedef struct
{
  int msg_flag;
  char committed_preedit[HILDON_IM_CLIENT_MESSAGE_BUFFER_SIZE];
} HildonIMPreeditCommittedContentMessage;

/* Key event message, from context to IM */
typedef struct
{
  Window input_window;
  int type;
  unsigned int state;
  unsigned int keyval;
  unsigned int hardware_keycode;
} HildonIMKeyEventMessage;

/* Type markers for IM messages that span several ClientMessages */
enum
{
  HILDON_IM_MSG_START,
  HILDON_IM_MSG_CONTINUE,
  HILDON_IM_MSG_END
};

/* Communication message from IM to context */
typedef struct
{
  Window input_window;
  HildonIMCommunication type;
  HildonIMOptionMask options;

} HildonIMComMessage;

G_END_DECLS

#endif
