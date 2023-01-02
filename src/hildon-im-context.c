/**
   @file: hildon-im-context.c

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
#include <libintl.h>
#include <gtk/gtk.h>
#if GTK_CHECK_VERSION(3,0,0)
#include <gtk/gtkx.h>
#endif
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include <gdk/gdkx.h>
#include <pango/pango.h>
#include <cairo/cairo.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include "hildon-im-context.h"
#include "hildon-im-gtk.h"
#include "hildon-im-common.h"

#define HILDON_IM_DEFAULT_LAUNCH_DELAY 70

#define COMPOSE_KEY GDK_Multi_key
#define LEVEL_KEY GDK_ISO_Level3_Shift
#define LEVEL_KEY_MOD_MASK GDK_MOD5_MASK

#define BASE_LEVEL     0
#define NUMERIC_LEVEL  2
#define LOCKABLE_LEVEL 4

/* if we don't use gtk_im_context_get_surrounding or a text buffer to get the
 * surrounding, this is the fixed size that will be used */
#define SURROUNDING_CHARS_BEFORE_CURSOR 48
#define SURROUNDING_CHARS_AFTER_CURSOR  16

/* Maximum distance that can be dragged in order to show the IM */
#define SHOW_CONTEXT_MAX_DISTANCE 25

/* Special cased widgets */
#define HILDON_IM_INTERNAL_TEXTVIEW "him-textview"
#define HILDON_ENTRY_COMPLETION_POPUP "hildon-completion-window"
#define MAEMO_BROWSER_URL_ENTRY "maemo-browser-url-entry"

/* Long-press feature */
#define DEFAULT_LONG_PRESS_TIMEOUT 600

static GtkIMContextClass *parent_class;
static GType im_context_type = 0;

static gulong grab_focus_hook_id = 0;
static gulong unmap_hook_id = 0;
static guint launch_delay_timeout_id = 0;
static guint launch_delay = HILDON_IM_DEFAULT_LAUNCH_DELAY;
static HildonIMTrigger trigger = HILDON_IM_TRIGGER_UNKNOWN;
static gboolean internal_reset = FALSE;
static gboolean enter_on_focus_pending = FALSE;

#if GTK_CHECK_VERSION(3,0,0)
static GtkIMContext *current_context = NULL;
#endif

typedef struct _HildonIMContext HildonIMContext;

struct _HildonIMContext
{
  GtkIMContext context;

  HildonIMInternalModifierMask mask;
  HildonIMOptionMask options;

  GdkWindow *client_gdk_window;
  GtkWidget *client_gtk_widget;
  gboolean is_internal_widget;
  
  Window im_window;

  HildonIMCommitMode commit_mode;
  HildonIMCommitMode previous_commit_mode;

  GString *preedit_buffer;
  /* we need the incoming preedit buffer because the message might be split */
  GString *incoming_preedit_buffer;
  /* keep the preedit's position on GtkTextView or GtkEditable */
  GtkTextMark *text_view_preedit_mark;
  gint editable_preedit_position;
  /* in case we want to hide the preedit buffer without canceling it */
  gboolean show_preedit;
  /* append a space after the text is committed */
  gboolean space_after_commit;

  /* IDs of handlers attached to client widget */
  gint client_changed_signal_handler;
  gint client_hide_signal_handler;
  gint client_copy_clipboard_signal_handler;

  /* State */
  gboolean last_internal_change;
  gint changed_count;
  GdkEventKey *last_key_event;
  guint32 combining_char;
  gboolean auto_upper;
  gboolean auto_upper_enabled;
  gboolean has_focus;
  gboolean is_url_entry;
  gboolean committed_preedit;

  /* Keep track on cursor position to prevent unnecessary calls */
  gint prev_cursor_x;
  gint prev_cursor_y;

  gchar *surrounding;
  guint  prev_surrounding_hash;
  guint  prev_surrounding_cursor_pos;

  gdouble button_press_x;
  gdouble button_press_y;

  /* used to implement long-press feature */
  gboolean enable_long_press;
  gboolean char_key_is_down;
  guint long_press_timeout_src_id;
  guint long_press_timeout;
  GdkEventKey *long_press_last_key_event;

  gboolean last_was_shift_backspace;
};

/* Initialisation/finalisation functions */
static void       hildon_im_context_init                (HildonIMContext*
                                                         self);

static void       hildon_im_context_class_init          (HildonIMContextClass*
                                                         im_context_class);
static void       hildon_im_context_class_finalize      (HildonIMContextClass*
                                                         im_context_class);

/* Virtual functions */
static void       hildon_im_context_focus_in            (GtkIMContext*
                                                         context);
static void       hildon_im_context_focus_out           (GtkIMContext*
                                                         context);
static void       hildon_im_context_hide                (GtkIMContext *context);
#ifdef MAEMO_CHANGES
static void       hildon_im_context_show                (GtkIMContext *context);
#endif
static gboolean   hildon_im_context_filter_event        (GtkIMContext *context,
                                                         GdkEvent     *event);

static void       hildon_im_context_reset               (GtkIMContext *context);

static gboolean   hildon_im_context_get_surrounding     (GtkIMContext *context,
                                                         gchar **text,
                                                         gint *cursor_index);

static void       hildon_im_context_set_client_window   (GtkIMContext *context,
                                                         GdkWindow *window);

static gboolean   hildon_im_context_filter_keypress     (GtkIMContext *context,
                                                         GdkEventKey *event);


static void       hildon_im_context_set_cursor_location (GtkIMContext *context,
                                                         GdkRectangle *area);

/* Private functions */
static void       hildon_im_context_show_real           (GtkIMContext *context);
static void       hildon_im_context_reset_real          (GtkIMContext *context);
static void       hildon_im_context_insert_utf8         (HildonIMContext *self,
                                                         gint flag,
                                                         const char *text);
static void       hildon_im_context_send_input_mode     (HildonIMContext *self);
static void       hildon_im_context_send_command        (HildonIMContext *self,
                                                         HildonIMCommand cmd);
static void       hildon_im_context_check_sentence_start(HildonIMContext*
                                                         self);
static void       hildon_im_context_send_surrounding    (HildonIMContext*
                                                         self,
                                                         gboolean
                                                         send_full_line);
static void       hildon_im_context_send_committed_preedit(HildonIMContext *self,
                                                           gchar* committed_preedit);
static void       hildon_im_context_send_key_event      (HildonIMContext *self,
                                                         GdkEventType type,
                                                         guint state,
                                                         guint keyval,
                                                         guint16 hardware_keycode);
static void       hildon_im_context_commit_surrounding  (HildonIMContext*
                                                         self);
static void       hildon_im_context_get_preedit_string  (GtkIMContext *context,
                                                         gchar **str,
                                                         PangoAttrList **attrs,
                                                         gint *cursor_pos);
static void       hildon_im_context_set_use_preedit     (GtkIMContext *context,
                                                         gboolean use_preedit);

/* Useful functions */
static Window          get_window_id                    (Atom window_atom);

static GdkFilterReturn client_message_filter            (GdkXEvent *xevent,
                                                         GdkEvent *event,
                                                         HildonIMContext *self);
/* this takes care of notifying changes in the buffer */
static void         set_preedit_buffer                  (HildonIMContext *self,
                                                         const gchar* s);
/* commits text */
static gboolean     commit_text                         (HildonIMContext *self,
                                                         const gchar* s);


static void hildon_im_context_set_mask_state (HildonIMContext *self,
                                              HildonIMInternalModifierMask *mask,
                                              HildonIMInternalModifierMask lock_mask,
                                              HildonIMInternalModifierMask sticky_mask,
                                              gboolean was_press_and_release);

static void hildon_im_context_send_event(HildonIMContext *self, XEvent *event);

static gboolean key_pressed (HildonIMContext *context, GdkEventKey *event);

static void hildon_im_context_abort_long_press (HildonIMContext *context);

static void
hildon_im_context_send_fake_key (guint key_val, gboolean is_press)
{
  GdkKeymapKey *keys=NULL;
  gint n_keys=0;

  if(gdk_keymap_get_entries_for_keyval (NULL, key_val, &keys, &n_keys))
  {
    XTestFakeKeyEvent(gdk_x11_get_default_xdisplay (),
                      keys->keycode, is_press, 0);
  }
  else
  {
    g_warning("Keycode not found for keyval %x", key_val);
  }

  g_free(keys);
}

void
hildon_im_context_register_type (GTypeModule *module )
{
  if (!im_context_type)
  {
    static const GTypeInfo im_context_info =
    {
      sizeof(HildonIMContextClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) hildon_im_context_class_init,
      (GClassInitFunc) hildon_im_context_class_finalize,
      NULL, /* class_data */
      sizeof ( HildonIMContext ),
      0,    /* n_preallocs */
      (GInstanceInitFunc) hildon_im_context_init,
    };
    im_context_type = g_type_module_register_type(module, GTK_TYPE_IM_CONTEXT,
                                                  "HildonIMContext",
                                                  &im_context_info, 0);
  }
}

