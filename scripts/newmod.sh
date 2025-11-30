#!/usr/bin/env bash
echo include/openminecraft/$1
mkdir include/openminecraft/$1
echo src/$1
mkdir src/$1
echo "target(\"openminecraft-$1\")" > src/$1/xmake.lua
echo "set_kind(\"static\")" >> src/$1/xmake.lua
echo "add_files(\"**.cpp\")" >> src/$1/xmake.lua
echo "add_includedirs(\"../../include\")" >> src/$1/xmake.lua
