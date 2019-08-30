#include "{{Identifier}}.hpp"

//------------------------------------------------------------------------------

#include <jack/jack.h>
#include <unistd.h>
#include <cstdio>

struct JackAudioContext {
    jack_client_t *client;
    jack_port_t *port_in[{{inputs}}];
    jack_port_t *port_out[{{outputs}}];
    {{Identifier}} dsp;
};

static int process(jack_nframes_t count, void *userdata)
{
    JackAudioContext *jack = (JackAudioContext *)userdata;
    jack->dsp.process(
        {% for i in range(inputs) %}(float *)jack_port_get_buffer(jack->port_in[{{i}}], count),{% endfor %}
        {% for i in range(outputs) %}(float *)jack_port_get_buffer(jack->port_out[{{i}}], count),{% endfor %}
        count);
    return 0;
}

int main()
{
    JackAudioContext jack;

    jack.client = jack_client_open({{cstr(name)}}, JackNoStartServer, nullptr);
    if (!jack.client) {
        fprintf(stderr, "Cannot open JACK client.\n");
        return 1;
    }

    {% if inputs > 0 %}
    if (!(
        {% for i in range(inputs) %}
        (jack.port_in[{{i}}] = jack_port_register(jack.client, "in_{{i}}", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0))
        {% if not loop.is_last %}&&{% endif %}
        {% endfor %})) {
        fprintf(stderr, "Cannot register JACK inputs.\n");
        jack_client_close(jack.client);
        return 1;
    }
    {% endif %}

    {% if outputs > 0 %}
    if (!(
        {% for i in range(outputs) %}
        (jack.port_out[{{i}}] = jack_port_register(jack.client, "out_{{i}}", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0))
        {% if not loop.is_last %}&&{% endif %}
        {% endfor %})) {
        fprintf(stderr, "Cannot register JACK outputs.\n");
        jack_client_close(jack.client);
        return 1;
    }
    {% endif %}

    jack_set_process_callback(jack.client, &process, &jack);

    jack.dsp.init(jack_get_sample_rate(jack.client));

    if (jack_activate(jack.client) != 0) {
        fprintf(stderr, "Cannot activate JACK client.\n");
        jack_client_close(jack.client);
        return 1;
    }

    for (;;)
        pause();

    return 0;
}