static void
hildon_im_context_finalize(GObject *obj)
{
  HildonIMContext *imc = HILDON_IM_CONTEXT(obj);

  hildon_im_context_set_client_window(GTK_IM_CONTEXT(imc), NULL);
  g_free(imc->surrounding);

  if (imc->last_key_event)
  {
    gdk_event_free((GdkEvent*)imc->last_key_event);
  }

  if (launch_delay_timeout_id != 0)
  {
    g_source_remove(launch_delay_timeout_id);
    launch_delay_timeout_id = 0;
  }

  g_string_free (imc->preedit_buffer, TRUE);
  g_string_free (imc->incoming_preedit_buffer, TRUE);

  if (imc->long_press_last_key_event != NULL)
  {
    gdk_event_free ((GdkEvent *) imc->long_press_last_key_event);
    imc->long_press_last_key_event = NULL;
  }

  G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static GtkTextBuffer *
get_buffer(GtkWidget *widget)
{
  if (GTK_IS_TEXT_VIEW (widget))
  {
    return gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
  }

  return NULL;
}

static gboolean
hildon_im_hook_grab_focus_handler(GSignalInvocationHint *ihint,
                                  guint n_param_values,
                                  const GValue *param_values,
                                  gpointer data)
{
  GtkIMContext *context = NULL;
  GtkWidget *focus_widget, *old_focus_widget, *toplevel;

  g_return_val_if_fail(n_param_values > 0, TRUE);

  /* Widgets created by the IM have valid contexts, but focusing them
     should not cause any action 
  if (is_internal_widget)
    return TRUE;
     */

  focus_widget = g_value_get_object(&param_values[0]);

  if (!gtk_widget_get_visible (focus_widget))
    return TRUE;

  toplevel = gtk_widget_get_toplevel (focus_widget);
  if (!gtk_widget_is_toplevel (toplevel) ||
      !GTK_IS_WINDOW(toplevel))
  {
    /* No parent toplevel */
    return TRUE;
  }

  old_focus_widget = gtk_window_get_focus(GTK_WINDOW(toplevel));
  if (old_focus_widget == focus_widget)
  {
    /* Already focused there */
    return TRUE;
  }

  if (old_focus_widget == NULL ||
      !gtk_widget_has_focus(old_focus_widget))
  {
    return TRUE;
  }

#if GTK_CHECK_VERSION(3,0,0)
  if (GTK_IS_ENTRY (old_focus_widget) || GTK_IS_TEXT_VIEW (old_focus_widget))
    context = current_context;
#else
  if (GTK_IS_ENTRY (old_focus_widget))
    context = GTK_ENTRY (old_focus_widget)->im_context;
  else if (GTK_IS_TEXT_VIEW (old_focus_widget))
    context = GTK_TEXT_VIEW (old_focus_widget)->im_context;
#endif

  if (focus_widget)
  {
    gboolean is_combo_box_entry, is_inside_toolbar;
    gboolean is_editable_entry, is_editable_text_view;
    gboolean is_inside_completion_popup;
    gboolean allow_deselect;
/*    GtkWidget *parent;

    parent = gtk_widget_get_parent (focus_widget);*/
#if GTK_CHECK_VERSION(3,0,0)
    is_combo_box_entry = GTK_IS_COMBO_BOX(focus_widget);

    if (is_combo_box_entry)
      g_object_get (focus_widget, "has-entry", &is_combo_box_entry, NULL);
#else
    is_combo_box_entry = GTK_IS_COMBO_BOX_ENTRY(focus_widget);
#endif
    is_inside_toolbar =
      gtk_widget_get_ancestor (focus_widget, GTK_TYPE_TOOLBAR) != NULL;
    is_editable_entry = GTK_IS_ENTRY (focus_widget) &&
      gtk_editable_get_editable (GTK_EDITABLE(focus_widget));
    is_editable_text_view = GTK_IS_TEXT_VIEW (focus_widget) &&
      gtk_text_view_get_editable (GTK_TEXT_VIEW (focus_widget));
    is_inside_completion_popup =
      g_strcmp0(gtk_widget_get_name(toplevel), HILDON_ENTRY_COMPLETION_POPUP) == 0;

    if (focus_widget == NULL ||
        (!is_editable_entry &&
         !is_editable_text_view &&
         !GTK_IS_SCROLLBAR (focus_widget) &&
         !GTK_IS_MENU_ITEM (focus_widget) &&
         !GTK_IS_MENU (focus_widget) &&
         !is_inside_toolbar &&
         !is_combo_box_entry &&
         !is_inside_completion_popup))
    {
      if (context != NULL && HILDON_IS_IM_CONTEXT (context))
      {
        hildon_im_context_hide(context);
      }
    }

    if (is_inside_toolbar && context)
    {
      internal_reset = TRUE;

#if GTK_CHECK_VERSION(3,0,0)
      if (GTK_IS_ENTRY (old_focus_widget))
        gtk_entry_reset_im_context (GTK_ENTRY (old_focus_widget));
      else if (GTK_IS_TEXT_VIEW (old_focus_widget))
        gtk_text_view_reset_im_context (GTK_TEXT_VIEW (old_focus_widget));
#else
      gtk_im_context_reset(context);
#endif
    }
    /* Remove text highlight (selection) unless focus is moved
       inside a toolbar, scrollbar or combo */
    allow_deselect = (!is_inside_toolbar &&
                      !is_combo_box_entry &&
                      !GTK_IS_SCROLLBAR (focus_widget));

    if (allow_deselect)
    {
      GtkWidget *selection_toplevel = gtk_widget_get_toplevel(old_focus_widget);

      if (!gtk_widget_is_toplevel(selection_toplevel) ||
          selection_toplevel != toplevel)
        return TRUE;

      if (GTK_IS_ENTRY (old_focus_widget) &&
          gtk_editable_get_selection_bounds (GTK_EDITABLE (old_focus_widget), NULL, NULL))
      {
        gint pos;
        pos = gtk_editable_get_position (GTK_EDITABLE (old_focus_widget));
        gtk_editable_set_position (GTK_EDITABLE (old_focus_widget), pos);
      }
      else if (GTK_IS_TEXT_VIEW (old_focus_widget))
      {
        GtkTextBuffer *text_buff = get_buffer (old_focus_widget);

        if (gtk_text_buffer_get_selection_bounds (text_buff, NULL, NULL))
        {
          GtkTextIter insert;
          gtk_text_buffer_get_iter_at_mark (text_buff, &insert,
            gtk_text_buffer_get_insert (text_buff));
          gtk_text_buffer_place_cursor (text_buff, &insert);
        }
      }
    }
  }

  return TRUE;
}

static gboolean
hildon_im_hook_unmap_handler(GSignalInvocationHint *ihint,
                             guint n_param_values,
                             const GValue *param_values,
                             gpointer data)
{
  GtkWidget *widget;

  g_return_val_if_fail(n_param_values > 0, TRUE);

  widget = g_value_get_object(&param_values[0]);

  /* If the IM is opened for this widget, hide the IM */
  if (gtk_widget_has_focus(widget))
  {
    GtkIMContext *context = NULL;

#if GTK_CHECK_VERSION(3,0,0)
    if (GTK_IS_ENTRY (widget) || GTK_IS_TEXT_VIEW (widget))
      context = current_context;
#else
    if (GTK_IS_ENTRY (widget))
      context = GTK_ENTRY (widget)->im_context;
    else if (GTK_IS_TEXT_VIEW (widget))
      context = GTK_TEXT_VIEW (widget)->im_context;
#endif

    if (HILDON_IS_IM_CONTEXT (context))
    {
      hildon_im_context_hide(context);
    }
#if !GTK_CHECK_VERSION(3,0,0)
    else if (GTK_IS_IM_CONTEXT (context))
    {
      gtk_im_context_hide (context);
    }
#endif
  }

  return TRUE;
}

static void
hildon_im_context_class_init (HildonIMContextClass *im_context_class)
{
  gint signal_id;

  GObjectClass *object_class = G_OBJECT_CLASS(im_context_class);
  GtkIMContextClass *gtk_im_context_class = GTK_IM_CONTEXT_CLASS(im_context_class);

  parent_class = g_type_class_peek_parent( im_context_class );

  object_class->finalize = hildon_im_context_finalize;

  /* Virtual functions */
  gtk_im_context_class->focus_in = hildon_im_context_focus_in;
  gtk_im_context_class->focus_out = hildon_im_context_focus_out;
#ifdef MAEMO_CHANGES
  gtk_im_context_class->show = hildon_im_context_show;
  gtk_im_context_class->hide = hildon_im_context_hide;
  gtk_im_context_class->filter_event = hildon_im_context_filter_event;
#endif
  gtk_im_context_class->set_client_window = hildon_im_context_set_client_window;
  gtk_im_context_class->filter_keypress = hildon_im_context_filter_keypress;
  gtk_im_context_class->set_cursor_location = hildon_im_context_set_cursor_location;
  gtk_im_context_class->reset = hildon_im_context_reset;
  gtk_im_context_class->get_surrounding = hildon_im_context_get_surrounding;
  gtk_im_context_class->get_preedit_string = hildon_im_context_get_preedit_string;
  gtk_im_context_class->set_use_preedit = hildon_im_context_set_use_preedit;

  signal_id = g_signal_lookup("grab-focus", GTK_TYPE_WIDGET);
  grab_focus_hook_id =
    g_signal_add_emission_hook(signal_id, 0, hildon_im_hook_grab_focus_handler,
                               NULL, NULL);

  signal_id = g_signal_lookup("unmap-event", GTK_TYPE_WIDGET);
  unmap_hook_id =
    g_signal_add_emission_hook(signal_id, 0, hildon_im_hook_unmap_handler,
                               NULL, NULL);
}

static void
hildon_im_context_class_finalize(HildonIMContextClass *im_context_class)
{
  gint signal_id;

  signal_id = g_signal_lookup("grab-focus", GTK_TYPE_WIDGET);
  g_signal_remove_emission_hook(signal_id, grab_focus_hook_id);

  signal_id = g_signal_lookup("unmap-event", GTK_TYPE_WIDGET);
  g_signal_remove_emission_hook(signal_id, unmap_hook_id);
}

GtkIMContext *
hildon_im_context_new(void)
{
  return g_object_new(im_context_type, NULL);
}

/* Retrieves the X window id of the IM */
static Window
get_window_id(Atom window_atom)
{
  Window result = None;
  unsigned long n=0;
  unsigned long extra=0;
  gint format=0;
  gint status=0;

  Atom realType;
  union
  {
    Window *win;
    unsigned char *val;
  } value;

  gdk_error_trap_push();
  status = XGetWindowProperty(gdk_x11_get_default_xdisplay (),
                              GDK_ROOT_WINDOW(),
                              window_atom, 0L, 4L, 0,
                              XA_WINDOW, &realType, &format,
                              &n, &extra, (unsigned char **) &value.val);

  if (gdk_error_trap_pop() == 0 &&
      (status == Success && realType == XA_WINDOW
      && format == HILDON_IM_WINDOW_ID_FORMAT && n == 1 && value.win != None))
  {
    result = value.win[0];
    XFree(value.val);
  }
  else
  {
    g_warning("Unable to get the window id\n");
  }

  return result;
}


static void
hildon_get_input_mode(HildonIMContext *self, HildonGtkInputMode *input_mode,
                      HildonGtkInputMode *default_input_mode)
{
#if defined(MAEMO_CHANGES) || GTK_CHECK_VERSION(3,0,0)
#if defined(MAEMO_CHANGES)
  if (input_mode)
    g_object_get (self, "hildon-input-mode", input_mode, NULL);

  if (default_input_mode)
    g_object_get (self, "hildon-input-default", default_input_mode, NULL);
#else
  GtkInputPurpose purpose = 0;
  GtkInputHints hints = 0;
  HildonGtkInputMode mode = 0;

  g_object_get (self,
                "input-purpose", &purpose,
                "input-hints", &hints,
                NULL);

  switch (purpose)
  {
    case GTK_INPUT_PURPOSE_ALPHA:
      mode = HILDON_GTK_INPUT_MODE_ALPHA;
    case GTK_INPUT_PURPOSE_DIGITS:
      mode = HILDON_GTK_INPUT_MODE_NUMERIC;
    case GTK_INPUT_PURPOSE_NUMBER:
      mode = HILDON_GTK_INPUT_MODE_NUMERIC | HILDON_GTK_INPUT_MODE_SPECIAL;
    case GTK_INPUT_PURPOSE_PHONE:
      mode = HILDON_GTK_INPUT_MODE_TELE;
    case GTK_INPUT_PURPOSE_PASSWORD:
    case GTK_INPUT_PURPOSE_PIN:
      mode = HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_INVISIBLE;
    default:
      mode = HILDON_GTK_INPUT_MODE_FULL;
  }

  if (hints & GTK_INPUT_HINT_UPPERCASE_SENTENCES)
    mode |= HILDON_GTK_INPUT_MODE_AUTOCAP;

  if (hints & GTK_INPUT_HINT_WORD_COMPLETION)
    mode |=  HILDON_GTK_INPUT_MODE_DICTIONARY ;

  if (input_mode)
    *input_mode = mode;

  if (default_input_mode)
    *default_input_mode = mode;
#endif
}
#else

  if (input_mode)
    *input_mode = 0;

  if (default_input_mode)
    *default_input_mode = 0;
}
#endif

#if defined(MAEMO_CHANGES) || GTK_CHECK_VERSION(3,0,0)
static void
hildon_im_context_input_mode_changed(GObject *object, GParamSpec *pspec)
{
  HildonIMContext *self = HILDON_IM_CONTEXT(object);
  HildonGtkInputMode input_mode = 0;
  HildonGtkInputMode default_input_mode = 0;

  hildon_get_input_mode (self, &input_mode, &default_input_mode);

  self->auto_upper_enabled =
    ( (self->options & HILDON_IM_AUTOCASE) != 0 &&
      (input_mode & HILDON_GTK_INPUT_MODE_AUTOCAP) != 0);

  if (self->client_gtk_widget != NULL && gtk_widget_has_focus (self->client_gtk_widget))
  {
    /* Notify IM of any input mode changes in cases where the UI is
       already visible. */
    hildon_im_context_send_input_mode(self);
  }
}
#endif

#ifndef MAEMO_CHANGES
static gboolean
button_press_release (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  HildonIMContext *self = HILDON_IM_CONTEXT(user_data);

  if ((GTK_IS_ENTRY(widget) || GTK_IS_TEXT_VIEW(widget)) &&
      (!self->is_internal_widget ||  self->is_url_entry))
  {
    return hildon_im_context_filter_event (GTK_IM_CONTEXT(self), (GdkEvent *)event);
  }
  return FALSE;
}
#endif

static void
hildon_im_context_init(HildonIMContext *self)
{
  self->prev_cursor_y = None;
  self->prev_cursor_x = None;
  self->has_focus = FALSE;
  self->surrounding = g_strdup("");
  self->preedit_buffer = g_string_new ("");
  self->show_preedit = FALSE;
  self->space_after_commit = FALSE;
  self->is_internal_widget = FALSE;
  self->im_window = get_window_id(hildon_im_protocol_get_atom(HILDON_IM_WINDOW));
  self->commit_mode = HILDON_IM_COMMIT_REDIRECT;
  self->previous_commit_mode = self->commit_mode;
  self->incoming_preedit_buffer = g_string_new ("");

  self->auto_upper_enabled = FALSE;
  self->auto_upper = FALSE;

  self->button_press_x = -1.0;
  self->button_press_y = -1.0;

  self->enable_long_press = TRUE;
  self->char_key_is_down = FALSE;
  self->long_press_timeout_src_id = 0;
  self->long_press_timeout = DEFAULT_LONG_PRESS_TIMEOUT;
  self->long_press_last_key_event = NULL;

  self->last_was_shift_backspace = FALSE;

#ifdef MAEMO_CHANGES
  g_signal_connect(self, "notify::hildon-input-mode",
    G_CALLBACK(hildon_im_context_input_mode_changed), NULL);
  g_signal_connect(self, "notify::hildon-input-default",
      G_CALLBACK(hildon_im_context_input_mode_changed), NULL);
#elif GTK_CHECK_VERSION(3,0,0)
  g_signal_connect(self, "notify::input-purpose",
    G_CALLBACK(hildon_im_context_input_mode_changed), NULL);
  g_signal_connect(self, "notify::input-hints",
      G_CALLBACK(hildon_im_context_input_mode_changed), NULL);
#endif
}

/* gets the character with relation to the cursor
 * offset : -1 for previous char, 0 for char at cursor 
 * returns 0x0 if there was an error */
static gunichar
get_unichar_by_offset_from_cursor (HildonIMContext *self,
                                   gint desired_offset)
{
  gchar *surrounding = NULL;
  gchar *character = NULL;
  gunichar unicharacter = 0x0;
  gint offset = 0;
  
  hildon_im_context_get_surrounding (GTK_IM_CONTEXT(self), &surrounding, &offset);
  
  if (surrounding != NULL)
  {
    character = g_utf8_offset_to_pointer (surrounding, (glong)(offset+desired_offset));
    
    if (character != NULL)
    {
      unicharacter = g_utf8_get_char_validated(character, -1);
    }
  }
  
  g_free(surrounding);
  
  if (unicharacter == (gunichar)-1 || unicharacter == (gunichar)-2)
    unicharacter = 0x0;
  
  return unicharacter;
}

static gboolean
commit_text (HildonIMContext *self, const gchar* s)
{
  g_return_val_if_fail(HILDON_IS_IM_CONTEXT(self), FALSE);
#if GTK_CHECK_VERSION(3,0,0)
  if (s == NULL)
#else
  if (self->client_gtk_widget == NULL
      || s == NULL)
#endif
    return FALSE;

  g_signal_emit_by_name(self, "commit", s);

  return TRUE;
}

static void
set_preedit_buffer (HildonIMContext *self, const gchar* s)
{
  HildonGtkInputMode input_mode;

  hildon_get_input_mode (self, &input_mode, NULL);

  if ((input_mode & HILDON_GTK_INPUT_MODE_DICTIONARY) == 0 ||
      (input_mode & HILDON_GTK_INPUT_MODE_INVISIBLE) != 0)
  {
    return;
  }

  if (self->client_gtk_widget == NULL
      || !gtk_widget_get_realized(self->client_gtk_widget))
    return;

  if (s != NULL)
  {
    GtkTextIter cursor;
    GtkTextBuffer *buffer;
    gchar *up_string = NULL;
    gchar *string;
    
    if (self->mask & HILDON_IM_SHIFT_LOCK_MASK)
    {
      up_string = g_utf8_strup(s, -1);
      string = up_string;
    }
    else
    {
      string = (gchar*)s;
    }
    
    if (self->preedit_buffer == NULL)
    {
      self->preedit_buffer = g_string_new (string);
    }
    else
    {
      g_string_append(self->preedit_buffer, string);
    }

    if (GTK_IS_TEXT_VIEW (self->client_gtk_widget))
    {
      buffer = get_buffer(self->client_gtk_widget);
      gtk_text_buffer_get_iter_at_mark(buffer, &cursor,
                                       gtk_text_buffer_get_insert(buffer));
      gtk_text_buffer_move_mark (buffer, self->text_view_preedit_mark, &cursor);
    }
    else if (GTK_IS_EDITABLE (self->client_gtk_widget))
    {
      self->editable_preedit_position =
        gtk_editable_get_position(GTK_EDITABLE(self->client_gtk_widget));
    }
    
    self->show_preedit = TRUE;
    g_signal_emit_by_name(self, "preedit-changed", self->preedit_buffer->str);
    
    g_free(up_string);
  }
  else
  {
    self->show_preedit = FALSE;
    
    if (self->preedit_buffer != NULL && self->preedit_buffer->len != 0)
    {
      g_string_truncate(self->preedit_buffer, 0);
      g_signal_emit_by_name(self, "preedit-changed", self->preedit_buffer->str);
    }
  }
}

static void
hildon_im_context_commit_preedit_data(HildonIMContext *self)
{
  gchar *prefix_to_commit;
  
  if (self->preedit_buffer != NULL && self->preedit_buffer->len != 0
      && self->client_gtk_widget)
  {
    GtkTextIter iter;
    GtkTextBuffer *buffer;

    if (GTK_IS_TEXT_VIEW (self->client_gtk_widget))
    {
      buffer = get_buffer(self->client_gtk_widget);
      gtk_text_buffer_get_iter_at_mark (buffer, &iter, self->text_view_preedit_mark);
      gtk_text_buffer_place_cursor (buffer, &iter);
    }
    else if (GTK_IS_EDITABLE (self->client_gtk_widget))
    {
      gtk_editable_set_position(GTK_EDITABLE(self->client_gtk_widget),
                                self->editable_preedit_position);
    }

    prefix_to_commit = g_strdup(self->preedit_buffer->str);
    
    if (self->space_after_commit)
    {
      gunichar next_char = get_unichar_by_offset_from_cursor(self, 0);
      
      if (next_char == 0x0 
          || (g_unichar_type(next_char) !=  G_UNICODE_SPACE_SEPARATOR
              && (next_char != 0x09)
              && !g_unichar_ispunct(next_char)))
      {
        g_string_append(self->preedit_buffer, " ");
        self->committed_preedit = TRUE;
      }
    }
    
    if (commit_text(self, self->preedit_buffer->str))
    {
      hildon_im_context_send_committed_preedit (self, prefix_to_commit);
    }
    
    set_preedit_buffer(self, NULL);
    
    g_free(prefix_to_commit);
  }
}

