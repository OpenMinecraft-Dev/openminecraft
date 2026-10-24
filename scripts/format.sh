#!/usr/bin/env fish
clang-format -style=file -i src/**.cpp tools/**.cpp include/**.hpp
