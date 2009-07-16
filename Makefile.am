# This file is part of hildon-input-method
#
# Copyright (C) 2005-2007 Nokia Corporation.
#
# Contact: Mohammad Anwari <Mohammad.Anwari@nokia.com>
SUBDIRS = src docs

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-input-method-framework-3.0.pc 

configdir = $(sysconfdir)/hildon

deb: dist
	-mkdir debian-build
	cd debian-build && \
	tar zxvf ../$(PACKAGE)-$(VERSION).tar.gz && \
	cd $(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