static void
hildon_im_clipboard_copied(HildonIMContext *self)
{
  XEvent ev;
  gint xerror;
  
  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = self->im_window;
  ev.xclient.message_type =
    hildon_im_protocol_get_atom( HILDON_IM_CLIPBOARD_COPIED );
  ev.xclient.format = HILDON_IM_CLIPBOARD_FORMAT;

  gdk_error_trap_push();
  XSendEvent(gdk_x11_get_default_xdisplay (), self->im_window, False, 0, &ev);

  xerror = gdk_error_trap_pop();
  if (xerror)
  {
    if (xerror == BadWindow)
    {
      self->im_window = get_window_id(hildon_im_protocol_get_atom(HILDON_IM_WINDOW));
      ev.xclient.window = self->im_window;
      XSendEvent(gdk_x11_get_default_xdisplay (), self->im_window, False, 0, &ev);
    }
    else
    {
      g_warning("Received the X error %d\n", xerror);
    }
  }
}

static void
hildon_im_clipboard_selection_query(HildonIMContext *self)
{
  XEvent ev;
  gint xerror;
  
  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = self->im_window;
  ev.xclient.message_type =
    hildon_im_protocol_get_atom( HILDON_IM_CLIPBOARD_SELECTION_REPLY );
  ev.xclient.format = HILDON_IM_CLIPBOARD_SELECTION_REPLY_FORMAT;
#ifdef MAEMO_CHANGES
  ev.xclient.data.l[0] = hildon_gtk_im_context_has_selection(GTK_IM_CONTEXT(self));
#endif

  gdk_error_trap_push();
  XSendEvent(gdk_x11_get_default_xdisplay (), self->im_window, False, 0, &ev);

  xerror = gdk_error_trap_pop();
  if (xerror)
  {
    if (xerror == BadWindow)
    {
      self->im_window = get_window_id(hildon_im_protocol_get_atom(HILDON_IM_WINDOW));
      ev.xclient.window = self->im_window;
      XSendEvent(gdk_x11_get_default_xdisplay (), self->im_window, False, 0, &ev);
    }
    else
    {
      g_warning("Received the X error %d\n", xerror);
    }
  }
}

static gboolean
surroundings_search_predicate (gunichar c, gpointer data)
{
  return !g_unichar_isspace(c);
}

static gboolean
get_textview_surrounding(GtkTextView *text_view,
                         GtkTextIter *iter,  /* the beginning of the selection or the location of the cursor */
                         gchar **surrounding,
                         gint *iter_index)
{
  GtkTextIter start;
  GtkTextIter end;
  gint pos;
  gchar *text;
  gchar *text_between = NULL;
  GtkTextBuffer *buffer;

  buffer = get_buffer(GTK_WIDGET(text_view));
  if (iter != NULL)
    end = start = *iter;
  
  gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

  gtk_text_iter_set_line_offset(&start, 0);
  gtk_text_iter_forward_to_line_end (&end);

  /* Include the previous non-whitespace character in the surrounding */
  if (gtk_text_iter_backward_char (&start))
    gtk_text_iter_backward_find_char(&start, surroundings_search_predicate,
                                     NULL, NULL);

  text_between = gtk_text_iter_get_slice(&start, iter);

  if (text_between != NULL)
    pos = g_utf8_strlen(text_between, -1);
  else
    pos = 0;

  text = gtk_text_iter_get_slice(&start, &end);

  *surrounding = text;
  *iter_index = pos;

  g_free(text_between);

  return TRUE;
}

gboolean
hildon_im_context_get_surrounding(GtkIMContext *context,
                                  gchar **text,
                                  gint *cursor_index)
{
  HildonIMContext *self;
  gboolean result = FALSE;

  g_return_val_if_fail(HILDON_IS_IM_CONTEXT(context), FALSE);
  self = HILDON_IM_CONTEXT(context);

  /* Override the textview surrounding handler */
  if (GTK_IS_TEXT_VIEW(self->client_gtk_widget))
  {
    GtkTextIter cursor;
    GtkTextBuffer *buffer;
    
    buffer = get_buffer(self->client_gtk_widget);
    gtk_text_buffer_get_iter_at_mark(buffer, &cursor,
                                    gtk_text_buffer_get_insert(buffer));
    result = get_textview_surrounding(GTK_TEXT_VIEW(self->client_gtk_widget),
                                      &cursor,
                                      text,
                                      cursor_index);
  }
  else
  {
    gchar *local_text = NULL;
    gint local_index = 0;

    result = parent_class->get_surrounding(context,
                                           text ? text : &local_text,
                                           cursor_index ? cursor_index : &local_index);

    if (result)
    {
      g_free(local_text);
    }
  }

  return result;
}

static void
hildon_im_context_commit_surrounding(HildonIMContext *self)
{
  gchar *surrounding;
  gint cpos;
  gboolean has_surrounding;

  has_surrounding = gtk_im_context_get_surrounding (GTK_IM_CONTEXT(self),
                                                    &surrounding, &cpos);
  if (has_surrounding)
  {
    gint offset_start, offset_end;
    gchar *str, *start, *end = NULL;
    /* gboolean deleted; */

    str = &surrounding[cpos];

    start = surrounding;
    for (end = str; end[0] != '\0'; end++);

    if (end-start > 0)
    {
      /* Offset to the start of the line, from the insertion point */
      offset_start = str - start;
      /* Offset to the end of the line */
      offset_end = end - start;

      /* Remove the surrounding context on the line with the cursor */
      /* deleted = */
      gtk_im_context_delete_surrounding(GTK_IM_CONTEXT(self),
                                        -offset_start,
                                        offset_end);
    }
  }

  /* Place the new surrounding context at the insertion point */
  commit_text(self, self->surrounding);

  if (has_surrounding)
  {
    g_free(surrounding);
  }
}

static GSList *
get_pango_attribute_list_for_insert (GtkIMContext *context, gchar *str_to_apply_attr)
{
  GSList *attr_list = NULL, *insert_attr_list = NULL, *list;
  GtkTextIter iter;
  GtkTextMark *insert_mark;
  GtkTextBuffer *buffer;
  HildonIMContext *self;
  
  self = HILDON_IM_CONTEXT(context);

  if (!GTK_IS_TEXT_VIEW (self->client_gtk_widget))
    return NULL;

  buffer = get_buffer (self->client_gtk_widget);
  if (buffer == NULL)
    return NULL;

  insert_mark = gtk_text_buffer_get_insert (buffer);
  gtk_text_buffer_get_iter_at_mark (buffer, &iter, insert_mark);
  /* We backward because we want the iter of the previous char */
  gtk_text_iter_backward_char (&iter);
  insert_attr_list = gtk_text_iter_get_tags (&iter);

  list = insert_attr_list;
  for (; list != NULL; list = g_slist_next (list))
  {
    PangoFontDescription *font_desc = NULL;
    GtkTextTag *tag = GTK_TEXT_TAG (list->data);
    g_object_get (tag, "font-desc", &font_desc, NULL);
    if (font_desc != NULL)
    {
      PangoAttribute *attr;
      attr = pango_attr_font_desc_new (font_desc);
      attr->start_index = 0;
      attr->end_index = strlen (str_to_apply_attr);

      attr_list = g_slist_append (attr_list, attr);
    }
  }
  g_slist_free (insert_attr_list);

  return attr_list;
}

static void
hildon_im_context_get_preedit_string (GtkIMContext *context,
                                      gchar **str,
                                      PangoAttrList **attrs,
                                      gint *cursor_pos)
{
  HildonIMContext *self;
  GSList *insert_attr_list = NULL, *list;
  GtkStyle *style = NULL;
  PangoAttribute *attr1, *attr2, *attr3;
  
  g_return_if_fail(HILDON_IS_IM_CONTEXT(context));
  self = HILDON_IM_CONTEXT(context);
  
  if (cursor_pos != NULL)
    *cursor_pos = 0;
  
  if (str != NULL)
  {
    if (self->preedit_buffer != NULL && self->show_preedit)
    {
      *str = g_strdup (self->preedit_buffer->str);
    }
    else
    {
      *str = g_strdup ("");
    }
  }

  if (attrs != NULL && self->client_gtk_widget != NULL)
  {
    if (GTK_IS_WIDGET(self->client_gtk_widget))
    {
      style = gtk_widget_get_style (self->client_gtk_widget);
    }
    
    if (style == NULL)
    {
      style = gtk_widget_get_default_style ();
    }
    
    attr1 = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
    attr1->start_index = 0;
    attr1->end_index = strlen (*str);
    attr2 = pango_attr_background_new (style->bg[GTK_STATE_SELECTED].red,
                                       style->bg[GTK_STATE_SELECTED].green,
                                       style->bg[GTK_STATE_SELECTED].blue);
    attr2->start_index = 0;
    attr2->end_index = strlen (*str);
    attr3 = pango_attr_foreground_new (style->fg[GTK_STATE_SELECTED].red,
                                       style->fg[GTK_STATE_SELECTED].green,
                                       style->fg[GTK_STATE_SELECTED].blue);
    attr3->start_index = 0;
    attr3->end_index = strlen (*str);
    
    *attrs = pango_attr_list_new ();
    pango_attr_list_insert (*attrs, attr1);
    pango_attr_list_insert (*attrs, attr2);
    pango_attr_list_insert (*attrs, attr3);

    insert_attr_list = get_pango_attribute_list_for_insert (context, *str);
    for (list = insert_attr_list; list != NULL; list = g_slist_next (list))
    {
      pango_attr_list_insert (*attrs, (PangoAttribute *) list->data);
    }
    g_slist_free (insert_attr_list);
  }
  
  return;
}

static void
hildon_im_context_set_use_preedit (GtkIMContext *context,
                                   gboolean use_preedit)
{
  /* TODO something... */
  return;
}

static void
hildon_im_context_set_client_cursor_location(HildonIMContext *self,
                                             gboolean is_relative,
                                             gint offset)
{
  GtkWidget *widget;

  widget = self->client_gtk_widget;

  if (widget != NULL && (GTK_IS_EDITABLE(widget) || GTK_IS_TEXT_VIEW(widget)))
  {
    if (GTK_IS_EDITABLE(widget))
    {
      if (is_relative)
      {
        gtk_editable_set_position(GTK_EDITABLE(widget), 
                                  gtk_editable_get_position(GTK_EDITABLE(widget)) + offset);
      }
      else
      {
        gtk_editable_set_position(GTK_EDITABLE(widget), offset);
      }
    }
    else if (GTK_IS_TEXT_VIEW(widget))
    {
      GtkTextBuffer *buffer;
      GtkTextMark *insert;
      GtkTextIter iter;

      buffer = get_buffer(widget);

      insert = gtk_text_buffer_get_mark(buffer, "insert");
      gtk_text_buffer_get_iter_at_mark(buffer, &iter, insert);

      if (is_relative)
      {
        if (offset == 0)
          gtk_text_buffer_select_range (buffer, &iter, &iter);
        else if (offset >= 0)
          gtk_text_iter_forward_chars(&iter, offset);
        else
          gtk_text_iter_backward_chars(&iter, -offset);
      }
      else
      {
        gtk_text_buffer_get_iter_at_offset(buffer, &iter, offset);
      }

      gtk_text_buffer_place_cursor(buffer, &iter);
    }
  }
}

static void
hildon_im_context_clear_selection(HildonIMContext *self)
{
  if (GTK_IS_TEXT_VIEW(self->client_gtk_widget))
  {
    GtkTextMark *selection, *insert;
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    buffer = get_buffer(self->client_gtk_widget);
    selection = gtk_text_buffer_get_selection_bound(buffer);
    insert = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, insert);
    gtk_text_buffer_move_mark(buffer, selection, &iter);
  }
  else if (GTK_IS_EDITABLE(self->client_gtk_widget))
  {
    gint pos;

    pos = gtk_editable_get_position(GTK_EDITABLE(self->client_gtk_widget));
    gtk_editable_select_region(GTK_EDITABLE(self->client_gtk_widget), pos, pos);
  }
}

static void
hildon_im_context_do_backspace (HildonIMContext *self)
{
  if (self->commit_mode == HILDON_IM_COMMIT_REDIRECT &&
      GTK_IS_TEXT_VIEW(self->client_gtk_widget))
  {
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    buffer = get_buffer(self->client_gtk_widget);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter,
                                     gtk_text_buffer_get_insert(buffer));
    gtk_text_buffer_backspace(buffer, &iter, TRUE, TRUE);
  }
  else if (self->commit_mode == HILDON_IM_COMMIT_REDIRECT &&
           GTK_IS_EDITABLE(self->client_gtk_widget))
  {
    gint position = gtk_editable_get_position(GTK_EDITABLE(self->client_gtk_widget));
    gtk_editable_delete_text(GTK_EDITABLE(self->client_gtk_widget),
                             position - 1, position);
  }
  else
  {
    hildon_im_context_send_fake_key(GDK_BackSpace, TRUE);
    hildon_im_context_send_fake_key(GDK_BackSpace, FALSE);
  }
}

