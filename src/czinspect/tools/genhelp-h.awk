#!/usr/bin/env awk -f

BEGIN {
    OFS="";
}

{
    print "extern char _binary_helptxt_", $1, "_start[];";
}

{
    print "extern char _binary_helptxt_", $1, "_end[];";
}
