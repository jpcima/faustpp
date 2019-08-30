declare name "Oversampled Hard Clipper";
declare author "Jean Pierre Cimalando";
declare copyright "2019";
declare license "CC0-1.0";
declare version "1.0";

import("stdfaust.lib");

OS = fconstant(int gOversampling, <math.h>);

process(x) = ba.if(x>0.0, x, 0.0);
