#!/bin/bash

set -e

JXRDECAPP="JxrDecApp"
CONVERT="vips --vips-concurrency=1 copy"

JXRFMT="-i %s -o %s"

usage () {
    cat <<EOF
Usage: deczi.sh [options] <input_dir>

mass-jxr-convert's options:
    -d <dir>    Output directory (defaults to input directory)
    -x          Destructive: remove old images after making new ones
    -t <suffix> Specify output image type (default is '.tif')
    -P <procs>  Number of processes to run simultaneously;
                passed to xargs if used
    -v          Verbosity of output (default is 0 - silent)
    -h          Print this help message

jxr conversion tool in use: '$JXRDECAPP'
general image converter in use: '$CONVERT'

jxr conversion tool's help text:
===
$($JXRDECAPP -h $*)
===

If -t is specified to be anything other than '.tif', a general image conversion
tool is used, limited to one process. MAke sure this converter can convert to
the image type you're specifying!

general image converter's help text:
===
$($CONVERT --help)
===
EOF
}

error () {
    echo "$@" >& 2
    exit 1
}

OUTDIR=""
DESTROY=""
SUFFIX=".tif"
XARGSARGS=""
VERBOSITY=0

while getopts "d:hxt:P:v:" opt; do
    case $opt in
        d)
            OUTDIR="$OPTARG"
            ;;
        h)
            usage
            exit 0
            ;;
        x)
            DESTROY="1"
            ;;
        t)
            SUFFIX="$OPTARG"
            ;;
        P)
            XARGSARGS="$XARGSARGS -$opt$OPTARG"
            ;;
        v)
            VERBOSITY="$OPTARG"
            ;;
    esac
done

shift $((OPTIND - 1))

if (( $# == 0 )); then
    error "Missing input folder"
fi

INDIR=$1
INDIRSAFE="$(echo $INDIR | sed 's/[[\.*^$,]/\\&/g')"

if [[ -z "$OUTDIR" ]]; then
    OUTDIR="$INDIR"
fi

EXEC="mid=\"\$(echo '{}' | sed 's,$INDIRSAFE/*\(.*\)\.jxr\$,$OUTDIR/\1,g')\""

if [[ -n "$VERBOSITY" ]]; then
    EXEC="$EXEC && echo \"\$mid\"\"{.jxr->.tif}\""
fi

EXEC="$EXEC && $JXRDECAPP \$(printf -- '$JXRFMT' '{}' \"\$mid\"\".tif\")"

if [[ -n "$DESTROY" ]] ; then
    EXEC="$EXEC && rm -f '{}'"
fi

if [ "$SUFFIX" != ".tif" ] ; then
    if [[ -n "$VERBOSITY" ]] ; then
        EXEC="$EXEC && echo \"\$mid\"\"{.tif->$SUFFIX}\""
    fi

    EXEC="$EXEC && $CONVERT \"\$mid\".tif \"\$mid\"\"$SUFFIX\""
    
    if [[ -n "$DESTROY" ]] ; then
        EXEC="$EXEC && rm -f \"\$mid\".tif"
    fi
fi

find $INDIR -maxdepth 1 -name '*.jxr' | xargs $XARGSARGS -I'{}' sh -c "$EXEC"
