declare name "Oversampled Triangle Oscillator";
declare author "Jean Pierre Cimalando";
declare copyright "2019";
declare license "CC0-1.0";
declare version "1.0";

import("stdfaust.lib");

OS = fconstant(int gOversampling, <math.h>);

process = os.lf_triangle(frequency/OS) with {
  note = hslider("[1] Note [unit:semitone]", 69, 0, 127, 1);
  frequency = ba.midikey2hz(note);
};
