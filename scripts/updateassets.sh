#!/usr/bin/env bash
cd bootassets
zip -r boot.bundle .
mv boot.bundle ..
cd ..
cd externalassets
zip -r external.bundle .
mv external.bundle ..
cd ..