ACLOCAL_AMFLAGS = -I m4

# Note: The order of directories in this list isn't arbitrary
SUBDIRS = \
	xfce4++ \
	xfce4++/util \
	include \
	lib \
	panel-plugin \
	src \
	po \
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

EXTRA_DIST =                            \
	AUTHORS                             \
	COPYING                             \
	ChangeLog                           \
	LICENSE                             \
	NEWS                                \
	README                              \
	TODO                                \
	NOTES                               \
	xfce4-sensors-plugin.spec.in        \
	$(NULL)

DISTCLEANFILES =                        \
	xfce4-sensors-plugin.spec           \
	$(NULL)

DISTCHECK_CONFIGURE_FLAGS =             \
	--disable-hddtemp                   \
	--disable-xnvctrl                   \
	--enable-libsensors                 \
	--enable-notification               \
	--enable-procacpi                   \
	--enable-sysfsacpi

checksums: distcheck
	sha1sum $(PACKAGE)-$(VERSION).tar.bz2 > $(PACKAGE)-$(VERSION).tar.bz2.sha1.asc

gpg-sign: checksums
	gpg -b --armor $(PACKAGE)-$(VERSION).tar.bz2

release: gpg-sign
	-mkdir releases/
	mv $(PACKAGE)-$(VERSION).tar.bz2* releases/
	-rm $(PACKAGE)-$(VERSION).tar.gz
