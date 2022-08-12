#          Copyright Jean Pierre Cimalando 2022.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# SPDX-License-Identifier: BSL-1.0

from faustpp.utility import safe_element_text, safe_element_attribute, is_decint_string, parse_cfloat
import xml.etree.ElementTree as ET
import re

WIDGET_Button = 0
WIDGET_CheckBox = 1
WIDGET_VSlider = 2
WIDGET_HSlider = 3
WIDGET_NEntry = 4
WIDGET_VBarGraph = 5
WIDGET_HBarGraph = 6

WTYPE_Active = 0
WTYPE_Passive = 1

SCALE_Linear = 0
SCALE_Log = 1
SCALE_Exp = 2

class Metadata:
    name : str
    author : str
    copyright : str
    license : str
    version : str
    classname : str
    filename : str
    metadata = list[tuple[str, str]]
    inputs : int
    outputs : int

    class Widget:
        type: int
        typestring: str
        id: int
        label: str
        var: str
        #symbol: str
        init: float
        min: float
        max: float
        step: float
        metadata: list[tuple[str, str]]

        # metadata interpretation
        unit: str
        scale: int
        scalestring: str
        tooltip: str

    active: list[Widget];
    passive: list[Widget];

    class_code: str

def extract_metadata(doc: ET.ElementTree, mdsource: str) -> Metadata:
    root: ET.Element = doc.getroot()

    md = Metadata()
    md.name = safe_element_text(root.find('name'))
    md.author = safe_element_text(root.find("author"))
    md.copyright = safe_element_text(root.find("copyright"))
    md.license = safe_element_text(root.find("license"))
    md.version = safe_element_text(root.find("version"))
    md.classname = safe_element_text(root.find("classname"))
    md.filename = ""
    md.inputs = int(safe_element_text(root.find("inputs"), '0'))
    md.outputs = int(safe_element_text(root.find("outputs"), '0'))

    md.metadata = []
    meta: ET.Element
    for meta in root.findall('meta'):
        key: str = safe_element_attribute(meta, "key")
        value: str = safe_element_text(meta)
        md.metadata.append((key, value))

    md.active = []
    for elt in doc.findall('./ui/activewidgets/widget'):
        md.active.append(extract_widget(elt, True, md))

    md.passive = []
    for elt in doc.findall('./ui/passivewidgets/widget'):
        md.passive.append(extract_widget(elt, False, md))

    ###
    ccode: str = ''
    is_in_class: bool = False

    # add our warning suppressions
    ccode += \
        "#if defined(__GNUC__)" "\n" \
        "#   pragma GCC diagnostic push" "\n" \
        "#   pragma GCC diagnostic ignored \"-Wunused-parameter\"" "\n" \
        "#endif" "\n" \
        "\n"

    # add our overrides for source keywords
    ccode += \
        "#ifndef FAUSTPP_PRIVATE" "\n" \
        "#   define FAUSTPP_PRIVATE private" "\n" \
        "#endif" "\n" \
        "#ifndef FAUSTPP_PROTECTED" "\n" \
        "#   define FAUSTPP_PROTECTED protected" "\n" \
        "#endif" "\n" \
        "#ifndef FAUSTPP_VIRTUAL" "\n" \
        "#   define FAUSTPP_VIRTUAL virtual" "\n" \
        "#endif" "\n" \
        "\n"

    # add our namespace definitions
    ccode += \
        "#ifndef FAUSTPP_BEGIN_NAMESPACE" "\n" \
        "#   define FAUSTPP_BEGIN_NAMESPACE" "\n" \
        "#endif" "\n" \
        "#ifndef FAUSTPP_END_NAMESPACE" "\n" \
        "#   define FAUSTPP_END_NAMESPACE" "\n" \
        "#endif" "\n" \
        "\n"

    # open the namespace
    ccode += \
        "FAUSTPP_BEGIN_NAMESPACE" "\n" \
        "\n"
    is_in_namespace : bool = True

    reg_include = re.compile("^\\s*#\\s*include\\s+(.*)$")

    for line in mdsource.splitlines():
        if is_in_class:
            if line == "<<<<EndFaustClass>>>>":
                is_in_class = False
            else:
                # make sure not to enclose #include in the namespace
                is_include: bool = reg_include.match(line) is not None
                if is_include and is_in_namespace:
                    ccode += "FAUSTPP_END_NAMESPACE" "\n";
                    is_in_namespace = False;
                elif not is_include and not is_in_namespace:
                    ccode += "FAUSTPP_BEGIN_NAMESPACE" "\n"
                    is_in_namespace = True;

                ccode += line;
                ccode += '\n';
        elif line == "<<<<BeginFaustClass>>>>":
            is_in_class = True

    # close the namespace
    if is_in_namespace:
        ccode += \
            "FAUSTPP_END_NAMESPACE" "\n" \
            "\n"

    #  add our warning suppressions
    ccode += \
        "\n" \
        "#if defined(__GNUC__)" "\n" \
        "#   pragma GCC diagnostic pop" "\n" \
        "#endif" "\n"

    md.class_code = ccode

    return md

