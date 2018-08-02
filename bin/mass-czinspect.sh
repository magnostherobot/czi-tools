#!/bin/bash

set -e

CZINSPECT="czinspect"
JXRCONVERT="mass-jxr-convert.sh -P1"

usage () {
    cat<<EOF
Usage: mass-czinspect.sh [operation] <in_folder>

This a wrapper script around a czi manipulation tool, and will pass most options
directly to said tool.

mass-czinspect's options:
    -d          <dir> Output directory for output(s) of czinspect.
    -P          Number of maximum concurrent czinspect processes; passed
                directly to xargs if used
    -v          Verbosity of output (default is 0 - silent)
    -x          Remove .czi files after extraction (caution!)
    -t <suffix> Specify output image type (default is '.jxr')
    -v          Verbosity of output (default is 0 - silent)

czi manipulation tool in use: '$CZINSPECT'
jxr conversion tool in use: '$JXRCONVERT'

czi extraction tool's help text:
---
$($CZINSPECT $opts -h $*)
---

If -t is specified to be anything other than '.jxr', an image conversion tool is
used, limited to one process to allow the concurrency of many mass-czinspect
jobs. In this case, -v and -x are passed to it. Make sure the conversion tool
can convert to the image type you're specifying!

jxr conversion tool's help text:
---
$($JXRCONVERT -h $*)
---
EOF
}

error () {
    echo "$@" >&2
    exit 1
}

outdir=""
opts=""
xargargs=""
destroy=""
suffix=".jxr"
verbosity=0

while getopts "xv:P:EShd:f:gaes:t:" opt; do
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
    v)
        verbosity="$OPTARG"
        ;;
    P)
        xargargs="$xargargs -$opt $OPTARG"
        ;;
    x)
        destroy="1"
        ;;
    t)
        suffix="$OPTARG"
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

EXEC="out=\"\$(echo '{}' | sed 's,$indirsafe/*\(.*\).czi\$,$outdir/\1,g')\""

if [[ -n "$verbosity" ]] ; then
    EXEC="$EXEC && echo extracting '{}'"
fi

EXEC="$EXEC && mkdir -p \"\$out\""
EXEC="$EXEC && '$CZINSPECT' $opts -d\"\$out\" '{}'"

if [[ -n "$destroy" ]] ; then
    EXEC="$EXEC && rm -f '{}'"
fi

if [[ "$suffix" != ".jxr" ]] ; then
    jxrcflags="-v \"$verbosity\" -t \"$suffix\""

    if [[ -n "$destroy" ]] ; then
        jxrcflags="$jxrcflags -x"
    fi

    if [[ -n "$verbose" ]] ; then
        EXEC="$EXEC && echo converting tiles for '{}'"
    fi

    EXEC="$EXEC && $JXRCONVERT $jxrcflags \"\$out\""
fi

find $INDIR -maxdepth 1 -name '*.czi' | xargs $xargargs -I'{}' sh -c "$EXEC"
