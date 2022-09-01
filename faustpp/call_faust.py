#          Copyright Jean Pierre Cimalando 2022.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# SPDX-License-Identifier: BSL-1.0

from faustpp.utility import parse_cstrlit, safe_element_text
from typing import Optional, List, Dict
from subprocess import run, CompletedProcess, PIPE
from tempfile import TemporaryDirectory
import xml.etree.ElementTree as ET
import re
import os
import sys

class FaustVersion:
    maj: int
    min: int
    pat: int

    def __init__(self, maj: int, min: int, pat: int):
        self.maj = maj
        self.min = min
        self.pat = pat

    def __lt__(self, other: 'FaustVersion'):
        return (self.maj, self.min, self.pat) < (other.maj, other.min, other.pat)

    def __str__(self):
        return '%d.%d.%d' % (self.maj, self.min, self.pat)

FAUST_COMMAND_: Optional[str] = None

def get_faust_command() -> str:
    global FAUST_COMMAND_
    cmd: Optional[str] = FAUST_COMMAND_
    if cmd is None:
        cmd = os.getenv('FAUST')
        if cmd is None:
            cmd = 'faust'
        FAUST_COMMAND_ = cmd
    return cmd

def get_faust_version() -> FaustVersion:
    cmd: List[str] = [get_faust_command(), '--version']
    proc: CompletedProcess = run(cmd, stdout=PIPE)
    proc.check_returncode()

    reg = re.compile(r'(\d+)\.(\d+).(\d+)')
    mat = reg.search(proc.stdout.decode('utf-8'))
    if mat is None:
        raise ValueError('Cannot extract the version of faust.')

    ver = FaustVersion(int(mat.group(1)), int(mat.group(2)), int(mat.group(3)))
    return ver

class FaustVersionError(Exception):
    pass

def ensure_faust_version(minver: FaustVersion):
    ver: FaustVersion = get_faust_version()
    if ver < minver:
        msg = "The Faust version %s is too old, the requirement is %s.\n" % (ver, minver)
        raise FaustVersionError(msg)

class FaustResult:
    cppsource: str
    docmd: ET.ElementTree

def call_faust(dspfile: str, faustargs: List[str]) -> FaustResult:
    workdir = TemporaryDirectory()

    dspfilebase = os.path.basename(dspfile)
    xmlfilebase = dspfilebase + '.xml'
    cppfilebase = dspfilebase + '.cpp'
    xmlfile = os.path.join(workdir.name, xmlfilebase)
    cppfile = os.path.join(workdir.name, cppfilebase)

    fargv: List[str] = [
        get_faust_command(),
        "-O", workdir.name,
        "-o", cppfilebase,
        "-xml",
        dspfile,
    ] + faustargs

    proc: CompletedProcess = run(fargv)
    proc.check_returncode()

    cppsource = open(cppfile, 'r').read()
    docmd = ET.parse(xmlfile)
    cppsource = apply_workarounds(cppsource, docmd)

    result = FaustResult()
    result.cppsource = cppsource
    result.docmd = docmd
    return result

def apply_workarounds(cppsource: str, docmd: ET.ElementTree):
    line: str
    lines: List[str]

    # fix missing <meta>
    has_meta: bool = docmd.find('meta') is not None
    if not has_meta:
        lines = cppsource.splitlines()

        widget_nodes: Dict[str, ET.Element] = {}

        for elt in docmd.findall('./ui/activewidgets/widget'):
            widget_nodes[safe_element_text(elt.find('varname'))] = elt
        for elt in docmd.findall('./ui/passivewidgets/widget'):
            widget_nodes[safe_element_text(elt.find('varname'))] = elt

        for line in lines:
            REG_STRLIT = r'"(?:\\.|[^"\\])*"'
            REG_IDENT = r"[a-zA-Z_][0-9a-zA-Z_]*"

            reg_global = re.compile(
                r"^\s*m->declare\((" + REG_STRLIT + r"), (" + REG_STRLIT + r")\);");
            reg_control = re.compile(
                r"^\s*ui_interface->declare\(&(" + REG_IDENT + r"), "
                r"(" + REG_STRLIT + r"), (" + REG_STRLIT + r")\);");

            mat = reg_global.match(line)
            sub: ET.Element

            if mat is not None:
                key = parse_cstrlit(mat.group(1))
                value = parse_cstrlit(mat.group(2))
                sub = ET.SubElement(docmd.getroot(), 'meta', {'key': key})
                sub.text = value
                continue

            mat = reg_control.match(line)
            if mat is not None:
                varname = mat.group(1)
                key = parse_cstrlit(mat.group(2))
                value = parse_cstrlit(mat.group(3))
                sub = ET.SubElement(widget_nodes[varname], 'meta', {'key': key})
                sub.text = value
                continue

    # fix visibility keywords
    reg_private = re.compile(r"^(\s*)private(\s*:.*)$");
    reg_protected = re.compile(r"^(\s*)protected(\s*:.*)$");
    reg_virtual = re.compile(r"^(\s*)virtual([ \t].*$|$)");

    lines = cppsource.splitlines()
    cppsource = ''
    for line in lines:
        mat = reg_private.match(line)
        if mat is not None:
            cppsource += '%sFAUSTPP_PRIVATE%s\n' % (mat.group(1), mat.group(2))
            continue
        mat = reg_protected.match(line)
        if mat is not None:
            cppsource += '%sFAUSTPP_PROTECTED%s\n' % (mat.group(1), mat.group(2))
            continue
        mat = reg_virtual.match(line)
        if mat is not None:
            cppsource += '%sFAUSTPP_VIRTUAL%s\n' % (mat.group(1), mat.group(2))
            continue
        cppsource += line
        cppsource += '\n'

    return cppsource