def extract_widget(node: ET.Element, is_active: bool, md: Metadata) -> Metadata.Widget:
    w = Metadata.Widget()
    w.typestring = safe_element_attribute(node, "type")
    w.type = widget_type_from_name(w.typestring)

    w.id = int(safe_element_attribute(node, "id", "0"))
    w.label = safe_element_text(node.find("label"))
    w.var = safe_element_text(node.find("varname"))

    #w.symbol = mangle(w.label);

    w.init = 0
    w.min = 0
    w.max = 0
    w.step = 0
    w.metadata = []
    w.unit = ""
    w.scale = SCALE_Linear
    w.scalestring = ""
    w.tooltip = ""

    if is_active and w.type in (WIDGET_HSlider, WIDGET_VSlider, WIDGET_NEntry):
        w.init = parse_cfloat(safe_element_text(node.find("init"), "0"))
        w.min = parse_cfloat(safe_element_text(node.find("min"), "0"))
        w.max = parse_cfloat(safe_element_text(node.find("max"), "0"))
        w.step = parse_cfloat(safe_element_text(node.find("step"), "0"))
    elif is_active and w.type in (WIDGET_Button, WIDGET_CheckBox):
        w.init = 0
        w.min = 0
        w.max = 1
        w.step = 1
    elif not is_active and w.type in (WIDGET_VBarGraph, WIDGET_HBarGraph):
        w.min = parse_cfloat(safe_element_text(node.find("min"), "0"))
        w.max = parse_cfloat(safe_element_text(node.find("max"), "0"))
    else:
        raise ValueError("Unsupported widget type")

    meta: ET.Element
    for meta in node.findall('./meta'):
        key: str = safe_element_attribute(meta, "key")
        value: str = safe_element_text(meta)
        if is_decint_string(key) and len(value) == 0:
            continue
        w.metadata.append((key, value))

        if key == "unit":
            w.unit = value;
        elif key == "scale":
            w.scalestring = value
            w.scale = widget_scale_from_name(value)
        elif key == "tooltip":
            w.tooltip = value
        #elif (key == "md.symbol")
        #    w.symbol = mangle(value);

    return w

def widget_type_from_name(name: str) -> int:
    if name == "button":
        return WIDGET_Button
    elif name == "checkbox":
        return WIDGET_CheckBox
    elif name == "vslider":
        return WIDGET_VSlider
    elif name == "hslider":
        return WIDGET_HSlider
    elif name == "nentry":
        return WIDGET_NEntry
    elif name == "vbargraph":
        return WIDGET_VBarGraph
    elif name == "hbargraph":
        return WIDGET_HBarGraph
    else:
        raise ValueError("Invalid widget type name")

def widget_scale_from_name(name: str) -> int:
    if name == "log":
        return SCALE_Log
    elif name == "exp":
        return SCALE_Exp
    else:
        raise ValueError("Invalid widget scale name")
