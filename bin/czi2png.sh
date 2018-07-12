#!/bin/bash

set -e

INFILE=$1
OUTDIR="$2/"

mkdir -p "$OUTDIR"
./cziconvert "$INFILE" "$OUTDIR"
ls "$OUTDIR"/*.jxr | xargs -P 4 -n 1 "$PWD/jxr2png.sh"
