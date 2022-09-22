{% extends "generic.hpp" %}

{% block HeaderPrologue %}
{{super()}}
{% if not (Oversampling in [1, 2, 4, 8, 16]) %}
{{fail("`Oversampling` is invalid, accepted values are [1, 2, 4, 8, 16].")}}
{% endif %}
{% endblock %}

{% block ClassExtraDecls %}
{% if Oversampling >= 2 %}
private:
    void process_segment(const float *const inputs[], float *const outputs[], unsigned count) noexcept;

    struct Oversampler;
    std::unique_ptr<Oversampler> fOversampler;
{% endif %}
{% endblock %}
