//-----------------------------------------------------------------------------
// This file was generated using the Faust compiler (https://faust.grame.fr),
// and the Faust post-processor (https://github.com/jpcima/faustpp).
//
// Source: {{file_name}}
// Name: {{name}}
// Author: {{author}}
// Copyright: {{copyright}}
// License: {{license}}
// Version: {{version}}
//-----------------------------------------------------------------------------

{% if Oversampling >= 2 %}
//------------------------------------------------------------------------------
// Oversampling filters reused from iPlug2 source code
//------------------------------------------------------------------------------
// Copyright (C) the iPlug 2 Developers. Portions copyright other contributors,
// see each source file for more information.
//
// Based on WDL-OL/iPlug by Oli Larkin (2011-2018),
// and the original iPlug v1 (2008) by John Schwartz / Cockos
//
// LICENSE:
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the
// use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not claim
//    that you wrote the original software. If you use this software in a
//    product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//------------------------------------------------------------------------------
{% endif %}

#include "{{Identifier}}.hpp"
#include <algorithm>
#include <cmath>

class {{Identifier}}::BasicDsp {
public:
    virtual ~BasicDsp() {}
};

//------------------------------------------------------------------------------
// Begin the Faust code section

namespace {

template <class T> inline T min(T a, T b) { return (a < b) ? a : b; }
template <class T> inline T max(T a, T b) { return (a > b) ? a : b; }

class Meta {
public:
    // dummy
    void declare(...) {}
};

class UI {
public:
    // dummy
    void openHorizontalBox(...) {}
    void openVerticalBox(...) {}
    void closeBox(...) {}
    void declare(...) {}
    void addButton(...) {}
    void addCheckButton(...) {}
    void addVerticalSlider(...) {}
    void addHorizontalSlider(...) {}
    void addVerticalBargraph(...) {}
};

typedef {{Identifier}}::BasicDsp dsp;

} // namespace

#define FAUSTPP_VIRTUAL // do not declare any methods virtual
#define FAUSTPP_PRIVATE public // do not hide any members
#define FAUSTPP_PROTECTED public // do not hide any members

// define the DSP in the anonymous namespace
#define FAUSTPP_BEGIN_NAMESPACE namespace {
#define FAUSTPP_END_NAMESPACE }

enum { gOversampling = {{Oversampling}} };

static_assert(
    gOversampling == 1 || gOversampling == 2 || gOversampling == 4 ||
    gOversampling == 8 || gOversampling == 16,
    "This oversampling factor is not supported.");

{{intrinsic_code}}
{{class_code}}

//------------------------------------------------------------------------------
// End the Faust code section

#include "hiir/Upsampler2xFpu.h"
#include "hiir/Downsampler2xFpu.h"

{% if Oversampling >= 2 %}
static constexpr unsigned MaximumFrames = {{default(MaximumFrames, 512)}};

struct {{Identifier}}::Oversampler {
    struct Up {
        hiir::Upsampler2xFpu<12> f2x;
        hiir::Upsampler2xFpu<4> f4x;
        hiir::Upsampler2xFpu<3> f8x;
        hiir::Upsampler2xFpu<2> f16x;
    };
    struct Down {
        hiir::Downsampler2xFpu<2> f16x;
        hiir::Downsampler2xFpu<3> f8x;
        hiir::Downsampler2xFpu<4> f4x;
        hiir::Downsampler2xFpu<12> f2x;
    };
    Up fUpsampler[{{inputs}}];
    Down fDownsampler[{{outputs}}];
    float fWorkBuffer[({{inputs}} + {{outputs}}) * (2 * gOversampling * MaximumFrames)];
};
{% endif %}

