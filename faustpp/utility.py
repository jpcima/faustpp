#          Copyright Jean Pierre Cimalando 2022.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# SPDX-License-Identifier: BSL-1.0

import xml.etree.ElementTree as ET
from typing import Optional

def parse_cfloat(text: str):
    if len(text) > 0 and text[-1] in 'fF':
        text = text[:-1]
    return float(text)

def safe_element_text(elt: Optional[ET.Element], default_value: str = '') -> str:
    if elt is None:
        return default_value
    text: Optional[str] = elt.text
    if text is None:
        return default_value
    return text

def safe_element_attribute(elt: Optional[ET.Element], name: str, default_value: str = '') -> str:
    if elt is None:
        return default_value
    text: Optional[str] = elt.attrib.get(name)
    if text is None:
        return default_value
    return text

def is_decint_string(str: str) -> bool:
    if str.startswith('-'):
        str = str[1:]

    if len(str) == 0:
        return False

    c: int
    for c in map(ord, str):
        if c < ord('0') or c > ord('9'):
            return False

    return True;

def parse_cstrlit(lit: str) -> str:
    src: bytes = lit.encode('utf-8')

    n: int = len(src)
    if n < 2 or src[0] != ord('"') or src[n - 1] != ord('"'):
        raise ValueError('Invalid C string literal')

    dst = bytearray()
    for i in range(1, n - 1):
        c: int = src[i]
        if c != ord('\\'):
            dst.append(c)
        else:
            if ++i == n - 1:
                raise ValueError('Invalid C string literal')
            c = src[i];
            if c == ord('0'): dst.append(ord('\0'))
            elif c == ord('a'): dst.append(ord('\a'))
            elif c == ord('b'): dst.append(ord('\b'))
            elif c == ord('t'): dst.append(ord('\t'))
            elif c == ord('n'): dst.append(ord('\n'))
            elif c == ord('v'): dst.append(ord('\v'))
            elif c == ord('f'): dst.append(ord('\f'))
            elif c == ord('r'): dst.append(ord('\r'))
            else: dst.append(c)

    return dst.decode('utf-8')

def cstrlit(text: str) -> str:
    lit = bytearray()

    #lit.append(ord('u'))
    #lit.append(ord('8'))
    lit.append(ord('"'))

    c: int
    for c in map(ord, text):
        if c == ord('\a'): lit.append(ord("\\")); lit.append(ord("a"))
        elif c == ord('\b'): lit.append(ord("\\")); lit.append(ord("b"))
        elif c == ord('\t'): lit.append(ord("\\")); lit.append(ord("t"))
        elif c == ord('\n'): lit.append(ord("\\")); lit.append(ord("n"))
        elif c == ord('\v'): lit.append(ord("\\")); lit.append(ord("v"))
        elif c == ord('\f'): lit.append(ord("\\")); lit.append(ord("f"))
        elif c == ord('\r'): lit.append(ord("\\")); lit.append(ord("r"))
        else:
            if c in (ord('"'), ord("\\")): lit.append(ord("\\"))
            lit.append(c)

    lit.append(ord('"'))
    return lit.decode('utf-8')

def mangle(name: str) -> str:
    src: bytes

    n: int = len(name)
    if n > 0:
        src = name.encode('utf-8')
        n = len(src)
    else:
        src = b"_"
        n = 1

    id = bytearray()

    c: int
    c = src[0]
    if c >= ord('0') and c <= ord('9'):
        id.append(ord('_'))

    for i in range(n):
        c = src[i]
        al: bool = (c >= ord('a') and c <= ord('z')) or (c >= ord('A') and c <= ord('Z'))
        num: bool = c >= ord('0') and c <= ord('9')
        if not al and not num:
            c = ord('_')
        id.append(c);

    return id.decode('utf-8')
