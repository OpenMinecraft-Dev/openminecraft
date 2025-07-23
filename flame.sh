#!/usr/bin/env bash

sudo perf record -F 256 -g --call-graph dwarf -- $1
sudo perf script | ../FlameGraph/stackcollapse-perf.pl > out.folded
../FlameGraph/flamegraph.pl --color=java out.folded > flame.svg

rm out.folded
rm perf.data