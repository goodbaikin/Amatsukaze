#!/bin/bash
VER=`git describe --tags`
sudo $(which docker) build -t goodbaikin/amatsukaze:${VER} --build-arg VERSION=${VER} .