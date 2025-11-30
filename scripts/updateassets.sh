#!/usr/bin/env bash
xmake run -w . openminecraft-bundlemaker -a Cyrene -c -t res.bundle -P bootassets
xxd -i res.bundle > include/openminecraft/resource/bootassets.h
rm res.bundle
