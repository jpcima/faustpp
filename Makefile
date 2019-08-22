PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share
CXX = g++
CXXFLAGS = -O2 -g -Wall -std=c++11
LDFLAGS =

PROJECT_CXXFLAGS = -Ithirdparty
PROJECT_LDFLAGS =

SRCS = main.cpp call_faust.cpp metadata.cpp thirdparty/pugixml/pugixml.cpp
OBJS = $(SRCS:%.cpp=build/%.o)

all: bin/faustpp

clean:
	rm -rf bin
	rm -rf build

install: bin/faustpp
	install -D -m 755 bin/faustpp $(DESTDIR)$(BINDIR)/faustpp
	install -D -m 644 architectures/example.hpp $(DESTDIR)$(DATADIR)/faustpp/architectures/example.hpp
	install -D -m 644 architectures/example.cpp $(DESTDIR)$(DATADIR)/faustpp/architectures/example.cpp

bin/faustpp: $(OBJS)
	@install -d $(dir $@)
	$(CXX) $(LDFLAGS) $(PROJECT_LDFLAGS) -o $@ $^

build/%.o: %.cpp
	@install -d $(dir $@)
	$(CXX) $(CXXFLAGS) $(PROJECT_CXXFLAGS) -MD -c -o $@ $<

.PHONY: all clean install

-include $(OBJS:%.o=%.d)
