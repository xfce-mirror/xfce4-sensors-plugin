# $Id$

SUBDIRS = panel-plugin					\
	  po						\
	  icons

distclean-local:
	rm -rf *.cache *~

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | \
	  bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION).tar.gz | \
	  bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

EXTRA_DIST =							\
        AUTHORS								\
        COPYING								\
        ChangeLog							\
        INSTALL								\
        NEWS								\
        README								\
        TODO								\
		NOTES								\
        xfce4-sensors-plugin.spec.in		\
        intltool-extract.in                 \
        intltool-merge.in                   \
        intltool-update.in

DISTCLEANFILES =						\
	intltool-extract					\
	intltool-merge						\
	intltool-update						\
	xfce4-sensors-plugin.spec

DISTCHECK_CONFIGURE_FLAGS =						\
	--with-libsensors						\
	--with-hddtemp							\
	--with-acpi

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
