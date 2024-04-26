#!/bin/bash
VER=`git describe --abbrev=0 --tags`
sudo $(which docker) build -t goodbaikin/amatsukaze:${VER} --build-arg VERSION=${VER} .