static gboolean
hildon_im_context_do_del (HildonIMContext *self)
{
  gint cpos1;
  gchar *sur;

  /* This is only for non-GTK+ widgets. For normal GTK+ text entries,
     'del' key is handled normally at X level */
  self->last_was_shift_backspace = TRUE;

  if ( (! GTK_IS_TEXT_VIEW (self->client_gtk_widget)) &&
       (! GTK_IS_EDITABLE (self->client_gtk_widget)) )
  {
    gtk_im_context_get_surrounding (GTK_IM_CONTEXT (self), &sur, &cpos1);

    if (g_utf8_strlen (sur, -1) > cpos1)
    {
      hildon_im_context_send_fake_key (GDK_Shift_L, FALSE);
      hildon_im_context_send_fake_key (GDK_Right, TRUE);
      hildon_im_context_send_fake_key (GDK_Right, FALSE);
      hildon_im_context_send_fake_key (GDK_BackSpace, TRUE);
      hildon_im_context_send_fake_key (GDK_Shift_L, TRUE);
    }
    g_free (sur);

    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

/* Filter function to intercept and process XClientMessages */
static GdkFilterReturn
client_message_filter(GdkXEvent *xevent,GdkEvent *event,
                      HildonIMContext *self)
{
  GdkFilterReturn result = GDK_FILTER_CONTINUE;
  XEvent *xe = (XEvent *)xevent;

  g_return_val_if_fail(HILDON_IS_IM_CONTEXT(self), GDK_FILTER_CONTINUE);


  if (xe->type == DestroyNotify)
  { 
    self->client_gdk_window = NULL;
  }
  else if (xe->type == ClientMessage)
  {
    XClientMessageEvent *cme = xevent;

    if (cme->message_type == hildon_im_protocol_get_atom(HILDON_IM_INSERT_UTF8)
        && cme->format == HILDON_IM_INSERT_UTF8_FORMAT)
    {
      HildonIMInsertUtf8Message *msg = (HildonIMInsertUtf8Message *)&cme->data;

      hildon_im_context_insert_utf8( self, msg->msg_flag, msg->utf8_str );
      result = GDK_FILTER_REMOVE;
    }
    else if (cme->message_type == hildon_im_protocol_get_atom(HILDON_IM_COM)
        && cme->format == HILDON_IM_COM_FORMAT)
    {
      HildonIMComMessage *msg = (HildonIMComMessage *)&cme->data;

      /* if autocap was suddenly deactivated, cleanup shift stickiness */
      if ( (! (msg->options & HILDON_IM_AUTOCASE)) &&
           (self->options & HILDON_IM_AUTOCASE) )
        {
          self->mask &= ~HILDON_IM_SHIFT_STICKY_MASK;
          hildon_im_context_send_command (self, HILDON_IM_SHIFT_UNSTICKY);
        }

      self->options = msg->options;

      switch(msg->type)
      {
        case HILDON_IM_CONTEXT_WIDGET_CHANGED:
          self->mask = 0;
          break;
        case HILDON_IM_CONTEXT_ENTER_ON_FOCUS:
          enter_on_focus_pending = TRUE;
          break;
        case HILDON_IM_CONTEXT_CONFIRM_SENTENCE_START:
          hildon_im_context_check_sentence_start(self);
          break;
        case HILDON_IM_CONTEXT_HANDLE_TAB:
          hildon_im_context_send_fake_key(GDK_Tab, TRUE);
          hildon_im_context_send_fake_key(GDK_Tab, FALSE);
          break;
        case HILDON_IM_CONTEXT_HANDLE_BACKSPACE:
          hildon_im_context_do_backspace (self);
          break;
        case HILDON_IM_CONTEXT_HANDLE_SPACE:
          hildon_im_context_insert_utf8(self, HILDON_IM_MSG_CONTINUE, " ");
          break;
        case HILDON_IM_CONTEXT_BUFFERED_MODE:
          set_preedit_buffer (self, NULL);
          self->commit_mode = HILDON_IM_COMMIT_BUFFERED;
          break;
        case HILDON_IM_CONTEXT_DIRECT_MODE:
          set_preedit_buffer (self, NULL);
          self->commit_mode = HILDON_IM_COMMIT_DIRECT;
          break;
        case HILDON_IM_CONTEXT_REDIRECT_MODE:
          set_preedit_buffer (self, NULL);
          self->commit_mode = HILDON_IM_COMMIT_REDIRECT;
          hildon_im_context_clear_selection(self);
          break;
        case HILDON_IM_CONTEXT_SURROUNDING_MODE:
          set_preedit_buffer (self, NULL);
          self->commit_mode = HILDON_IM_COMMIT_SURROUNDING;
          break;
        case HILDON_IM_CONTEXT_PREEDIT_MODE:
          set_preedit_buffer (self, NULL);
          /* Preedit is a temporary mode that will be reset after
           * the next text has been received.
           */
          self->previous_commit_mode = self->commit_mode;
          self->commit_mode = HILDON_IM_COMMIT_PREEDIT;
          break;
        case HILDON_IM_CONTEXT_REQUEST_SURROUNDING:
          hildon_im_context_send_surrounding(self, FALSE);
          if (self->is_url_entry)
          {
            hildon_im_context_send_command(self, HILDON_IM_SELECT_ALL);
          }
          break;
        case HILDON_IM_CONTEXT_REQUEST_SURROUNDING_FULL:
          hildon_im_context_send_surrounding(self, TRUE);
          if (self->is_url_entry)
          {
            hildon_im_context_send_command(self, HILDON_IM_SELECT_ALL);
          }
          break;
        case HILDON_IM_CONTEXT_FLUSH_PREEDIT:
          hildon_im_context_commit_preedit_data(self);
          break;
        case HILDON_IM_CONTEXT_CANCEL_PREEDIT:
          set_preedit_buffer (self, NULL);
          break;
#ifdef MAEMO_CHANGES
        case HILDON_IM_CONTEXT_CLIPBOARD_COPY:
            hildon_gtk_im_context_copy(GTK_IM_CONTEXT(self));
          break;
        case HILDON_IM_CONTEXT_CLIPBOARD_CUT:
            hildon_gtk_im_context_cut(GTK_IM_CONTEXT(self));
          break;
        case HILDON_IM_CONTEXT_CLIPBOARD_PASTE:
            hildon_gtk_im_context_paste(GTK_IM_CONTEXT(self));
          break;
#elif GTK_CHECK_VERSION(3,0,0)
        case HILDON_IM_CONTEXT_CLIPBOARD_COPY:
        {
          if (GTK_IS_ENTRY(self->client_gtk_widget))
            g_signal_emit_by_name(self, "copy-clipboard", 0);

          break;
        }
        case HILDON_IM_CONTEXT_CLIPBOARD_CUT:
        {
          if (GTK_IS_ENTRY(self->client_gtk_widget))
            g_signal_emit_by_name(self, "cut-clipboard", 0);

          break;
        }
        case HILDON_IM_CONTEXT_CLIPBOARD_PASTE:
        {
          if (GTK_IS_ENTRY(self->client_gtk_widget))
            g_signal_emit_by_name(self, "paste-clipboard", 0);

          break;
        }
#endif
        case HILDON_IM_CONTEXT_CLIPBOARD_SELECTION_QUERY:
            hildon_im_clipboard_selection_query(self);
          break;
        case HILDON_IM_CONTEXT_OPTION_CHANGED:
          break;
        case HILDON_IM_CONTEXT_SPACE_AFTER_COMMIT:
          self->space_after_commit = TRUE;
          break;
        case HILDON_IM_CONTEXT_NO_SPACE_AFTER_COMMIT:
          self->space_after_commit = FALSE;
          break;
        case HILDON_IM_CONTEXT_SHIFT_LOCKED:
          self->mask |= HILDON_IM_SHIFT_LOCK_MASK;
          break;
        case HILDON_IM_CONTEXT_SHIFT_UNLOCKED:
          self->mask &= ~HILDON_IM_SHIFT_LOCK_MASK;
        case HILDON_IM_CONTEXT_SHIFT_UNSTICKY:
          self->mask &= ~HILDON_IM_SHIFT_STICKY_MASK;
          break;
        case HILDON_IM_CONTEXT_LEVEL_LOCKED:
          self->mask |= HILDON_IM_LEVEL_LOCK_MASK;
          break;
        case HILDON_IM_CONTEXT_LEVEL_UNLOCKED:
          self->mask &= ~HILDON_IM_LEVEL_LOCK_MASK;
        case HILDON_IM_CONTEXT_LEVEL_UNSTICKY:
          self->mask &= ~HILDON_IM_LEVEL_STICKY_MASK;
          break;
        default:
          g_warning("Invalid communication message from IM");
          break;
      }
      result = GDK_FILTER_REMOVE;
    }
    else if (cme->message_type == hildon_im_protocol_get_atom(HILDON_IM_SURROUNDING_CONTENT)
        && cme->format == HILDON_IM_SURROUNDING_CONTENT_FORMAT)
    {
      HildonIMSurroundingContentMessage *msg =
        (HildonIMSurroundingContentMessage *)&cme->data;
      gchar *new_surrounding;

      if (msg->msg_flag == HILDON_IM_MSG_START && self->surrounding)
      {
        g_free(self->surrounding);
        self->surrounding = g_strdup("");
      }

      if (msg->msg_flag == HILDON_IM_MSG_END && self->surrounding)
      {
        hildon_im_context_commit_surrounding(self);
        result = GDK_FILTER_REMOVE;
      }
      else
      {
        new_surrounding = g_strconcat(self->surrounding,
                                      msg->surrounding, NULL);
        
        g_free(self->surrounding);
        self->surrounding = new_surrounding;

        result = GDK_FILTER_REMOVE;
      }
    }
    else if (cme->message_type == hildon_im_protocol_get_atom(HILDON_IM_SURROUNDING)
        && cme->format == HILDON_IM_SURROUNDING_FORMAT)
    {
      HildonIMSurroundingMessage *msg =
        (HildonIMSurroundingMessage *)&cme->data;

      hildon_im_context_set_client_cursor_location(self,
                                                   msg->offset_is_relative,
                                                   msg->cursor_offset);
      result = GDK_FILTER_REMOVE;

    }
    else if (cme->message_type ==
             hildon_im_protocol_get_atom (HILDON_IM_LONG_PRESS_SETTINGS))
    {
      HildonIMLongPressSettingsMessage *msg =
        (HildonIMLongPressSettingsMessage *) &cme->data;

      self->enable_long_press = msg->enable_long_press;
      self->long_press_timeout = (msg->long_press_timeout > 0)
        ? msg->long_press_timeout : DEFAULT_LONG_PRESS_TIMEOUT;

      result = GDK_FILTER_REMOVE;
    }
  }
  
  return result;
}
/* Virtual functions */

static void
hildon_im_context_widget_changed(HildonIMContext *self)
{
  /* Count how many "changed" signals we get for
     hildon_im_context_insert_utf8() */
  self->changed_count++;
}

static void
hildon_im_context_widget_hide(GtkIMContext *context)
{
  HildonIMContext *self;

  g_return_if_fail(HILDON_IS_IM_CONTEXT(context));
  self = HILDON_IM_CONTEXT(context);

  if (self->client_gtk_widget &&
      gtk_widget_has_focus(self->client_gtk_widget))
  {
    hildon_im_context_hide(context);
  }
}

static void
hildon_im_context_widget_copy_clipboard(GtkIMContext *context)
{
  HildonIMContext *self;
  gboolean copied = FALSE;

  g_return_if_fail(HILDON_IS_IM_CONTEXT(context));
  self = HILDON_IM_CONTEXT(context);

  if (self->client_gtk_widget)
  {
    if (GTK_IS_EDITABLE(self->client_gtk_widget))
    {
      copied = gtk_editable_get_selection_bounds(
        GTK_EDITABLE(self->client_gtk_widget), NULL, NULL);
    }
    else if (GTK_IS_TEXT_VIEW(self->client_gtk_widget))
    {
      GtkTextBuffer *buffer;

      buffer = get_buffer(self->client_gtk_widget);
      copied = gtk_text_buffer_get_selection_bounds(buffer,
                                                    NULL, NULL);
    }
  }

  if (copied)
  {
    hildon_im_clipboard_copied(HILDON_IM_CONTEXT(context));
  }
}

/* Register the client widgets GdkWindow where the input will appear */
static void
hildon_im_context_set_client_window(GtkIMContext *context,
                                    GdkWindow *window)
{
  HildonIMContext *self;

  g_return_if_fail( HILDON_IS_IM_CONTEXT( context ) );
  self = HILDON_IM_CONTEXT(context);

  if (self->client_changed_signal_handler > 0)
  {
    g_signal_handler_disconnect(self->client_gtk_widget,
                                self->client_changed_signal_handler);
    self->client_changed_signal_handler = 0;
  }

  if (self->client_hide_signal_handler > 0)
  {
    g_signal_handler_disconnect(self->client_gtk_widget,
                                self->client_hide_signal_handler);
    self->client_hide_signal_handler = 0;
  }

  if (self->client_copy_clipboard_signal_handler > 0)
  {
    g_signal_handler_disconnect(self->client_gtk_widget,
                                self->client_copy_clipboard_signal_handler);
    self->client_copy_clipboard_signal_handler = 0;
  }

  if (self->client_gdk_window != NULL) {
    /* Need to clean up old window unhook gdk_event_filter etc */
    gdk_window_remove_filter(self->client_gdk_window,
                             (GdkFilterFunc)client_message_filter, self );

    if (window == NULL && !self->is_internal_widget)
    {
      hildon_im_context_send_command(self, HILDON_IM_HIDE);
    }
  }

  self->is_url_entry = FALSE;
  self->committed_preedit = FALSE;
  self->client_gdk_window = window;
  self->client_gtk_widget = NULL;
  self->text_view_preedit_mark = NULL;
  self->editable_preedit_position = 0;
  self->last_key_event = NULL;
  self->mask = 0;

  if (window)
  {
    /* Filter the window for ClientMessages*/
    gpointer widget;

    gdk_window_add_filter(window, (GdkFilterFunc)client_message_filter, self);

    /* If the widget contents change, we want to know about it. */
    gdk_window_get_user_data(window, &widget);
    if (widget != NULL)
    {
      self->client_gtk_widget = widget;

      if (GTK_IS_WIDGET(widget))
      {
        self->client_hide_signal_handler = g_signal_connect_swapped(widget,
               "hide", G_CALLBACK(hildon_im_context_widget_hide), self);

        if (GTK_IS_ENTRY(widget) || GTK_IS_TEXT_VIEW(widget))
        {
          self->client_copy_clipboard_signal_handler =
            g_signal_connect_swapped(widget, "copy-clipboard",
              G_CALLBACK(hildon_im_context_widget_copy_clipboard), self);
        }

        if (GTK_IS_TEXT_VIEW(widget))
        {
          GtkTextIter start;
          gtk_text_buffer_get_start_iter (get_buffer(widget),
                                          &start);
          self->text_view_preedit_mark = gtk_text_buffer_create_mark (
                                get_buffer(widget),
                                "preedit", &start, FALSE);
        }

        /* The commit mode depends on the type of widget */
        if (GTK_IS_TEXT_VIEW(self->client_gtk_widget) ||
            GTK_IS_EDITABLE(self->client_gtk_widget))
        {
          self->commit_mode = HILDON_IM_COMMIT_REDIRECT;
        }
        else
        {
          /* Other widgets (direct IM implementations like browsers)
               are edited in the fullscreen mode and then commited all at once */
          self->commit_mode = HILDON_IM_COMMIT_SURROUNDING;
        }

#ifndef MAEMO_CHANGES
        g_signal_connect(self->client_gtk_widget, "button-press-event",
            G_CALLBACK(button_press_release), self);
        g_signal_connect(self->client_gtk_widget, "button-release-event",
            G_CALLBACK(button_press_release), self);
#endif
      }

      if (g_strcmp0(gtk_widget_get_name(widget), HILDON_IM_INTERNAL_TEXTVIEW) == 0)
      {
        self->is_internal_widget = TRUE;
      }
      else if (g_strcmp0 (gtk_widget_get_name(widget), MAEMO_BROWSER_URL_ENTRY) == 0)
      {
        self->is_url_entry = TRUE;
      }

      if (GTK_IS_EDITABLE(widget))
      {
        self->client_changed_signal_handler = g_signal_connect_swapped(widget,
                "changed", G_CALLBACK(hildon_im_context_widget_changed), self);
      }

#if !GTK_CHECK_VERSION(3,0,0)
      gtk_widget_set_extension_events(self->client_gtk_widget,
                                      GDK_EXTENSION_EVENTS_ALL);
#endif
    }
  }
}

static void
hildon_im_context_focus_in(GtkIMContext *context)
{
  HildonIMContext *self;
  g_return_if_fail(HILDON_IS_IM_CONTEXT(context));
  self = HILDON_IM_CONTEXT(context);

  self->has_focus = TRUE;

  if (self->is_internal_widget)
  {
    return;
  }

#if GTK_CHECK_VERSION(3,0,0)
  current_context = context;
#endif

  hildon_im_context_send_command(self, HILDON_IM_SETCLIENT);

  hildon_im_context_send_command (self, HILDON_IM_SHIFT_UNSTICKY);
  hildon_im_context_send_command (self, HILDON_IM_MOD_UNSTICKY);

  if (enter_on_focus_pending)
  {
    hildon_im_context_send_fake_key(GDK_KP_Enter, TRUE);
    hildon_im_context_send_fake_key(GDK_KP_Enter, FALSE);
    enter_on_focus_pending = FALSE;
  }
}

static void
hildon_im_context_focus_out(GtkIMContext *context)
{
  HildonIMContext *self;

  g_return_if_fail(HILDON_IS_IM_CONTEXT(context));
  self = HILDON_IM_CONTEXT(context);

#if GTK_CHECK_VERSION(3,0,0)
  current_context = NULL;
#endif

  self->has_focus = FALSE;

  set_preedit_buffer (self, NULL);

  /* clear any long-press data */
  hildon_im_context_abort_long_press (self);
  if (self->long_press_last_key_event != NULL)
  {
    gdk_event_free ((GdkEvent *) self->long_press_last_key_event);
    self->long_press_last_key_event = NULL;
  }
}

static gboolean
hildon_im_context_font_has_char(HildonIMContext *self, guint32 c)
{
  PangoLayout *layout = NULL;
  gboolean has_char;
  cairo_t *cr;
  gchar utf8[7];
  gint len;

  g_return_val_if_fail(HILDON_IS_IM_CONTEXT(self), TRUE);
  g_return_val_if_fail(self->client_gtk_widget &&
                       gtk_widget_get_window(self->client_gtk_widget), TRUE);

  cr = gdk_cairo_create(gtk_widget_get_window(self->client_gtk_widget));
  len = g_unichar_to_utf8(c, utf8);
  layout = pango_cairo_create_layout(cr);
  pango_layout_set_text(layout, utf8, len);
  has_char = pango_layout_get_unknown_glyphs_count(layout) == 0;

  g_object_unref(layout);
  cairo_destroy(cr);

  return has_char;
}

static guint32
dead_key_to_unicode_combining_character(guint keyval)
{
  guint32 combining;

  switch (keyval)
  {
    case GDK_dead_grave:            combining = 0x0300; break;
    case GDK_dead_acute:            combining = 0x0301; break;
    case GDK_dead_circumflex:       combining = 0x0302; break;
    case GDK_dead_tilde:            combining = 0x0303; break;
    case GDK_dead_macron:           combining = 0x0304; break;
    case GDK_dead_breve:            combining = 0x032e; break;
    case GDK_dead_abovedot:         combining = 0x0307; break;
    case GDK_dead_diaeresis:        combining = 0x0308; break;
    case GDK_dead_abovering:        combining = 0x030a; break;
    case GDK_dead_doubleacute:      combining = 0x030b; break;
    case GDK_dead_caron:            combining = 0x030c; break;
    case GDK_dead_cedilla:          combining = 0x0327; break;
    case GDK_dead_ogonek:           combining = 0x0328; break;
    case GDK_dead_iota:             combining = 0; break; /* Cannot be combined */
    case GDK_dead_voiced_sound:     combining = 0; break; /* Cannot be combined */
    case GDK_dead_semivoiced_sound: combining = 0; break; /* Cannot be combined */
    case GDK_dead_belowdot:         combining = 0x0323; break;
    case GDK_dead_hook:             combining = 0x0309; break;
    case GDK_dead_horn:             combining = 0x031b; break;
    default: combining = 0; break; /* Unknown dead key */
  }

  return combining;
}

static guint32
combining_character_to_unicode (guint32 combining)
{
  guint32 unicode;

  switch (combining)
  {
    case 0x0300: unicode = 0x0060; break;
    case 0x0301: unicode = 0x00b4; break;
    case 0x0302: unicode = 0x005e; break;
    case 0x0303: unicode = 0x007e; break;
    case 0x0304: unicode = 0x00af; break;
    case 0x0307: unicode = 0x02d9; break;
    case 0x0308: unicode = 0x00a8; break;
    case 0x0309: unicode = 0x0294; break;
    case 0x030a: unicode = 0x00b0; break;
    case 0x030b: unicode = 0x0022; break;
    case 0x030c: unicode = 0x02c7; break;
    case 0x031b: unicode = 0x031b; break;
    case 0x0323: unicode = 0x02d4; break;
    case 0x0327: unicode = 0x00b8; break;
    case 0x0328: unicode = 0x02db; break;
    case 0x032e: unicode = 0x032e; break;
    default: unicode = 0; break; /* Unknown combining char */
  }

  return unicode;
}

static guint
hildon_im_context_get_event_keyval_for_level(HildonIMContext *self,
                                             GdkEventKey *event,
                                             gint level)
{
  guint keyval;
  GdkKeymapKey *keys = NULL;
  guint *keyvals = NULL;
  gint n_entries;
  gboolean has_entries;

  has_entries = gdk_keymap_get_entries_for_keycode(gdk_keymap_get_default(),
                                                   event->hardware_keycode,
                                                   &keys,
                                                   &keyvals,
                                                   &n_entries);
  if (has_entries && level < n_entries &&
      keys[level].group == event->group)
  {
    GdkKeymapKey k;

    k.keycode = event->hardware_keycode;
    k.group = event->group;
    k.level = level;

    keyval = gdk_keymap_lookup_key(gdk_keymap_get_default(), &k);
  }
  else
  {
    keyval = event->keyval;
  }

  if (keys)
    g_free(keys);
  if (keyvals)
    g_free(keyvals);

  return keyval;
}

static void
hildon_im_context_set_mask_state(HildonIMContext *self,
                                 HildonIMInternalModifierMask *mask,
                                 HildonIMInternalModifierMask lock_mask,
                                 HildonIMInternalModifierMask sticky_mask,
                                 gboolean was_press_and_release)
{
#if defined(MAEMO_CHANGES) || GTK_CHECK_VERSION(3,0,0)
  HildonGtkInputMode input_mode = 0;
  hildon_get_input_mode (self, &input_mode, NULL);

  /* Locking Fn is disabled in TELE and NUMERIC */
  if (!(input_mode & HILDON_GTK_INPUT_MODE_ALPHA) &&
      !(input_mode & HILDON_GTK_INPUT_MODE_HEXA)  &&
      ((input_mode & HILDON_GTK_INPUT_MODE_TELE) || 
       (input_mode & HILDON_GTK_INPUT_MODE_NUMERIC)))
  {
    if (*mask & lock_mask)
    {
      /* already locked, remove lock and set it to sticky */
      *mask &= ~(lock_mask | sticky_mask);
      *mask |= sticky_mask;
    }
    else if (*mask & sticky_mask)
    {
      /* the key is already sticky, it's fine */
    }
    else if (was_press_and_release)
    {
      /* Pressing the key for the first time stickies the key for one character,
       * but only if no characters were entered while holding the key down */
      *mask |= sticky_mask;
    }
    return;
  }
#endif
  if (*mask & lock_mask)
  {
    /* Pressing the key while already locked clears the state */
    if (lock_mask & HILDON_IM_SHIFT_LOCK_MASK)
      hildon_im_context_send_command(self, HILDON_IM_SHIFT_UNLOCKED);
    else if (lock_mask & HILDON_IM_LEVEL_LOCK_MASK)
      hildon_im_context_send_command(self, HILDON_IM_MOD_UNLOCKED);    
    
    *mask &= ~(lock_mask | sticky_mask);
  }
  else if (*mask & sticky_mask)
  {
    /* When the key is already sticky, a second press locks the key */
    *mask |= lock_mask;

    if (lock_mask & HILDON_IM_SHIFT_LOCK_MASK)
      hildon_im_context_send_command(self, HILDON_IM_SHIFT_LOCKED);
    else if (lock_mask & HILDON_IM_LEVEL_LOCK_MASK)
      hildon_im_context_send_command(self, HILDON_IM_MOD_LOCKED);
  }
  else if (was_press_and_release)
  {
    /* Pressing the key for the first time stickies the key for one character,
     * but only if no characters were entered while holding the key down */
    *mask |= sticky_mask;
    if (sticky_mask & HILDON_IM_SHIFT_STICKY_MASK)
      hildon_im_context_send_command (self, HILDON_IM_SHIFT_STICKY);
    else if (sticky_mask & HILDON_IM_LEVEL_STICKY_MASK)
      hildon_im_context_send_command (self, HILDON_IM_MOD_STICKY);
  }
  
}

static void
perform_level_translation (GdkEventKey *event, GdkModifierType state)
{
  guint translated_keyval;

  gdk_keymap_translate_keyboard_state(gdk_keymap_get_default(),
                                      event->hardware_keycode,
                                      state,
                                      event->group,
                                      &translated_keyval,
                                      NULL, NULL, NULL);
  event->keyval = translated_keyval;
}

static void
reset_shift_and_level_keys_if_needed (HildonIMContext *context, GdkEventKey *event)
{
  if (event->is_modifier)
    return;

  /* If not locked, pressing any character resets shift state */
  if (event->keyval != GDK_Shift_L && event->keyval != GDK_Shift_R &&
      (context->mask & HILDON_IM_SHIFT_LOCK_MASK) == 0)
  {
    context->mask &= ~HILDON_IM_SHIFT_STICKY_MASK;
    hildon_im_context_send_command (context, HILDON_IM_SHIFT_UNSTICKY);
  }
  /* If not locked, pressing any character resets level state */
  if (event->keyval != LEVEL_KEY && (context->mask & HILDON_IM_LEVEL_LOCK_MASK) == 0)
  {
    context->mask &= ~HILDON_IM_LEVEL_STICKY_MASK;
    hildon_im_context_send_command (context, HILDON_IM_MOD_UNSTICKY);
  }
}

static gboolean
key_released (HildonIMContext *context, GdkEventKey *event, guint last_keyval)
{
  gboolean level_key_is_sticky = context->mask & HILDON_IM_LEVEL_STICKY_MASK;
  gboolean level_key_is_locked = context->mask & HILDON_IM_LEVEL_LOCK_MASK;
  gboolean level_key_is_down = event->state & LEVEL_KEY_MOD_MASK;

  if ((context->long_press_last_key_event != NULL) &&
      (event->hardware_keycode ==
       context->long_press_last_key_event->hardware_keycode))
  {
    hildon_im_context_abort_long_press (context);
    gdk_event_free ((GdkEvent *) context->long_press_last_key_event);
    context->long_press_last_key_event = NULL;
  }

  if (event->keyval == COMPOSE_KEY)
      context->mask &= ~HILDON_IM_COMPOSE_MASK;

  if (event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R)
  {
    if (! context->last_was_shift_backspace)
      hildon_im_context_set_mask_state(context,
                                       &context->mask,
                                       HILDON_IM_SHIFT_LOCK_MASK,
                                       HILDON_IM_SHIFT_STICKY_MASK,
                                       last_keyval == GDK_Shift_L || last_keyval == GDK_Shift_R);
    else
      context->last_was_shift_backspace = FALSE;
  }
  else if (event->keyval == LEVEL_KEY)
  {
    hildon_im_context_set_mask_state(context,
                                     &context->mask,
                                     HILDON_IM_LEVEL_LOCK_MASK,
                                     HILDON_IM_LEVEL_STICKY_MASK,
                                     last_keyval == LEVEL_KEY);
  }

  if (level_key_is_sticky || level_key_is_locked || level_key_is_down)
  {
    GdkModifierType state = LEVEL_KEY_MOD_MASK;

    if ((context->mask & HILDON_IM_SHIFT_LOCK_MASK) != 0 ||
        (context->mask & HILDON_IM_SHIFT_STICKY_MASK) != 0)
    {
      state |= GDK_SHIFT_MASK;
    }

    perform_level_translation (event, state);

    if (event->keyval == COMPOSE_KEY)
      context->mask &= ~HILDON_IM_COMPOSE_MASK;
  }

  hildon_im_context_check_sentence_start (context);

  hildon_im_context_send_key_event(context, event->type, event->state,
                                   event->keyval, event->hardware_keycode);

  reset_shift_and_level_keys_if_needed (context, event);

  return FALSE;
}

static void
hildon_im_context_invert_case (GdkEventKey *event)
{
  guint lower, upper;

  gdk_keyval_convert_case (event->keyval, &lower, &upper);

  if (gdk_keyval_is_upper (event->keyval))
  {
    event->keyval = lower;
  }
  else
  {
    event->keyval = upper;
  }
}

static void
perform_shift_translation (GdkEventKey *event, GdkModifierType state)
{
  guint lower, upper;

  gdk_keyval_convert_case(event->keyval, &lower, &upper);
  /* Simulate shift key being held down in sticky state for non-printables  */
  if (lower == upper)
  {
    gdk_keymap_translate_keyboard_state(gdk_keymap_get_default(),
                                        event->hardware_keycode,
                                        state,
                                        event->group,
                                        &event->keyval,
                                        NULL, NULL, NULL);
  }
  /* For printable characters sticky shift negates the case,
     including any autocapitalization changes */
  else
  {
    hildon_im_context_invert_case (event);
  }
}

static gboolean
client_has_selection (HildonIMContext *context)
{
  if (GTK_IS_TEXT_VIEW (context->client_gtk_widget))
  {
    GtkTextBuffer *buffer;
    buffer = get_buffer (context->client_gtk_widget);
    return gtk_text_buffer_get_has_selection (buffer);
  }
  if (GTK_IS_EDITABLE (context->client_gtk_widget))
  {
    return gtk_editable_get_selection_bounds (GTK_EDITABLE (context->client_gtk_widget),
                                              NULL, NULL);
  }
  return FALSE;
}

static gboolean
insert_text (HildonIMContext *context, gchar *text, gint offset)
{
  if (text == NULL || !strlen (text))
    return FALSE;

  if (GTK_IS_EDITABLE (context->client_gtk_widget))
  {
    gint cursor = gtk_editable_get_position (GTK_EDITABLE (context->client_gtk_widget));
    cursor += offset;
    gtk_editable_insert_text (GTK_EDITABLE (context->client_gtk_widget),
                              text,
                              -1,
                              &cursor);
  }
  else if (GTK_IS_TEXT_VIEW (context->client_gtk_widget))
  {
    GtkTextIter iter;
    GtkTextBuffer *buffer = get_buffer(context->client_gtk_widget);
    if (buffer == NULL)
      return FALSE;
    gtk_text_buffer_get_iter_at_mark (buffer,
                                      &iter,
                                      gtk_text_buffer_get_insert(buffer));
    gtk_text_iter_forward_chars (&iter, offset);
    gtk_text_buffer_insert (buffer, &iter, text, -1);
  }

  return TRUE;
}

static guint32
compose_character (HildonIMContext *context, guint32 character, guint32 combining_character)
{
  guint32 composed_char = character, tmp_char;
  gchar combined[12], *composed;
  gint base_length, combinator_length;

  base_length = g_unichar_to_utf8(character, combined);
  combinator_length = g_unichar_to_utf8(combining_character, &combined[base_length]);

  composed = g_utf8_normalize(combined,
                              base_length + combinator_length,
                              G_NORMALIZE_DEFAULT_COMPOSE);

  /* Prevent composing of characters that are valid,
   * but not available in the font used */
  tmp_char = g_utf8_get_char (composed);
  if (hildon_im_context_font_has_char(context, tmp_char))
    composed_char = tmp_char;

  g_free(composed);

  context->combining_char = 0;

  return composed_char;
}

static guint32
get_representation_for_dead_character (GdkEventKey *event, guint32 dead_character)
{
  gint32 last;
  last = dead_key_to_unicode_combining_character (event->keyval);

  if ((last == dead_character) || event->keyval == GDK_space)
    return combining_character_to_unicode (dead_character);

  return gdk_keyval_to_unicode (event->keyval);
}

static void
hildon_im_context_delete_penultimate_char (HildonIMContext *self)
{
  if (GTK_IS_TEXT_VIEW (self->client_gtk_widget))
  {
    GtkTextBuffer *buffer;
    GtkTextIter iter1, iter2;

    buffer = get_buffer (self->client_gtk_widget);
    gtk_text_buffer_get_iter_at_mark (buffer,
                                      &iter1,
                                      gtk_text_buffer_get_insert (buffer));
    gtk_text_iter_backward_char (&iter1);
    iter2 = iter1;
    gtk_text_iter_backward_char (&iter1);
    gtk_text_buffer_delete (buffer,
                            &iter1, &iter2);
  }
  else if (GTK_IS_EDITABLE(self->client_gtk_widget))
  {
    gint pos;

    pos = gtk_editable_get_position (GTK_EDITABLE (self->client_gtk_widget));
    gtk_editable_delete_text (GTK_EDITABLE(self->client_gtk_widget),
                              pos - 2,
                              pos - 1);
  }
  else
  {
    hildon_im_context_send_fake_key (GDK_Left, TRUE);
    hildon_im_context_send_fake_key (GDK_Left, FALSE);

    hildon_im_context_send_fake_key (GDK_BackSpace, TRUE);
    hildon_im_context_send_fake_key (GDK_BackSpace, FALSE);

    hildon_im_context_send_fake_key (GDK_Right, TRUE);
    hildon_im_context_send_fake_key (GDK_Right, FALSE);
  }
}

static gboolean
hildon_im_context_on_long_press_timeout (gpointer user_data)
{
  HildonIMContext *self = HILDON_IM_CONTEXT (user_data);
  HildonIMInternalModifierMask mask_backup;
  gint cpos1 = 0, cpos2 = 0;

  mask_backup = self->mask;

  if ((self->mask & HILDON_IM_LEVEL_LOCK_MASK) != 0)
  {
    self->mask &= ~ (HILDON_IM_LEVEL_LOCK_MASK | HILDON_IM_LEVEL_STICKY_MASK);

    perform_level_translation (self->long_press_last_key_event,
                               GDK_SHIFT_MASK);
  }
  else
  {
    self->mask |= HILDON_IM_LEVEL_STICKY_MASK;
  }

  gtk_im_context_get_surrounding (GTK_IM_CONTEXT (self), NULL, &cpos1);

  self->enable_long_press = FALSE;
  key_pressed (self, self->long_press_last_key_event);
  self->enable_long_press = TRUE;

  gtk_im_context_get_surrounding (GTK_IM_CONTEXT (self), NULL, &cpos2);

  if ( (cpos2 > cpos1) ||
       (! GTK_IS_EDITABLE (self->client_gtk_widget)) ||
       (! GTK_IS_TEXT_VIEW (self->client_gtk_widget)) )
    {
      hildon_im_context_delete_penultimate_char (self);
    }

  self->mask = mask_backup;

  self->long_press_timeout_src_id = 0;

  return FALSE;
}

static void
hildon_im_context_abort_long_press (HildonIMContext *context)
{
  if (context->long_press_timeout_src_id != 0)
  {
    g_source_remove (context->long_press_timeout_src_id);
    context->long_press_timeout_src_id = 0;
  }
}

/*
 * GTK3 added couple of control chars to be supported by
 * gdk_keyval_to_unicode(), ignore them
 */

static gboolean
is_ascii_control_char(guint32 c)
{
#if GTK_CHECK_VERSION(3,0,0)
  return
      c == '\b' || c == '\t' || c == '\n' ||
      c == '\v' || c == '\r' || c == '\033';
#else
  return FALSE;
#endif
}

static gboolean
key_pressed (HildonIMContext *context, GdkEventKey *event)
{
#if defined(MAEMO_CHANGES) || GTK_CHECK_VERSION(3,0,0)
  HildonGtkInputMode input_mode, default_input_mode;
#endif

  guint32 c = 0;

  /* We manually lower the case because when shift is down
   * the event will be upper case and that will be
   * inconsistent with the rest of the shift states*/
  event->keyval = gdk_keyval_to_lower (event->keyval);

  gboolean is_suggesting_autocompleted_word;

  gboolean shift_key_is_down = event->state & GDK_SHIFT_MASK;
  gboolean shift_key_is_locked = context->mask & HILDON_IM_SHIFT_LOCK_MASK;
  gboolean shift_key_is_sticky = context->mask & HILDON_IM_SHIFT_STICKY_MASK;

  gboolean level_key_is_sticky = context->mask & HILDON_IM_LEVEL_STICKY_MASK;
  gboolean level_key_is_locked = context->mask & HILDON_IM_LEVEL_LOCK_MASK;
  gboolean level_key_is_down = event->state & LEVEL_KEY_MOD_MASK;

  /*gboolean enter_key_is_down = event->keyval == GDK_Return   ||
                               event->keyval == GDK_KP_Enter ||
                               event->keyval == GDK_ISO_Enter;*/

  gboolean ctrl_key_is_down = event->state & GDK_CONTROL_MASK;

  gboolean tab_key_is_down = event->keyval == GDK_Tab;

  gboolean invert_level_behavior = FALSE;
  
  GdkModifierType translation_state = 0;

  /* avoid key repeating when a long-press is in course  */
  if ((context->enable_long_press) &&
      (context->long_press_last_key_event != NULL))
  {
    if (event->hardware_keycode !=
        context->long_press_last_key_event->hardware_keycode)
    {
      hildon_im_context_abort_long_press (context);
    }
    else
    {
      return TRUE;
    }
  }

  if (event->keyval == GDK_Delete)
    if (hildon_im_context_do_del (context))
      return TRUE;

  is_suggesting_autocompleted_word = context->preedit_buffer != NULL &&
                                     context->preedit_buffer->len != 0;

  if (event->keyval == COMPOSE_KEY)
    context->mask |= HILDON_IM_COMPOSE_MASK;

  if (ctrl_key_is_down)
  {
    hildon_im_context_send_key_event(context, event->type, event->state,
                                    event->keyval, event->hardware_keycode);
    return FALSE;
  }

  /* word completion manipulation */
  if (is_suggesting_autocompleted_word)
  {
    if (event->keyval == GDK_Right)
    {
      hildon_im_context_commit_preedit_data(context);
      return TRUE;
    }
    else
    {
      set_preedit_buffer(context, NULL);
      context->committed_preedit = FALSE;
      
      if (event->keyval == GDK_BackSpace || event->keyval == GDK_Left)
      {
        return TRUE;
      }
    }
  }

  if (tab_key_is_down && GTK_IS_ENTRY (context->client_gtk_widget))
  {
    hildon_im_gtk_focus_next_text_widget(context->client_gtk_widget,
                                         ctrl_key_is_down ? GTK_DIR_TAB_FORWARD :
                                                            GTK_DIR_TAB_BACKWARD);
    context->committed_preedit = FALSE;
    return TRUE;
  }

#if defined(MAEMO_CHANGES) || GTK_CHECK_VERSION(3,0,0)
  hildon_get_input_mode (context, &input_mode, &default_input_mode);
  /* If the input mode is TELE, the behavior of the level key is inverted */
  gboolean should_invert = default_input_mode == HILDON_GTK_INPUT_MODE_NUMERIC;
  if (!should_invert)
  {
    should_invert = (input_mode & HILDON_GTK_INPUT_MODE_ALPHA) == 0  &&
                    (input_mode & HILDON_GTK_INPUT_MODE_HEXA)  == 0  &&
                    ((input_mode & HILDON_GTK_INPUT_MODE_TELE) ||
                    (input_mode & HILDON_GTK_INPUT_MODE_SPECIAL));
  }
  if ((context->options & HILDON_IM_AUTOLEVEL_NUMERIC &&
      (input_mode & HILDON_GTK_INPUT_MODE_FULL) == HILDON_GTK_INPUT_MODE_NUMERIC)
      || should_invert)
  {
    invert_level_behavior = TRUE;
  }
#endif

  if (context->options & HILDON_IM_LOCK_LEVEL)
  {
    event->keyval = hildon_im_context_get_event_keyval_for_level(context,
                                                                 event,
                                                                 LOCKABLE_LEVEL);
  }


  if (shift_key_is_sticky || shift_key_is_locked || shift_key_is_down)
  {
    translation_state |= GDK_SHIFT_MASK;
  }

  /* When the level key is in sticky or locked state, translate the
   * keyboard state as if that level key was being held down. */
  if (level_key_is_sticky || level_key_is_locked || level_key_is_down)
  {
    translation_state |= LEVEL_KEY_MOD_MASK;

    /* When the level key is in sticky or locked state,  and we're not
     * in numeric/tele mode, translate the keyboard state as if that
     * level key was being held down. */
    if (!invert_level_behavior)
    {
      perform_level_translation (event, translation_state);
    }
    else if (level_key_is_down)
    {
      gboolean lock_level = context->options & HILDON_IM_LOCK_LEVEL;
      /* this fixes the case of pressing Fn+key at the same time:
       * X gives us the translated value, so as the LEVEL behaviour should be
       * inverted we use this to translate that numeric/special value to alpha;
       * we get the translated value, and have to translate it back */
      event->keyval = hildon_im_context_get_event_keyval_for_level(context,
                                                                   event,
                                                                   lock_level ? LOCKABLE_LEVEL:
                                                                   BASE_LEVEL);
    }

    if (event->keyval == COMPOSE_KEY)
      context->mask |= HILDON_IM_COMPOSE_MASK;

    invert_level_behavior = FALSE;
  }

  if (invert_level_behavior)
  {
    perform_level_translation (event, translation_state | LEVEL_KEY_MOD_MASK);
  }

#if defined(MAEMO_CHANGES) || GTK_CHECK_VERSION(3,0,0)
  /* Hardware keyboard autocapitalization  */
  if (context->auto_upper)
  {
    event->keyval = gdk_keyval_to_upper(event->keyval);
    if (event->keyval != GDK_Shift_L && event->keyval != GDK_Shift_R)
      context->auto_upper = FALSE;
  }
#endif

  /* Shift lock or holding the shift down forces uppercase,
   * ignoring autocap */
  if (shift_key_is_locked)
  {
    event->keyval = gdk_keyval_to_upper (event->keyval);
  }

  if (shift_key_is_sticky && !shift_key_is_locked)
  {
    /* Inverts the case or gets the character in the shift level */
    perform_shift_translation (event, translation_state);
  }
  else if (shift_key_is_down)
  {
    hildon_im_context_invert_case (event);
  }

  /* A dead key will not be immediately commited, but combined with the
   * next key */
  if (dead_key_to_unicode_combining_character (event->keyval))
  {
    context->mask |= HILDON_IM_DEAD_KEY_MASK;

    if (context->combining_char == 0)
    {
      context->combining_char = dead_key_to_unicode_combining_character(event->keyval);
      return TRUE;
    }
  }
  else
  {
    context->mask &= ~HILDON_IM_DEAD_KEY_MASK;
  }

  /* If a dead key was pressed twice, or once and followed by a space,
   * inputs the dead key's character representation */
  if ((context->mask & HILDON_IM_DEAD_KEY_MASK || event->keyval == GDK_space) &&
      context->combining_char)
  {
    c = get_representation_for_dead_character (event, context->combining_char);
    context->combining_char = 0;
  }
  else
  {
    c = gdk_keyval_to_unicode (event->keyval);
  }

  if (c && !is_ascii_control_char(c))
  {
    gchar utf8[10];

    reset_shift_and_level_keys_if_needed (context, event);

    /* Entering a new character cleans the preedit buffer */
    set_preedit_buffer (context, NULL);

    /* Pressing a dead key followed by a regular key combines to form
     * an accented character */
    if (context->combining_char)
      c = compose_character (context, c, context->combining_char);

    utf8[g_unichar_to_utf8 (c, utf8)] = '\0';

    hildon_im_context_send_key_event(context, event->type, event->state,
                                     gdk_unicode_to_keyval(c),
                                     event->hardware_keycode);
    context->last_internal_change = TRUE;

    gboolean inserted_text = FALSE;

    if (context->committed_preedit &&
        hildon_im_common_should_be_appended_after_letter (utf8))
    {
      inserted_text = insert_text (context, utf8, -1);
    }
    if (!inserted_text)
      commit_text (context, utf8);

    context->committed_preedit = FALSE;

    /* launch long press timeout */
    if (context->enable_long_press)
    {
      context->char_key_is_down = TRUE;
      context->long_press_timeout_src_id =
        g_timeout_add (context->long_press_timeout,
                       (GSourceFunc) hildon_im_context_on_long_press_timeout,
                       (gpointer) context);
      /* cache the key event */
      if (context->long_press_last_key_event != NULL)
        gdk_event_free ((GdkEvent *) context->long_press_last_key_event);

      context->long_press_last_key_event =
        (GdkEventKey *) gdk_event_copy ((GdkEvent *) event);
    }

    return TRUE;
  }
  else
  {
    if (level_key_is_sticky || level_key_is_locked)
      event->state |= LEVEL_KEY_MOD_MASK;

    hildon_im_context_send_key_event(context, event->type,
                                     event->state, event->keyval,
                                     event->hardware_keycode);

    /* Non-printable characters invalidate any previous dead keys */
    if (event->keyval != GDK_Shift_L &&
        event->keyval != GDK_Shift_R &&
        event->keyval != LEVEL_KEY)
    {
      context->combining_char = 0;
      context->committed_preedit = FALSE;
    }
  }

  if (event->keyval == GDK_BackSpace)
  {
    context->last_internal_change = TRUE;
  }

  return FALSE;
}

/* Filter for key events received by the client widget. */
static gboolean
hildon_im_context_filter_keypress(GtkIMContext *context, GdkEventKey *event)
{
  HildonIMContext *self;
  gboolean result = FALSE;

  guint last_keyval = 0;

  g_return_val_if_fail(HILDON_IS_IM_CONTEXT(context), FALSE);
  self = HILDON_IM_CONTEXT(context);

  /* When the widget isn't yet fully initialized, keys shouldn't
   * be processed in order to avoid eventual X errors. */
  if (!self->has_focus)
    return FALSE;

  /* Ignore already filtered events. Possible causes include derived
   * widgets where both child and parent keypress handlers are called
   * when the context itself doesn't consume the event. */
  if (self->last_key_event)
  {
    /* The EventKey struct can be reused by GDK with different values
     * filled in during one main loop iteration, so we do this lame
     * comparison */
    if (event->type == self->last_key_event->type &&
        event->time == self->last_key_event->time &&
        event->keyval == self->last_key_event->keyval)
    {
      return FALSE;
    }
    last_keyval = self->last_key_event->keyval;
    gdk_event_free((GdkEvent*)self->last_key_event);
  }
  self->last_key_event = (GdkEventKey*) gdk_event_copy((GdkEvent*)event);

  if (event->type == GDK_KEY_PRESS)
  {
    result = key_pressed (self, event);
  }
  else if (event->type == GDK_KEY_RELEASE)
  {
    result = key_released (self, event, last_keyval);
  }
  
  /* check here if we should change the autocaps state */
  if (g_unichar_isspace(gdk_keyval_to_unicode (event->keyval)))
  {
    hildon_im_context_check_sentence_start(self);
  }
    

  return result;
}

/* Filter for events (currently only button events) received by the client widget.
   This triggers the visibility of the IM window on button release. */
static gboolean
hildon_im_context_filter_event(GtkIMContext *context, GdkEvent *event)
{
  HildonIMContext *self;

  if (!context)
  {
    return FALSE;
  }

  g_return_val_if_fail(HILDON_IS_IM_CONTEXT(context), FALSE);
  self = HILDON_IM_CONTEXT(context);

  GdkEventButton *button_event = (GdkEventButton*) event;

  if (event->type == GDK_BUTTON_PRESS)
  {
    self->committed_preedit = FALSE;

    self->button_press_x = button_event->x_root;
    self->button_press_y = button_event->y_root;

    trigger = HILDON_IM_TRIGGER_FINGER;
    /* In Diablo, it would be a finger event if 
     *         gdk_event_get_axis ((GdkEvent*)event, GDK_AXIS_PRESSURE, &pressure)
     *      && pressure > HILDON_IM_FINGER_PRESSURE_THRESHOLD
     * 
     * with       #define HILDON_IM_FINGER_PRESSURE_THRESHOLD 0.4
     *
     * A finger event will cancel any prior launch that is still inside
     * the launch delay window, creating a better chance that a finger
     * press that generates multiple events is correctly indentified
     */
    if (launch_delay_timeout_id != 0)
    {
      g_source_remove(launch_delay_timeout_id);
      launch_delay_timeout_id = 0;
    }

    launch_delay = 0;
  }

  if (event->type == GDK_BUTTON_RELEASE)
  {
    trigger = HILDON_IM_TRIGGER_FINGER;

    gboolean should_show = FALSE;

    if (self->button_press_x == -1.0 && self->button_press_y == -1.0)
    {
      should_show = TRUE;
    }
    else if (!client_has_selection (self))
    {
      gdouble x = ABS (button_event->x_root - self->button_press_x);
      gdouble y = ABS (button_event->y_root - self->button_press_y);

      if (x < SHOW_CONTEXT_MAX_DISTANCE &&
          y < SHOW_CONTEXT_MAX_DISTANCE)
      {
        should_show = TRUE;
      }
      self->button_press_x = -1.0;
      self->button_press_y = -1.0;
    }

    if (should_show && self->has_focus)
      hildon_im_context_show_real(context);
  }

  return FALSE;
}

static void
hildon_im_context_set_cursor_location(GtkIMContext *context,
                                      GdkRectangle *area)
{
  HildonIMContext *self;
  g_return_if_fail(HILDON_IS_IM_CONTEXT(context));
  self = HILDON_IM_CONTEXT(context);

  if (!self->has_focus)
  {
    return;
  }

  if (self->last_internal_change)
  {
    /* Our own change */
    hildon_im_context_check_sentence_start(self);
    self->last_internal_change = FALSE;
    self->prev_surrounding_hash = 0;
  }
  else
  {
    gboolean need_free;
    gchar *surrounding;
    gint   cpos;
    guint  hash = 0;

    need_free = gtk_im_context_get_surrounding(context, &surrounding, &cpos);
    if (surrounding)
      hash = g_str_hash(surrounding);

    if (need_free)
      g_free(surrounding);

    /* Now we wish to see if cursor has actually moved.
       If cursor x/y hasn't moved, we're in same position */
    if ((area->y != self->prev_cursor_y || area->x != self->prev_cursor_x) &&
        (self->prev_surrounding_cursor_pos != cpos ||
         self->prev_surrounding_hash != hash ||
         self->prev_surrounding_hash == 0)
       )
    {
      hildon_im_context_check_sentence_start(self);
      hildon_im_context_reset_real(context);
      set_preedit_buffer (self, NULL);
    }

    self->prev_surrounding_hash = hash;
    self->prev_surrounding_cursor_pos = cpos;
  }

  self->prev_cursor_y = area->y;
  self->prev_cursor_x = area->x;
}

static gint
hildon_im_context_get_insert (HildonIMContext *self, gint cursor_position)
{
  gint insert = -1;

  if (GTK_IS_TEXT_VIEW (self->client_gtk_widget))
  {
    GtkTextIter selection_begin, cursor;
    GtkTextBuffer *buffer = NULL;
    GtkTextMark *cursor_mark = NULL;
    gchar *slice = NULL;

    buffer = get_buffer (self->client_gtk_widget);
    if (buffer != NULL &&
        gtk_text_buffer_get_selection_bounds (buffer, &selection_begin, NULL))
    {
      cursor_mark = gtk_text_buffer_get_insert (buffer);
      gtk_text_buffer_get_iter_at_mark (buffer, &cursor, cursor_mark);
      if (!gtk_text_iter_equal (&cursor, &selection_begin))
      {
        slice = gtk_text_iter_get_visible_slice (&selection_begin, &cursor);

        if (slice != NULL)
          insert = cursor_position - g_utf8_strlen (slice, -1);
      }
    }
  }
  else if (GTK_IS_EDITABLE (self->client_gtk_widget))
  {
    gtk_editable_get_selection_bounds (GTK_EDITABLE (self->client_gtk_widget),
                                       &insert,
                                       NULL);
  }

  if (insert < 0)
    return cursor_position;
  return insert;
}

/* Updates the IM with the autocap state at the active cursor position */
static void
hildon_im_context_check_sentence_start (HildonIMContext *self)
{
  g_return_if_fail(HILDON_IS_IM_CONTEXT(self));

  hildon_im_context_input_mode_changed (G_OBJECT (self), NULL);

  if (! self->auto_upper_enabled)
  {
    self->auto_upper = FALSE;
  }
  else
  {
    /* gboolean shift_is_sticky; */
    gboolean shift_is_locked;
    gboolean old_auto_upper;

    /* shift_is_sticky = self->mask & HILDON_IM_SHIFT_STICKY_MASK; */
    shift_is_locked = self->mask & HILDON_IM_SHIFT_LOCK_MASK;

    old_auto_upper = self->auto_upper;

    if (shift_is_locked)
    {
      self->auto_upper = FALSE;
    }
    else
    {
      gchar *surrounding = NULL;
      gint cpos = 0;

      hildon_im_context_get_surrounding (GTK_IM_CONTEXT (self),
                                         &surrounding,
                                         &cpos);

      cpos = hildon_im_context_get_insert (self, cpos);

      if (hildon_im_common_check_auto_cap (surrounding, cpos))
        self->auto_upper = TRUE;
      else
        self->auto_upper = FALSE;

      g_free (surrounding);
    }

    if ( (! old_auto_upper) && self->auto_upper)
      hildon_im_context_send_command (self, HILDON_IM_SHIFT_STICKY);
    else if (old_auto_upper && (! self->auto_upper))
      hildon_im_context_send_command (self, HILDON_IM_SHIFT_UNSTICKY);
  }
}

/* Ask the client widget to insert the specified text at the cursor
   position, by triggering the commit signal on the context */
static void
hildon_im_context_insert_utf8(HildonIMContext *self, gint flag,
                              const char *text)
{
  gint cpos;
  gint to_copy;
  gchar *surrounding, *text_clean = (gchar*) text;
  gchar tmp[3] = { 0, 0, 0};
  gboolean has_surrounding, free_text = FALSE;

  g_return_if_fail( HILDON_IS_IM_CONTEXT(self) );
  
  /* In PREEDIT mode, the text is used as the predicted suffix.
   * After this text has been received, the commit mode is reset. */
  if (self->commit_mode == HILDON_IM_COMMIT_PREEDIT)
  {
    switch(flag)
    {
    case HILDON_IM_MSG_START:
      g_string_truncate(self->incoming_preedit_buffer, 0);
      g_string_append(self->incoming_preedit_buffer, text);
      break;
    case HILDON_IM_MSG_CONTINUE:
      g_string_append(self->incoming_preedit_buffer, text);
      break;
    case HILDON_IM_MSG_END:
      g_string_append(self->incoming_preedit_buffer, text);
      set_preedit_buffer (self, self->incoming_preedit_buffer->str);
      g_string_truncate(self->incoming_preedit_buffer, 0);
      self->commit_mode = self->previous_commit_mode;
      self->changed_count = 0;
      self->last_internal_change = TRUE;
      break;
    }
    return;
  }
  else
  {
    set_preedit_buffer (self, NULL);
  }
  
  if (self->options & HILDON_IM_AUTOCORRECT)
  {
    has_surrounding = gtk_im_context_get_surrounding (
        GTK_IM_CONTEXT(self),
        &surrounding, &cpos);

    if (has_surrounding)
    {
      if (surrounding [cpos] == 0)
      {
        if (surrounding [cpos - 1] == ' ')
        {
          to_copy = hildon_im_autocorrection_check_character (text);

          if (to_copy > 0)
          {
            memcpy (tmp, text, to_copy);
            text_clean = g_strconcat (tmp, " ", text + to_copy, NULL);
            gtk_im_context_delete_surrounding (GTK_IM_CONTEXT(self),
                -1, 1);
            free_text = TRUE;
          }
        }
      }
      g_free (surrounding);
    }
  }

  /* This last "commit" signal adds the actual text. We're assuming it sends
     0 or 1 "changed" signals (we try to guarantee that by sending a "" commit
     above to delete highlights).

     If we get more than one "changed" signal, it means that the
     application's "changed" signal handler went and changed the text
     as a result of the change, and we need to clear IM. */
  self->changed_count = 0;
  self->last_internal_change = TRUE;
  commit_text (self, text_clean);

  if (free_text == TRUE)
  {
    g_free (text_clean);
  }
  /* If last_internal_change is still TRUE, it means set_cursor_location()
     wasn't called yet. This happens at least with GtkEntry where it's called
     in idle handler. */

  if (self->last_internal_change && self->changed_count > 1)
  {
    /* External change seen, clear IM in set_cursor_location() handler */
    self->last_internal_change = FALSE;
    self->prev_cursor_y = self->prev_cursor_x = -1;
  }
}

static void
hildon_im_context_send_input_mode (HildonIMContext *self)
{
  XEvent event;
  Window window;
  HildonIMInputModeMessage *msg;
  HildonGtkInputMode input_mode = 0;
  HildonGtkInputMode default_input_mode = 0;

  window = get_window_id(hildon_im_protocol_get_atom(HILDON_IM_WINDOW));

  if (window == None)
  {
    g_warning("hildon_im_context_send_input_mode: Could not get the window id.\n");
    return;
  }

#if defined(MAEMO_CHANGES) || GTK_CHECK_VERSION(3,0,0)
  hildon_get_input_mode (self, &input_mode, &default_input_mode);
#endif

  memset(&event, 0, sizeof(XEvent));
  event.xclient.type = ClientMessage;
  event.xclient.window = window;
  event.xclient.message_type =
          hildon_im_protocol_get_atom (HILDON_IM_INPUT_MODE);
  event.xclient.format = HILDON_IM_INPUT_MODE_FORMAT;

  msg = (HildonIMInputModeMessage *) &event.xclient.data;
  msg->input_mode = input_mode;
  msg->default_input_mode = default_input_mode;

  hildon_im_context_send_event (self, &event);
}

/* Sends a client message with the specified command to the IM window */
static void
hildon_im_context_send_command(HildonIMContext *self,
                               HildonIMCommand cmd)
{
  XEvent event;
  gint xerror;
  HildonIMActivateMessage *msg;
  GdkWindow *input_window = NULL;

  g_return_if_fail (self != NULL);

  if (self->client_gdk_window != NULL)
  {
    input_window = self->client_gdk_window;
  }
  else
  {
    return;
  }

  memset(&event, 0, sizeof(XEvent));
  event.xclient.type = ClientMessage;
  event.xclient.window = self->im_window;
  event.xclient.message_type =
          hildon_im_protocol_get_atom(HILDON_IM_ACTIVATE);
  event.xclient.format = HILDON_IM_ACTIVATE_FORMAT;

  msg = (HildonIMActivateMessage *) &event.xclient.data;

  if (cmd == HILDON_IM_SETCLIENT || cmd == HILDON_IM_SETNSHOW)
  {
    hildon_im_context_send_input_mode (self);
  }

  if (cmd != HILDON_IM_HIDE)
  {
    msg->input_window = GDK_WINDOW_XID(input_window);

    /* When the client widget is a child of GtkPlug, the application can
       override the ID of the window the IM will be set transient to */
    if (self->client_gtk_widget != NULL &&
        GTK_IS_PLUG(gtk_widget_get_toplevel(self->client_gtk_widget)))
    {
      GtkPlug *plug = (GtkPlug *)gtk_widget_get_toplevel(self->client_gtk_widget);
      guint32 transient_window_xid;

      transient_window_xid =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(plug),
                                           "hildon_im_transient_xid"));
      if (transient_window_xid)
        msg->app_window = transient_window_xid;
      else
        msg->app_window = GDK_WINDOW_XID(gdk_window_get_toplevel(input_window));
    }
    else
    {
      msg->app_window = GDK_WINDOW_XID(gdk_window_get_toplevel(input_window));
    }
  }

  msg->cmd = cmd;
  msg->trigger = trigger;

  /*trap X errors.  We need this, because if the IM window is destroyed for some
   *reason (segfault etc), then it's possible that the IM window id
   *is invalid.
   */
  gdk_error_trap_push();

  XSendEvent(gdk_x11_get_default_xdisplay (), self->im_window, False, 0, &event);
  /* TODO this call can cause a hang, see NB#97380 */

  xerror = gdk_error_trap_pop();
  if (xerror)
  {
    if (xerror == BadWindow)
    {
      self->im_window = get_window_id(hildon_im_protocol_get_atom(HILDON_IM_WINDOW));
      event.xclient.window = self->im_window;
      XSendEvent(gdk_x11_get_default_xdisplay (), self->im_window, False, 0, &event);
    }
    else
    {
      g_warning("Received the X error %d\n", xerror);
    }
  }
}