{% if Oversampling >= 2 %}
namespace {
    static constexpr double sCoefs2x[12] = { 0.036681502163648017, 0.13654762463195794, 0.27463175937945444, 0.42313861743656711, 0.56109869787919531, 0.67754004997416184, 0.76974183386322703, 0.83988962484963892, 0.89226081800387902, 0.9315419599631839, 0.96209454837808417, 0.98781637073289585 };
    {% if Oversampling >= 4 %}static constexpr double sCoefs4x[4] = {0.041893991997656171, 0.16890348243995201, 0.39056077292116603, 0.74389574826847926 };{% endif %}
    {% if Oversampling >= 8 %}static constexpr double sCoefs8x[3] = {0.055748680811302048, 0.24305119574153072, 0.64669913119268196 };{% endif %}
    {% if Oversampling >= 16 %}static constexpr double sCoefs16x[2] = {0.10717745346023573, 0.53091435354504557 };{% endif %}
}
{% endif %}

{{Identifier}}::{{Identifier}}()
    : fDsp(new {{class_name}}){% if Oversampling >= 2 %},
      fOversampler(new Oversampler){% endif %}
{
    {{class_name}} &dsp = static_cast<{{class_name}} &>(*fDsp);
    dsp.instanceResetUserInterface();

    {% if Oversampling >= 2 %}
    for (unsigned i = 0; i < {{inputs}}; ++i) {
        Oversampler::Up &up = fOversampler->fUpsampler[i];
        {% if Oversampling >= 2 %}up.f2x.set_coefs(sCoefs2x);{% endif %}
        {% if Oversampling >= 4 %}up.f4x.set_coefs(sCoefs4x);{% endif %}
        {% if Oversampling >= 8 %}up.f8x.set_coefs(sCoefs8x);{% endif %}
        {% if Oversampling >= 16 %}up.f16x.set_coefs(sCoefs16x);{% endif %}
    }
    for (unsigned i = 0; i < {{outputs}}; ++i) {
        Oversampler::Down &down = fOversampler->fDownsampler[i];
        {% if Oversampling >= 2 %}down.f2x.set_coefs(sCoefs2x);{% endif %}
        {% if Oversampling >= 4 %}down.f4x.set_coefs(sCoefs4x);{% endif %}
        {% if Oversampling >= 8 %}down.f8x.set_coefs(sCoefs8x);{% endif %}
        {% if Oversampling >= 16 %}down.f16x.set_coefs(sCoefs16x);{% endif %}
    }
    {% endif %}
}

{{Identifier}}::~{{Identifier}}()
{
}

void {{Identifier}}::init(float sample_rate)
{
    {{class_name}} &dsp = static_cast<{{class_name}} &>(*fDsp);
    dsp.classInit(sample_rate);
    dsp.instanceConstants(sample_rate);
    dsp.instanceClear();
}

void {{Identifier}}::clear() noexcept
{
    {{class_name}} &dsp = static_cast<{{class_name}} &>(*fDsp);
    dsp.instanceClear();

    {% if Oversampling >= 2 %}
    for (unsigned i = 0; i < {{inputs}}; ++i) {
        Oversampler::Up &up = fOversampler->fUpsampler[i];
        {% if Oversampling >= 2 %}up.f2x.clear_buffers();{% endif %}
        {% if Oversampling >= 4 %}up.f4x.clear_buffers();{% endif %}
        {% if Oversampling >= 8 %}up.f8x.clear_buffers();{% endif %}
        {% if Oversampling >= 16 %}up.f16x.clear_buffers();{% endif %}
    }
    for (unsigned i = 0; i < {{outputs}}; ++i) {
        Oversampler::Down &down = fOversampler->fDownsampler[i];
        {% if Oversampling >= 2 %}down.f2x.clear_buffers();{% endif %}
        {% if Oversampling >= 4 %}down.f4x.clear_buffers();{% endif %}
        {% if Oversampling >= 8 %}down.f8x.clear_buffers();{% endif %}
        {% if Oversampling >= 16 %}down.f16x.clear_buffers();{% endif %}
    }
    {% endif %}
}

