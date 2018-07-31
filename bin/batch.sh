#!/bin/bash

set -e

DECZI="deczi.sh"

usage () {
    cat<<EOF
Usage: $0 [options] input_folder

    Options:
    -d <dir>   Directory to extract image tiles to
    -f <level> Specify subsampling level to filter for
    -p         Convert extracted images to PNG after converting from JXR to TIFF
    -h         Print this help message
EOF
}

error () {
    echo "$@" >&2
    exit 1
}

while getopts ":d:f:ph" opt; do
    case $opt in
    d)
        outdir="$OPTARG"
        ;;
    f)
        decziopts="$decziopts -$opt $OPTARG"
        ;;
    p)
        decziopts="$decziopts -$opt"
        ;;
    h)
        usage
        exit 0
        ;;
    esac
done

shift $((OPTIND - 1))

if (( $# == 0 )); then
    error "Missing input directory"
fi

if [[ -z "$outdir" ]]; then
    error "Missing output directory"
fi

INDIR="$1"
indirsafe="$(echo $INDIR | sed 's/[[\.*^$,]/\\&/g')"

find $INDIR -name '*.czi' \
    | xargs -I'{}' sh -c "out=\"\$(echo '{}' \
        | sed 's,$indirsafe/*\(.*\).czi\$,$outdir/\1,g')\" ; \
        mkdir -p \"\$out\" ; '$DECZI' -d\"\$out\" '{}'"