static char *
get_next_packet_start(char *str)
{
  char *candidate, *good;

  candidate = good = str;

  while (*candidate)
  {
    candidate++;
    if (candidate - str >= HILDON_IM_CLIENT_MESSAGE_BUFFER_SIZE)
    {
      return good;
    }
    good = candidate;
  }

  /* The whole string is small enough */
  return candidate;
}

static void
hildon_im_context_send_event(HildonIMContext *self, XEvent *event)
{
  gint xerror;

  g_return_if_fail(event);

  if(self->im_window != None )
  {
    event->xclient.type = ClientMessage;
    event->xclient.window = self->im_window;

    /* trap X errors. Sometimes we recieve a badwindow error,
     * because the input_window id is wrong.
     * 
     * TODO this code is replicated in too many places in this file
     */
    gdk_error_trap_push();

    XSendEvent(gdk_x11_get_default_xdisplay (), self->im_window, False, 0, event);

    xerror = gdk_error_trap_pop();
    if( xerror )
    {
      if( xerror == BadWindow )
      {
        self->im_window = get_window_id(hildon_im_protocol_get_atom(HILDON_IM_WINDOW));
        event->xclient.window = self->im_window;
        XSendEvent(gdk_x11_get_default_xdisplay (), self->im_window, False, 0, event);
      }
      else
      {
        g_warning("Received the X error %d\n", xerror);
      }
    }
  }
}