{% if Oversampling >= 2 %}
void {{Identifier}}::process(
    {% for i in range(inputs) %}const float *in{{i}},{% endfor %}
    {% for i in range(outputs) %}float *out{{i}},{% endfor %}
    unsigned count) noexcept
{
    for (unsigned index = 0; index < count; ++index) {
        unsigned left = count - index;
        unsigned segment = (left < MaximumFrames) ? left : MaximumFrames;
        const float *inputs[] = {
            {% for i in range(inputs) %}in{{i}} + index,{% endfor %}
        };
        float *outputs[] = {
            {% for i in range(outputs) %}out{{i}} + index,{% endfor %}
        };
        process_segment(inputs, outputs, segment);
        index += segment;
    }
}

void {{Identifier}}::process_segment(const float *const inputs[], float *const outputs[], unsigned count) noexcept
{
    {{class_name}} &dsp = static_cast<{{class_name}} &>(*fDsp);
    float *inputsUp[{{inputs}}];
    float *outputsUp[{{outputs}}];

    for (unsigned channel = 0; channel < {{inputs}}; ++channel) {
        Oversampler::Up &up = fOversampler->fUpsampler[channel];
        float *curr = &fOversampler->fWorkBuffer[channel * (2 * gOversampling * MaximumFrames)];
        float *temp = curr + gOversampling * MaximumFrames;
        {% if Oversampling >= 2 %}up.f2x.process_block(curr, inputs[channel], count); std::swap(curr, temp);{% endif %}
        {% if Oversampling >= 4 %}up.f4x.process_block(curr, temp, 2 * count); std::swap(curr, temp);{% endif %}
        {% if Oversampling >= 8 %}up.f8x.process_block(curr, temp, 4 * count); std::swap(curr, temp);{% endif %}
        {% if Oversampling >= 16 %}up.f16x.process_block(curr, temp, 8 * count); std::swap(curr, temp);{% endif %}
        inputsUp[channel] = temp;
    }

    for (unsigned channel = 0; channel < {{outputs}}; ++channel) {
        float *curr = &fOversampler->fWorkBuffer[(channel + {{inputs}}) * (gOversampling * 2 * MaximumFrames)];
        outputsUp[channel] = curr;
    }

    dsp.compute(gOversampling * count, inputsUp, outputsUp);

    for (unsigned channel = 0; channel < {{outputs}}; ++channel) {
        Oversampler::Down &down = fOversampler->fDownsampler[channel];
        float *curr = outputsUp[channel];
        {% if Oversampling > 2 %}float *temp = curr + gOversampling * MaximumFrames;{% endif %}
        {% if Oversampling >= 16 %}down.f16x.process_block(temp, curr, 8 * count); std::swap(curr, temp);{% endif %}
        {% if Oversampling >= 8 %}down.f8x.process_block(temp, curr, 4 * count); std::swap(curr, temp);{% endif %}
        {% if Oversampling >= 4 %}down.f4x.process_block(temp, curr, 2 * count); std::swap(curr, temp);{% endif %}
        {% if Oversampling >= 2 %}down.f2x.process_block(outputs[channel], curr, count);{% endif %}
    }
}
{% else %}
void {{Identifier}}::process(
    {% for i in range(inputs) %}const float *in{{i}},{% endfor %}
    {% for i in range(outputs) %}float *out{{i}},{% endfor %}
    unsigned count) noexcept
{
    {{class_name}} &dsp = static_cast<{{class_name}} &>(*fDsp);
    float *inputs[] = {
        {% for i in range(inputs) %}const_cast<float *>(in{{i}}),{% endfor %}
    };
    float *outputs[] = {
        {% for i in range(outputs) %}out{{i}},{% endfor %}
    };
    dsp.compute(count, inputs, outputs);
}
{% endif %}

const char *{{Identifier}}::parameter_label(unsigned index) noexcept
{
    switch (index) {
    {% for w in active %}
    case {{loop.index}}:
        return {{cstr(w.label)}};
    {% endfor %}
    default:
        return 0;
    }
}

