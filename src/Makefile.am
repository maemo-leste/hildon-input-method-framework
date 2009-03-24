# This file is part of hildon-input-method-framework
#
# Copyright (C) 2005-2007 Nokia Corporation.
#
# Contact: Mohammad Anwari <Mohammad.Anwari.com>
#

INCLUDES = \
	$(GTK_CFLAGS) \
	$(HILDON_CFLAGS) \
	$(HILDON_IM_CFLAGS)\
	$(GCONF_CFLAGS)\
	$(LIBOSSO_CFLAGS) \
	$(LIBOSSOHELP_CFLAGS) \
	$(HILDON_LGPL_CFLAGS) \
	$(XEXTPROTO_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\"

immoduledir = $(libdir)/gtk-2.0/$(GTK_VERSION)/immodules
immodule_LTLIBRARIES = hildon-im-module.la 

commondir = $(libdir)
common_LTLIBRARIES = libhildon_im_common.la

libhildon_im_common_la_SOURCES = \
	hildon-im-common.c \
  hildon-im-protocol.c \
	hildon-im-common.h
libhildon_im_common_la_LDFLAGS = -version-info $(LIBVERSION)\
	$(GTK_LIBS) 

hildon_im_module_la_SOURCES = \
	hildon-im-context.h \
	hildon-im-context.c \
	hildon-im-module.c \
	hildon-im-gtk.h \
	hildon-im-gtk.c
hildon_im_module_la_LIBADD = \
	$(GTK_LIBS) $(HILDON_LIBS) $(XEXTPROTO_LIBS) \
	-lhildon_im_common
hildon_im_module_la_LDFLAGS = \
	-rpath $(immoduledir) \
	-avoid-version \
	-module
hildon_im_module_la_CFLAGS = @CFLAGS@ -DG_LOG_DOMAIN=\""hildon-im-module"\"

hildon_input_method_frameworkincludeinstdir = $(includedir)/hildon-input-method
hildon_input_method_frameworkincludeinst_DATA = \
	hildon-im-common.h \
	hildon-im-context.h \
	hildon-im-protocol.h