static gchar*
get_full_line (HildonIMContext *self, gint *offset)
{
  /* TODO gtk_im_context_get_surrounding doesn't work well with multiple lines
   * check mode & HILDON_GTK_INPUT_MODE_MULTILINE */
  gchar *surrounding = NULL;
  /* gboolean multiline = FALSE; */
  
  *offset = 0;

#if defined(MAEMO_CHANGES) || GTK_CHECK_VERSION(3,0,0)
  HildonGtkInputMode input_mode = 0;
  hildon_get_input_mode (self, &input_mode, NULL);
  /* multiline = (input_mode & HILDON_GTK_INPUT_MODE_MULTILINE) != 0; */
#else
  /* TODO add other multiline widgets */
  /* multiline = GTK_IS_TEXT_VIEW(self->client_gtk_widget); */
#endif
  
  if (GTK_IS_EDITABLE (self->client_gtk_widget))
  {
    surrounding = 
      gtk_editable_get_chars(GTK_EDITABLE(self->client_gtk_widget), 0, -1);
    *offset = gtk_editable_get_position (GTK_EDITABLE (self->client_gtk_widget));
  }
  else if (GTK_IS_TEXT_VIEW(self->client_gtk_widget))
  {
    GtkTextMark *insert_mark;
    GtkTextBuffer *buffer;
    GtkTextIter insert_i, start_i, end_i;

    buffer = get_buffer(self->client_gtk_widget);
    insert_mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &insert_i, insert_mark);

    start_i = insert_i;
    end_i = insert_i;

    if (gtk_text_iter_get_line_offset (&start_i) != 0)
    {
      gtk_text_iter_set_line_offset (&start_i, 0);
    }
    else
    {
      gtk_text_iter_backward_line (&start_i);
    }
    gtk_text_iter_forward_to_line_end (&end_i);

    surrounding = gtk_text_buffer_get_slice(buffer, &start_i, &end_i, FALSE);

    *offset = gtk_text_iter_get_offset(&insert_i) - gtk_text_iter_get_offset(&start_i);
  }
  else
  {
    gint byte_offset = 0;
    
    gtk_im_context_get_surrounding(GTK_IM_CONTEXT(self),
                                   &surrounding, &byte_offset);
    /* The following is needed since the value assigned to offset is
     * the byte position, not the real cursor position in the text */
    if (surrounding != NULL)
    {
      gchar *str_pos = surrounding + byte_offset;
      *offset = g_utf8_pointer_to_offset (surrounding, str_pos);
    }
    else
    {
      *offset = 0;
    }
  }
  
  return surrounding;
}