const char *{{Identifier}}::parameter_short_label(unsigned index) noexcept
{
    switch (index) {
    {% for w in active %}
    case {{loop.index}}:
        return {{cstr(default(w.meta.abbrev,""))}};
    {% endfor %}
    default:
        return 0;
    }
}

const char *{{Identifier}}::parameter_symbol(unsigned index) noexcept
{
    switch (index) {
    {% for w in active %}
    case {{loop.index}}:
        return {{cstr(cid(default(w.meta.symbol,w.label)))}};
    {% endfor %}
    default:
        return 0;
    }
}

const char *{{Identifier}}::parameter_unit(unsigned index) noexcept
{
    switch (index) {
    {% for w in active %}
    case {{loop.index}}:
        return {{cstr(w.unit)}};
    {% endfor %}
    default:
        return 0;
    }
}

const {{Identifier}}::ParameterRange *{{Identifier}}::parameter_range(unsigned index) noexcept
{
    switch (index) {
    {% for w in active %}
    case {{loop.index}}: {
        static const ParameterRange range = { {{w.init}}, {{w.min}}, {{w.max}} };
        return &range;
    }
    {% endfor %}
    default:
        return 0;
    }
}

bool {{Identifier}}::parameter_is_trigger(unsigned index) noexcept
{
    switch (index) {
    {% for w in active %}{% if (w.type == "button" or existsIn(w.meta, "trigger")) %}
    case {{loop.index}}:
        return true;
    {% endif %}{% endfor %}
    default:
        return false;
    }
}

bool {{Identifier}}::parameter_is_boolean(unsigned index) noexcept
{
    switch (index) {
    {% for w in active %}{% if (w.type == "button" or w.type == "checkbox") or existsIn(w.meta, "boolean") %}
    case {{loop.index}}:
        return true;
    {% endif %}{% endfor %}
    default:
        return false;
    }
}

bool {{Identifier}}::parameter_is_integer(unsigned index) noexcept
{
    switch (index) {
    {% for w in active %}{% if (w.type == "button" or w.type == "checkbox") or (existsIn(w.meta, "integer") or existsIn(w.meta, "boolean")) %}
    case {{loop.index}}:
        return true;
    {% endif %}{% endfor %}
    default:
        return false;
    }
}

bool {{Identifier}}::parameter_is_logarithmic(unsigned index) noexcept
{
    switch (index) {
    {% for w in active %}{% if w.scale == "log" %}
    case {{loop.index}}:
        return true;
    {% endif %}{% endfor %}
    default:
        return false;
    }
}

float {{Identifier}}::get_parameter(unsigned index) const noexcept
{
    {{class_name}} &dsp = static_cast<{{class_name}} &>(*fDsp);
    switch (index) {
    {% for w in active %}
    case {{loop.index}}:
        return dsp.{{w.var}};
    {% endfor %}
    default:
        return 0;
    }
}

void {{Identifier}}::set_parameter(unsigned index, float value) noexcept
{
    {{class_name}} &dsp = static_cast<{{class_name}} &>(*fDsp);
    switch (index) {
    {% for w in active %}
    case {{loop.index}}:
        dsp.{{w.var}} = value;
        break;
    {% endfor %}
    default:
        (void)value;
        break;
    }
}

{% for w in active %}
float {{Identifier}}::get_{{cid(default(w.meta.symbol,w.label))}}() const noexcept
{
    {{class_name}} &dsp = static_cast<{{class_name}} &>(*fDsp);
    return dsp.{{w.var}};
}

void {{Identifier}}::set_{{cid(default(w.meta.symbol,w.label))}}(float value) noexcept
{
    {{class_name}} &dsp = static_cast<{{class_name}} &>(*fDsp);
    dsp.{{w.var}} = value;
}
{% endfor %}
