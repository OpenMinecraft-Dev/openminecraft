#!/usr/bin/env python3

import urllib.request
from xml.dom.minidom import parseString
import xml.dom.minidom

URL = "https://registry.khronos.org/OpenGL/xml/gl.xml"
with urllib.request.urlopen(urllib.request.Request(URL, headers={'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36'})) as response:
    xml_data = response.read()

funcs = []
DOMTree = xml.dom.minidom.parseString(str(xml_data, encoding='utf-8'))
collection = DOMTree.documentElement

removed = []

for fea in collection.getElementsByTagName("feature"):
    if fea.getAttribute("api") == "gl":
        for rm in fea.getElementsByTagName("remove"):
            for cmd in rm.getElementsByTagName("command"):
                removed.append(cmd.getAttribute("name"))

removed = list(set(removed))

for fea in collection.getElementsByTagName("feature"):
    if fea.getAttribute("api") == "gl":
        for rm in fea.getElementsByTagName("require"):
            if not rm.hasAttribute("profile") or rm.getAttribute("profile") != "compatibility":
                for cmd in rm.getElementsByTagName("command"):
                    fnc = cmd.getAttribute("name")
                    if not fnc in removed:
                        funcs.append(fnc)

for fea in collection.getElementsByTagName("extension"):
    if fea.getAttribute("supported") == "gl|glcore":
        for rm in fea.getElementsByTagName("require"):
            if not rm.hasAttribute("profile") or rm.getAttribute("profile") != "compatibility":
                for cmd in rm.getElementsByTagName("command"):
                    fnc = cmd.getAttribute("name")
                    funcs.append(fnc)

funcs = list(set(funcs))

print("#ifdef OM_OGL_DEF")
for f in funcs:
    print(f"PFN{f.upper()}PROC {f};")
print("#else")
for f in funcs:
    print(f"gl.{f} = fetchGlFunc<PFN{f.upper()}PROC>(\"{f}\");")
print("#endif")
