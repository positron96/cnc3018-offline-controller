#!/bin/sh

for f in images/*.png; do ff=`basename $f`; convert $f ${ff%.*}.XBM; done
