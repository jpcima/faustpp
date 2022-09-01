#          Copyright Jean Pierre Cimalando 2022.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# SPDX-License-Identifier: BSL-1.0

from faustpp.metadata import Metadata, WTYPE_Active, WTYPE_Passive
from faustpp.utility import cstrlit, mangle
from typing import Any, Optional, TextIO, List, Dict, Tuple
from jinja2 import Environment, FileSystemLoader
import os

class RenderFailure(Exception):
    pass

def render_metadata(out: TextIO, md: Metadata, tmplfile: str, defines: Dict[str, str]):
    tmpldir: str = os.path.dirname(tmplfile)
    env = Environment(loader=FileSystemLoader(tmpldir))
    template = env.get_template(os.path.basename(tmplfile))

    context: Dict[str, Any] = make_global_environment(md, defines)

    out.write(template.render(context))

def make_global_environment(md: Metadata, defines: Dict[str, str]) -> Dict[str, Any]:
    context: Dict[str, Any] = {}

    context["class_code"] = md.class_code;

    context["name"] = md.name;
    context["author"] = md.author;
    context["copyright"] = md.copyright;
    context["license"] = md.license;
    context["version"] = md.version;
    context["class_name"] = md.classname;
    context["file_name"] = md.filename;
    context["inputs"] = int(md.inputs);
    context["outputs"] = int(md.outputs);

    meta: Tuple[str, str]

    file_meta: Dict[str, Any] = {}
    for meta in md.metadata:
        file_meta[meta[0]] = parse_value_string(meta[1])
    context["meta"] = file_meta

    wtype: int
    for wtype in (WTYPE_Active, WTYPE_Passive):
        widget_list: List[Metadata.Widget] = []
        widget_list_obj: List[Dict[str, Any]] = []

        if wtype == WTYPE_Active:
            widget_list = md.active
        elif wtype == WTYPE_Passive:
            widget_list = md.passive

        i: int
        for i in range(len(widget_list)):
            widget: Metadata.Widget = widget_list[i]

            widget_obj: Dict[str, Any] = {}

            widget_obj["type"] = widget.typestring;
            widget_obj["id"] = widget.id;
            widget_obj["label"] = widget.label;
            widget_obj["var"] = widget.var;
            widget_obj["init"] = widget.init;
            widget_obj["min"] = widget.min;
            widget_obj["max"] = widget.max;
            widget_obj["step"] = widget.step;
            widget_obj["unit"] = widget.unit;
            widget_obj["scale"] = widget.scalestring;
            widget_obj["tooltip"] = widget.tooltip;

            widget_meta: Dict[str, Any] = {}
            for meta in widget.metadata:
                widget_meta[meta[0]] = parse_value_string(meta[1])
            widget_obj["meta"] = widget_meta

            widget_list_obj.append(widget_obj)

        if wtype == WTYPE_Active:
            context["active"] = widget_list_obj
        elif wtype == WTYPE_Passive:
            context["passive"] = widget_list_obj

    context["cstr"] = cstrlit
    context["cid"] = mangle

    def fail(msg: str):
        if len(msg) == 0:
            msg = "failure without a message";
        raise RenderFailure(msg);
    context["fail"] = fail

    key: str
    val: str
    for key, val in defines.items():
        context[key] = parse_value_string(val)

    return context;

def parse_value_string(value: str) -> Any:
    try:
        return int(value)
    except ValueError:
        pass
    try:
        return float(value)
    except ValueError:
        pass
    return value
