#!/usr/bin/env bash
xmake run -w . openminecraft-bundlemaker -c bootassets res.bundle
xxd -i res.bundle > include/openminecraft/resource/bootassets.h
rm res.bundle
