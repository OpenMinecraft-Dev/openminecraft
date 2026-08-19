#!/usr/bin/env bash
cd bootassets
zip -9 -r boot.bundle .
mv boot.bundle ..
cd ..
cd externalassets
zip -9 -r external.bundle .
mv external.bundle ..
cd ..