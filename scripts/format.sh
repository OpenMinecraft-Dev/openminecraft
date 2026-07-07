#!/bin/fish
clang-format -style=file -i src/**.cpp tools/**.cpp tests/**.cpp include/**.hpp