static gchar*
get_short_surrounding (HildonIMContext *self, gint *offset)
{
  /* Ideally, we should only return the two words before and the word after */
  gchar* long_surrounding = NULL;
  gchar* cursor_position = NULL;
  gchar* prev_char = NULL;
  gchar* short_surrounding = NULL;
  gchar* short_surrounding_start = NULL;
  gchar* short_surrounding_end = NULL;
  gchar* next_char = NULL;
  gint long_offset = 0;
  gint off_start = 0;
  /* gint off_end = 0; */
  
  long_surrounding = get_full_line (self, &long_offset);
  
  if (long_surrounding == NULL)
  {
    return NULL;
  }
  
  cursor_position = g_utf8_offset_to_pointer(long_surrounding, long_offset);
  prev_char = g_utf8_find_prev_char(long_surrounding, cursor_position);
  if (prev_char == NULL)
  {
    prev_char = cursor_position;
  }
  next_char = cursor_position;
  
  /* move to the previous word start */
  while (prev_char != NULL && !g_unichar_isspace (g_utf8_get_char(prev_char)))
  {
    short_surrounding_start = prev_char; 
    prev_char = g_utf8_find_prev_char(long_surrounding, prev_char);
  }
  
  /* and now the first word before that */
  while (prev_char != NULL && g_unichar_isspace (g_utf8_get_char(prev_char)))
  {
    short_surrounding_start = prev_char;
    prev_char = g_utf8_find_prev_char(long_surrounding, prev_char);
  }

  /* and now to its start */
  while (prev_char != NULL && !g_unichar_isspace (g_utf8_get_char(prev_char)))
  {
    short_surrounding_start = prev_char;
    prev_char = g_utf8_find_prev_char(long_surrounding, prev_char);
  }
  
  /* now, let's move forward */
  do
  {
    short_surrounding_end = next_char;
    next_char = g_utf8_find_next_char(next_char, NULL);
  }
  while (next_char != NULL && next_char[0] != '\0'
         && !g_unichar_isspace (g_utf8_get_char(next_char)));
  
  off_start = g_utf8_pointer_to_offset (long_surrounding, short_surrounding_start);
  /* off_end = g_utf8_pointer_to_offset (long_surrounding, short_surrounding_end); */
  
  *offset = long_offset - off_start;
  short_surrounding = g_strndup(short_surrounding_start,
                                short_surrounding_end - short_surrounding_start);

  g_free(long_surrounding);
  
  return short_surrounding;
}

