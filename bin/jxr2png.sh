#!/bin/bash

set -e

JXR=$1
TIF="${1%.*}.tif"
PNG="${1%.*}.png"

./JxrDecApp -i $JXR -o $TIF
convert $TIF $PNG
echo "Done with $JXR -> $TIF -> $PNG"
rm $TIF $JXR
