
PREFIX = /usr/local
DOCDIR = $(PREFIX)/share/doc
LANGS = en

all:
	$(foreach lang,$(LANGS),$(MAKE) -C $(lang);)

clean:
	$(foreach lang,$(LANGS),$(MAKE) -C $(lang) clean;)

install:
	install -d $(DESTDIR)$(DOCDIR)/faustpp/manual/
	cp -rf resources $(DESTDIR)$(DOCDIR)/faustpp/manual/
	$(foreach lang,$(LANGS),install -D $(lang)/manual.html $(DESTDIR)$(DOCDIR)/faustpp/manual/$(lang)/manual.html;)
