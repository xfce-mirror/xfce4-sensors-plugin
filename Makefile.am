@SET_MAKE@

SUBDIRS = panel-plugin

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

EXTRA_DIST =								\
	AUTHORS								\
	COPYING								\
	ChangeLog							\
	INSTALL								\
	NEWS								\
	README								\
	TODO								\
	xfce4-sensors-plugin.spec

ACLOCAL_AMFLAGS = -I m4
