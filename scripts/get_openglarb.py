#!/usr/bin/env python3
"""
从 Khronos OpenGL Registry (gl.xml) 提取所有核心 OpenGL 函数名称。
运行后打印全部函数名（按字母排序），并输出总数。
"""

import urllib.request
from xml.dom.minidom import parseString
import xml.dom.minidom

URL = "https://registry.khronos.org/OpenGL/xml/gl.xml"
print("Downloading gl.xml ...")
with urllib.request.urlopen(urllib.request.Request(URL, headers={'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36'})) as response:
    xml_data = response.read()

funcs = []
DOMTree = xml.dom.minidom.parseString(str(xml_data, encoding='utf-8'))
collection = DOMTree.documentElement
for cmd in collection.getElementsByTagName("commands")[0].getElementsByTagName("command"):
    fnc = cmd.getElementsByTagName("proto")[0].getElementsByTagName("name")[0].firstChild.data
    if not "NV" in fnc and not "ARB" in fnc and not "OES" in fnc and not "MESA" in fnc and not "EXT" in fnc and not "ATI" in fnc and not "APPLE" in fnc and not "AMD" in fnc and not "INTEL" in fnc and not "IBM" in fnc and not "QCOM" in fnc and not "IMG" in fnc and not "SUN" in fnc and not "SGI" in fnc and not "GREMEDY" in fnc and not "ANGLE" in fnc and not "KHR" in fnc and not "OVR" in fnc and not "INGR" in fnc and not "HP" in fnc and not "PGI" in fnc and not "3DFX" in fnc and not "ARM" in fnc:
        funcs.append(fnc)

print(len(funcs), "Funcs")
