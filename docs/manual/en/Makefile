
ADOC = asciidoctor
ADOCFLAGS = -a toc=left

all:
	$(ADOC) $(ADOCFLAGS) manual.txt

clean:
	rm -f manual.html

auto:
	while true; do $(ADOC) $(ADOCFLAGS) manual.txt; \
	    inotifywait -q -e close_write manual.txt; done
