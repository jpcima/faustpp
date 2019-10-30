{% extends "generic.cpp" %}

{% block ImplementationDescription %}
{{super()}}
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
{% endblock %}

{% block ImplementationPrologue %}
{{super()}}
{% if not (Oversampling in [1, 2, 4, 8, 16]) %}
{{fail("`Oversampling` is invalid, accepted values are [1, 2, 4, 8, 16].")}}
{% endif %}
{% endblock %}

{% block ImplementationIncludeExtra %}
{{super()}}
{% if Oversampling != 1 %}
#include "hiir/Upsampler2xFpu.h"
#include "hiir/Downsampler2xFpu.h"
{% endif %}
{% endblock %}

{% block ImplementationFaustCode %}
enum { gOversampling = {{Oversampling}} };
{{super()}}
{% endblock %}

{% block ImplementationBeforeClassDefs %}
{{super()}}
{% if Oversampling != 1 %}
static constexpr unsigned MaximumFrames = {{MaximumFrames|default(512)}};

struct {{Identifier}}::Oversampler {
    struct Up {
        hiir::Upsampler2xFpu<12> f2x;
        {% if Oversampling >= 4 %}hiir::Upsampler2xFpu<4> f4x;{% endif %}
        {% if Oversampling >= 8 %}hiir::Upsampler2xFpu<3> f8x;{% endif %}
        {% if Oversampling >= 16 %}hiir::Upsampler2xFpu<2> f16x;{% endif %}
    };
    struct Down {
        {% if Oversampling >= 16 %}hiir::Downsampler2xFpu<2> f16x;{% endif %}
        {% if Oversampling >= 8 %}hiir::Downsampler2xFpu<3> f8x;{% endif %}
        {% if Oversampling >= 4 %}hiir::Downsampler2xFpu<4> f4x;{% endif %}
        hiir::Downsampler2xFpu<12> f2x;
    };
    Up fUpsampler[{{inputs}}];
    Down fDownsampler[{{outputs}}];
    float fWorkBuffer[({{inputs + outputs}}) * (2 * gOversampling * MaximumFrames)];
};

namespace {
    static constexpr double sCoefs2x[12] = { 0.036681502163648017, 0.13654762463195794, 0.27463175937945444, 0.42313861743656711, 0.56109869787919531, 0.67754004997416184, 0.76974183386322703, 0.83988962484963892, 0.89226081800387902, 0.9315419599631839, 0.96209454837808417, 0.98781637073289585 };
    {% if Oversampling >= 4 %}static constexpr double sCoefs4x[4] = {0.041893991997656171, 0.16890348243995201, 0.39056077292116603, 0.74389574826847926 };{% endif %}
    {% if Oversampling >= 8 %}static constexpr double sCoefs8x[3] = {0.055748680811302048, 0.24305119574153072, 0.64669913119268196 };{% endif %}
    {% if Oversampling >= 16 %}static constexpr double sCoefs16x[2] = {0.10717745346023573, 0.53091435354504557 };{% endif %}
}
{% endif %}
{% endblock %}

{% block ImplementationSetupDsp %}
    {{super()}}
{% if Oversampling != 1 %}
    Oversampler *ovs = new Oversampler;
    fOversampler.reset(ovs);
    for (unsigned i = 0; i < {{inputs}}; ++i) {
        Oversampler::Up &up = ovs->fUpsampler[i];
        up.f2x.set_coefs(sCoefs2x);
        {% if Oversampling >= 4 %}up.f4x.set_coefs(sCoefs4x);{% endif %}
        {% if Oversampling >= 8 %}up.f8x.set_coefs(sCoefs8x);{% endif %}
        {% if Oversampling >= 16 %}up.f16x.set_coefs(sCoefs16x);{% endif %}
    }
    for (unsigned i = 0; i < {{outputs}}; ++i) {
        Oversampler::Down &down = ovs->fDownsampler[i];
        down.f2x.set_coefs(sCoefs2x);
        {% if Oversampling >= 4 %}down.f4x.set_coefs(sCoefs4x);{% endif %}
        {% if Oversampling >= 8 %}down.f8x.set_coefs(sCoefs8x);{% endif %}
        {% if Oversampling >= 16 %}down.f16x.set_coefs(sCoefs16x);{% endif %}
    }
{% endif %}
{% endblock %}

{% block ImplementationClearDsp %}
    {{super()}}
{% if Oversampling != 1 %}
    for (unsigned i = 0; i < {{inputs}}; ++i) {
        Oversampler::Up &up = fOversampler->fUpsampler[i];
        up.f2x.clear_buffers();
        {% if Oversampling >= 4 %}up.f4x.clear_buffers();{% endif %}
        {% if Oversampling >= 8 %}up.f8x.clear_buffers();{% endif %}
        {% if Oversampling >= 16 %}up.f16x.clear_buffers();{% endif %}
    }
    for (unsigned i = 0; i < {{outputs}}; ++i) {
        Oversampler::Down &down = fOversampler->fDownsampler[i];
        down.f2x.clear_buffers();
        {% if Oversampling >= 4 %}down.f4x.clear_buffers();{% endif %}
        {% if Oversampling >= 8 %}down.f8x.clear_buffers();{% endif %}
        {% if Oversampling >= 16 %}down.f16x.clear_buffers();{% endif %}
    }
{% endif %}
{% endblock %}

{% block ImplementationProcessDsp %}
{% if Oversampling != 1 %}
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
{% else %}
    {{super()}}
{% endif %}
{% endblock %}

{% block ImplementationEpilogue %}
{% if Oversampling != 1 %}
void {{Identifier}}::process_segment(const float *const inputs[], float *const outputs[], unsigned count) noexcept
{
    {{class_name}} &dsp = static_cast<{{class_name}} &>(*fDsp);
    float *inputsUp[{{inputs}}];
    float *outputsUp[{{outputs}}];

    for (unsigned channel = 0; channel < {{inputs}}; ++channel) {
        Oversampler::Up &up = fOversampler->fUpsampler[channel];
        float *curr = &fOversampler->fWorkBuffer[channel * (2 * gOversampling * MaximumFrames)];
        float *temp = curr + gOversampling * MaximumFrames;
        up.f2x.process_block(curr, inputs[channel], count); std::swap(curr, temp);
        {% if Oversampling >= 4 %}up.f4x.process_block(curr, temp, 2 * count); std::swap(curr, temp);{% endif %}
        {% if Oversampling >= 8 %}up.f8x.process_block(curr, temp, 4 * count); std::swap(curr, temp);{% endif %}
        {% if Oversampling >= 16 %}up.f16x.process_block(curr, temp, 8 * count); std::swap(curr, temp);{% endif %}
        inputsUp[channel] = temp;
    }

    for (unsigned channel = 0; channel < {{outputs}}; ++channel) {
        float *curr = &fOversampler->fWorkBuffer[(channel + {{inputs}}) * (2 * gOversampling * MaximumFrames)];
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
        down.f2x.process_block(outputs[channel], curr, count);
    }
}
{% endif %}
{% endblock %}
