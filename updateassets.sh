#!/usr/bin/env bash
xmake run -w . openminecraft-bundlemaker bootassets
xxd -i res.bundle > include/openminecraft/resource/bootassets.h
