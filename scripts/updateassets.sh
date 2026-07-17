#!/usr/bin/env bash
xmake run -w . openminecraft-bundlemaker -a Cyrene -c -t boot.bundle -P bootassets
xmake run -w . openminecraft-bundlemaker -r -t boot.bundle
# xxd -i res.bundle > include/openminecraft/resource/bootassets.h
# rm res.bundle