static void
hildon_im_context_send_surrounding_header(HildonIMContext *self, gint offset)
{
  HildonIMSurroundingMessage *surrounding_msg=NULL;
  XEvent event;

  g_return_if_fail(HILDON_IS_IM_CONTEXT(self));

  /* Send the cursor offset in the surrounding */
  memset( &event, 0, sizeof(XEvent) );
  event.xclient.message_type = hildon_im_protocol_get_atom(HILDON_IM_SURROUNDING);
  event.xclient.format = HILDON_IM_SURROUNDING_FORMAT;

  surrounding_msg = (HildonIMSurroundingMessage *) &event.xclient.data;
  surrounding_msg->commit_mode = self->commit_mode;
  surrounding_msg->cursor_offset = offset;
  hildon_im_context_send_event(self, &event);
}

static Bool
is_request_for_surrounding(Display *display,
                           XEvent *event,
                           XPointer arg)
{
  if (event->type == ClientMessage)
  {
    XClientMessageEvent *cme = (XClientMessageEvent *)event;

    if (cme->message_type == hildon_im_protocol_get_atom(HILDON_IM_COM)
        && cme->format == HILDON_IM_COM_FORMAT)
    {
      HildonIMComMessage *msg = (HildonIMComMessage *)&cme->data;

      if (msg->type == HILDON_IM_CONTEXT_REQUEST_SURROUNDING
          || msg->type == HILDON_IM_CONTEXT_REQUEST_SURROUNDING_FULL)
      {
        return True;
      }
    }
  }

  return False;
}
/* Send the text of the client widget surrounding the active cursor position,
   as well as the the cursor's position in the surrounding, to the IM */
static void
hildon_im_context_send_surrounding(HildonIMContext *self, gboolean send_full_line)
{
  HildonIMSurroundingContentMessage *surrounding_content_msg=NULL;
  XEvent event;
  XEvent next_request_event;
  Bool go_on = False;
  gint flag;
  gchar *surrounding = NULL;
  gchar *str;
  gint offset = 0;

  g_return_if_fail(HILDON_IS_IM_CONTEXT(self));

  if (self->client_gtk_widget == NULL)
  {
    return;
  }
  
  flag = HILDON_IM_MSG_START;

  do
  {
    go_on = XCheckIfEvent(gdk_x11_get_default_xdisplay(),
                          &next_request_event,
                          is_request_for_surrounding,
                          (XPointer)self);


    if (go_on == True)
    {
      XClientMessageEvent *cme = (XClientMessageEvent *) &next_request_event;
      HildonIMComMessage *msg = (HildonIMComMessage *)&cme->data;

      send_full_line = send_full_line || (msg->type == HILDON_IM_CONTEXT_REQUEST_SURROUNDING_FULL);
    }
    /* TODO free the event? */
  }  
  while (go_on == True);
  
  if (send_full_line)
  {
    surrounding = get_full_line (self, &offset);
  }
  else
  {
    surrounding = get_short_surrounding (self, &offset);
  }

  if (surrounding == NULL)
  {
    hildon_im_context_send_surrounding_header(self, 0);
    return;
  }

  /* Split surrounding context into pieces that are small enough
     to send in a x message */
  str = surrounding;
  do
  {
    gchar *next_start;
    gsize len;

    next_start = get_next_packet_start(str);
    len = next_start - str;
    g_return_if_fail(0 <= len && len < HILDON_IM_CLIENT_MESSAGE_BUFFER_SIZE);

    /*this call will take care of adding the null terminator*/
    memset( &event, 0, sizeof(XEvent) );
    event.xclient.message_type = hildon_im_protocol_get_atom( HILDON_IM_SURROUNDING_CONTENT );
    event.xclient.format = HILDON_IM_SURROUNDING_CONTENT_FORMAT;

    surrounding_content_msg = (HildonIMSurroundingContentMessage *) &event.xclient.data;
    surrounding_content_msg->msg_flag = flag;
    memcpy(surrounding_content_msg->surrounding, str, len);

    hildon_im_context_send_event(self, &event);

    str = next_start;
    flag = HILDON_IM_MSG_CONTINUE;
  } while (*str);

  hildon_im_context_send_surrounding_header(self, offset);

  g_free(surrounding);
}

/* Send to the IM the preedit text that has been committed by the context. You
 * can think of this as a way to make the IM aware of the "commit" signal. */
static void
hildon_im_context_send_committed_preedit(HildonIMContext *self, gchar* committed_preedit)
{
  HildonIMPreeditCommittedMessage *preedit_comm_msg = NULL;
  HildonIMPreeditCommittedContentMessage *preedit_comm_content_msg = NULL;
  XEvent event;
  gint flag;
  gchar *surrounding = NULL;
  gchar *str;

  g_return_if_fail(HILDON_IS_IM_CONTEXT(self));

  flag = HILDON_IM_MSG_START;

  str = committed_preedit;
  do
  {
    gchar *next_start;
    gsize len;

    next_start = get_next_packet_start(str);
    len = next_start - str;
    g_return_if_fail(0 <= len && len < HILDON_IM_CLIENT_MESSAGE_BUFFER_SIZE);

    memset( &event, 0, sizeof(XEvent) );
    event.xclient.message_type = hildon_im_protocol_get_atom( HILDON_IM_PREEDIT_COMMITTED_CONTENT );
    event.xclient.format = HILDON_IM_PREEDIT_COMMITTED_CONTENT_FORMAT;

    preedit_comm_content_msg = (HildonIMPreeditCommittedContentMessage *) &event.xclient.data;
    preedit_comm_content_msg->msg_flag = flag;
    memcpy(preedit_comm_content_msg->committed_preedit, str, len);

    hildon_im_context_send_event(self, &event);

    str = next_start;
    flag = HILDON_IM_MSG_CONTINUE;
  } while (*str);

  /*
   * Now we send the header.
   */
  
  memset( &event, 0, sizeof(XEvent) );
  event.xclient.message_type = hildon_im_protocol_get_atom(HILDON_IM_PREEDIT_COMMITTED);
  event.xclient.format = HILDON_IM_PREEDIT_COMMITTED_FORMAT;

  preedit_comm_msg = (HildonIMPreeditCommittedMessage *) &event.xclient.data;
  preedit_comm_msg->commit_mode = self->commit_mode;
  hildon_im_context_send_event(self, &event);

  g_free(surrounding);
}


/* Send a key event to the IM, which makes it available to the plugins */
static void
hildon_im_context_send_key_event(HildonIMContext *self,
                                 GdkEventType type,
                                 guint state,
                                 guint keyval,
                                 guint16 hardware_keycode)
{
  HildonIMKeyEventMessage *key_event_msg=NULL;
  XEvent event;

  g_return_if_fail(HILDON_IS_IM_CONTEXT(self));

  if (self->is_internal_widget)
    return;
  
  memset(&event, 0, sizeof(XEvent));
  event.xclient.message_type = hildon_im_protocol_get_atom(HILDON_IM_KEY_EVENT);
  event.xclient.format = HILDON_IM_KEY_EVENT_FORMAT;

  key_event_msg = (HildonIMKeyEventMessage *) &event.xclient.data;
  key_event_msg->input_window = GDK_WINDOW_XID(self->client_gdk_window);
  key_event_msg->type = type;
  key_event_msg->state = state;
  key_event_msg->keyval = keyval;
  key_event_msg->hardware_keycode = hardware_keycode;

  hildon_im_context_send_event(self, &event);
}

static gboolean
hildon_im_context_show_cb(GtkIMContext *context)
{
  HildonIMContext *self;
  
  g_return_val_if_fail(HILDON_IS_IM_CONTEXT(context), FALSE);
  self = HILDON_IM_CONTEXT(context);

  /* Avoid autocap on inactive window. */
  if (self->has_focus)
  {
    hildon_im_context_check_sentence_start(self);
  }

  hildon_im_context_send_command(self, HILDON_IM_SETNSHOW);

  launch_delay_timeout_id = 0;

  return FALSE;
}

static void
hildon_im_context_show_real(GtkIMContext *context)
{
  HildonIMContext *self;

  g_return_if_fail(HILDON_IS_IM_CONTEXT(context));
  self = HILDON_IM_CONTEXT(context);

  /* Prevent IM UI recursion */
  if (self->is_internal_widget)
  {
    return;
  }

  launch_delay_timeout_id = g_timeout_add(launch_delay,
      (GSourceFunc)hildon_im_context_show_cb, context);
}

#ifdef MAEMO_CHANGES
/* Exposed to applications through hildon_gtk_im_context_show.
 * We use the "unknown" trigger and the HIM UI will try to use the best plugin
 * taking into account the state of the keyboard and other things.
 * Inside the framework, use the _real variant.
 */
static void
hildon_im_context_show(GtkIMContext *context)
{
  trigger = HILDON_IM_TRIGGER_UNKNOWN;

  hildon_im_context_show_real(context);
}
#endif

/* Ask the IM to hide its window */
static void
hildon_im_context_hide(GtkIMContext *context)
{
  HildonIMContext *self;
  g_return_if_fail(HILDON_IS_IM_CONTEXT(context));
  self = HILDON_IM_CONTEXT(context);

  hildon_im_context_send_command(self, HILDON_IM_HIDE);
}

/* Ask the IM to reset the UI state */
static void
hildon_im_context_reset_real(GtkIMContext *context)
{
  HildonIMContext *self;
  g_return_if_fail(HILDON_IS_IM_CONTEXT(context));
  self = HILDON_IM_CONTEXT(context);

  /* the preedit buffer has to be cleared by the plugin
   * it will be cleared anyway when the current widget loses the focus
   */   
  self->show_preedit = FALSE;
  
  hildon_im_context_send_command(self, HILDON_IM_CLEAR);
}

/* This function is disabled from use outside the scope of the
   Hildon IM context, as GTK and the Hildon UI specification have
   irreconcilable views of when the IM should be reset */
static void
hildon_im_context_reset(GtkIMContext *context)
{
  if (internal_reset)
  {
    internal_reset = FALSE;
    hildon_im_context_reset_real(context);
  }
}
