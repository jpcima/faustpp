
FPP = ../bin/faustpp
CXX = g++
CXXFLAGS = -std=c++11 -O2 -g -I../thirdparty/hiir
PKG_CONFIG = pkg-config

all: \
    bin/stone_phaser \
    bin/stone_phaser_stereo \
    $(foreach oversampling,1 2 4 8 16,bin/osctriangle$(oversampling)X) \
    $(foreach oversampling,1 2 4 8 16,bin/hardclip$(oversampling)X)

clean:
	rm -rf bin build

bin/stone_phaser: stone_phaser.dsp
	@mkdir -p bin build
	$(FPP) -a ../architectures/generic.hpp -DIdentifier=$(notdir $@) $^ > build/$(notdir $@).hpp
	$(FPP) -a ../architectures/generic.cpp -DIdentifier=$(notdir $@) $^ > build/$(notdir $@).cpp
	$(FPP) -a ../architectures/jack_simple.cpp -DIdentifier=$(notdir $@) $^ > build/$(notdir $@)Jack.cpp
	$(CXX) -o $@ build/$(notdir $@).cpp build/$(notdir $@)Jack.cpp $(CXXFLAGS) `$(PKG_CONFIG) jack --cflags --libs`

bin/stone_phaser_stereo: stone_phaser_stereo.dsp
	@mkdir -p bin build
	$(FPP) -a ../architectures/generic.hpp -DIdentifier=$(notdir $@) $^ > build/$(notdir $@).hpp
	$(FPP) -a ../architectures/generic.cpp -DIdentifier=$(notdir $@) $^ > build/$(notdir $@).cpp
	$(FPP) -a ../architectures/jack_simple.cpp -DIdentifier=$(notdir $@) $^ > build/$(notdir $@)Jack.cpp
	$(CXX) -o $@ build/$(notdir $@).cpp build/$(notdir $@)Jack.cpp $(CXXFLAGS) `$(PKG_CONFIG) jack --cflags --libs`

bin/osctriangle%X: osctriangle.dsp
	@mkdir -p bin build
	$(FPP) -a ../architectures/oversampled.hpp -DIdentifier=$(notdir $@) -DOversampling=$* $^ > build/$(notdir $@).hpp
	$(FPP) -a ../architectures/oversampled.cpp -DIdentifier=$(notdir $@) -DOversampling=$* $^ > build/$(notdir $@).cpp
	$(FPP) -a ../architectures/jack_simple.cpp -DIdentifier=$(notdir $@) -DOversampling=$* $^ > build/$(notdir $@)Jack.cpp
	$(CXX) -o $@ build/$(notdir $@).cpp build/$(notdir $@)Jack.cpp $(CXXFLAGS) `$(PKG_CONFIG) jack --cflags --libs`

bin/hardclip%X: hardclip.dsp
	@mkdir -p bin build
	$(FPP) -a ../architectures/oversampled.hpp -DIdentifier=$(notdir $@) -DOversampling=$* $^ > build/$(notdir $@).hpp
	$(FPP) -a ../architectures/oversampled.cpp -DIdentifier=$(notdir $@) -DOversampling=$* $^ > build/$(notdir $@).cpp
	$(FPP) -a ../architectures/jack_simple.cpp -DIdentifier=$(notdir $@) -DOversampling=$* $^ > build/$(notdir $@)Jack.cpp
	$(CXX) -o $@ build/$(notdir $@).cpp build/$(notdir $@)Jack.cpp $(CXXFLAGS) `$(PKG_CONFIG) jack --cflags --libs`
