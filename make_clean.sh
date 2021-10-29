#!/bin/bash

rm -rf deps build
mos build --local --verbose --platform=esp32 && mos flash && mos console