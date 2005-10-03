@SET_MAKE@

SUBDIRS = panel-plugin					\
	  po

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
        xfce4-sensors-plugin.spec			\
        intltool-extract.in                 \
        intltool-merge.in                   \
        intltool-update.in            	    \
        xfce-sensors.png

DISTCLEANFILES =						\
	intltool-extract					\
	intltool-merge						\
	intltool-update

