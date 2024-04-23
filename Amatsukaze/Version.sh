#!/bin/bash
VER=`git describe --tags`
echo "#define AMATSUKAZE_VERSION \"${VER}\"" > Version.h

VER=`git describe --abbrev=0 --tags`
echo "#define AMATSUKAZE_PRODUCTVERSION \"${VER}\"" >> Version.h