#!/bin/bash
VER=`git describe --abbrev=0 --tags`
sudo $(which docker) buildx build --load -t goodbaikin/amatsukaze:${VER} \
	--cache-to type=registry,ref=goodbaikin/amatsukaze:cache \
	--cache-from type=registry,ref=goodbaikin/amatsukaze:cache \
	--build-arg VERSION=${VER} .
