#          Copyright Jean Pierre Cimalando 2022.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# SPDX-License-Identifier: BSL-1.0

from faustpp.call_faust import FaustVersion, ensure_faust_version, FaustResult, call_faust
from faustpp.metadata import Metadata, extract_metadata
from faustpp.render import render_metadata
from argparse import ArgumentParser, Namespace
from typing import Optional, TextIO, List, Dict
from tempfile import NamedTemporaryFile
from contextlib import ExitStack
import os
import sys

class CmdArgs:
    tmplfile: str
    outfile: Optional[str]
    dspfile: str
    defines: Dict[str, str]
    faustargs: List[str]

class CmdError(Exception):
    pass

def main(args=sys.argv):
    with ExitStack() as cleanup_stack:
        cmd: CmdArgs = do_cmdline(args)

        ensure_faust_version(FaustVersion(0, 9, 85))

        with NamedTemporaryFile('w', suffix='.cpp') as mdfile:
            mdfile.write("""<<<<BeginFaustClass>>>>
<<includeIntrinsic>>
<<includeclass>>
<<<<EndFaustClass>>>>""")
            mdfile.flush()

            mdargs: List[str] = cmd.faustargs + ['-a', mdfile.name]
            mdresult: FaustResult = call_faust(cmd.dspfile, mdargs)

            md: Metadata = extract_metadata(mdresult.docmd, mdresult.cppsource)

        md.filename = os.path.basename(cmd.dspfile)

        #
        success = False

        out: TextIO
        if cmd.outfile is None:
            out = sys.stdout
        else:
            out = open(cmd.outfile, 'w')

        def out_cleanup():
            if not success and cmd.outfile is not None:
                os.unlink(cmd.outfile)
        cleanup_stack.callback(out_cleanup)

        render_metadata(out, md, cmd.tmplfile, cmd.defines)

        out.flush()

        success = True

def do_cmdline(args: List[str]) -> CmdArgs:
    parser: ArgumentParser = ArgumentParser(description='A post-processor for the faust compiler')
    parser.add_argument(('-a'), metavar='tmplfile', dest='tmplfile', help='architecture file')
    parser.add_argument(('-o'), metavar='outfile', dest='outfile', help='output file')
    parser.add_argument(('-D'), metavar='defines', dest='defines', action='append', help='definition, in the form name=value')
    parser.add_argument(('-X'), metavar='faustargs', dest='faustargs', action='append', help='extra faust compiler argument')
    parser.add_argument('dspfile', help='source file')

    result: Namespace = parser.parse_args(args[1:])

    if result.tmplfile is None:
        raise CmdError("No architecture file has been specified.\n")

    cmd = CmdArgs()

    cmd.tmplfile = result.tmplfile
    cmd.outfile = result.outfile
    cmd.dspfile = result.dspfile
    cmd.defines = {}
    cmd.faustargs = []

    if result.defines is not None:
        defi: str
        for defi in result.defines:
            try:
                idx: int = defi.index('=')
            except ValueError:
                raise CmdError("The definition is malformed.\n")
            cmd.defines[defi[:idx]] = defi[idx+1:]

    return cmd

def find_template_file(name: str) -> str:
    #TODO implement the file search... 
    return name
