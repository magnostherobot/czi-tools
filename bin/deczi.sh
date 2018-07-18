#!/bin/bash

set -e

# Set these to the paths to extractjxr, JxrDecApp and the tiff to png conversion program
EXTRACTJXR="extractjxr"
JXRDECAPP="JxrDecApp"
TIFF2PNG="vips copy"

JXRFMT="-i %s -o %s"


usage () {
    cat <<EOF
Usage: deczi.sh [options] input_file

    -d <dir>    Directory to extract image tiles to (implies extraction should be performed)
    -f <level>  Specify subsampling level to filter for with extractjxr
    -s          Scan for available subsampling ratios using extractjxr
    -p          Convert extracted images to PNG after converting from JXR to TIFF
    -h          Print this help message
    
EOF
}

error () {
    echo "$@" >&2
    exit 1
}

while getopts ":d:f:hps" opt; do
    case $opt in
    d)
        outdir="$OPTARG"
        ;;
    f)
        filterlevel="$OPTARG"
        ;;
    h)
        usage
        exit 0
        ;;
    p)
        dopng=1
        ;;
    s)
        scan=1
        ;;
    esac
done

shift $((OPTIND - 1))

if (( $# == 0 )); then
    error "Missing input filename"
fi

INFILE="$1"

if [[ -z "$outdir" && -z "$scan" ]]; then
    error "Must have either -d or -s"
elif [[ -n "$outdir" && -n "$scan" ]]; then
    error "Cannot scan and extract simultaneously"
elif [[ -n "$scan" && -n "$filterlevel" ]]; then
    error "Cannot filter when scanning"
elif [[ -n "$scan" && -n "$dopng" ]]; then
    error "Cannot convert images when scanning"
fi

if [[ -n "$scan" ]]; then
    EXJXR_ARGS="-i $INFILE -s"
else
    EXJXR_ARGS="-i $INFILE -d $outdir"
fi

if [[ -n "$filterlevel" ]]; then
    EXJXR_ARGS="$EXJXR_ARGS -f $filterlevel"
fi

$EXTRACTJXR $EXJXR_ARGS

if [[ -n "$scan" ]]; then 
    exit 0
fi

pct=0
lastpct=0
count=$(ls "$outdir"/data-*.jxr | wc -l | cut -d' ' -f2)
n=1

if [[ -z "$dopng" ]]; then
    fmtstr="Converting JXR's to TIFF's: %3lu%%\r"
else
    fmtstr="Converting JXR's to TIFF's to PNG's: %3lu%%\r"
fi


printf "$fmtstr" 
for img in "$outdir"/data-*.jxr; do
    jxr="$img"
    png="${img%.jxr}.png"
    tif="${img%.jxr}.tif"

    $JXRDECAPP $(printf -- "$JXRFMT" "$jxr" "$tif")
    rm "$jxr"

    if [[ -n "$dopng" ]]; then
        $TIFF2PNG "$tif" "$png"
        rm "$tif"
    fi

    pct=$(( (n * 100) / count ))
    if (( $pct > $lastpct )); then
        lastpct=$pct
        printf "$fmtstr" "$pct"
    fi

    n=$((n + 1))
done

echo
