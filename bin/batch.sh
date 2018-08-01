#!/bin/bash -x

set -e

CZINSPECT="czinspect"

usage () {
    cat<<EOF
Usage: batch.sh [operation] <in_folder>

This a wrapper script around czinspect, and will pass most options directly to
czinspect.

czinspect's help text:
---
$($CZINSPECT $opts -h $*)
---
EOF
}

error () {
    echo "$@" >&2
    exit 1
}

outdir=""
opts=""

while getopts "EShd:f:gaes:" opt; do
    case $opt in
    d)
        outdir="$OPTARG"
        ;;
    h)
        usage
        exit 0
        ;;
    E|S|a|e|g)
        opts="$opts -$opt"
        ;;
    s|f)
        opts="$opts -$opt $OPTARG"
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

find $INDIR -maxdepth 1 -name '*.czi' \
    | xargs -I'{}' sh -c "out=\"\$(echo '{}' \
        | sed 's,$indirsafe/*\(.*\).czi\$,$outdir/\1,g')\" ; \
        mkdir -p \"\$out\" ; '$CZINSPECT' $opts -d\"\$out\" '{}'"
