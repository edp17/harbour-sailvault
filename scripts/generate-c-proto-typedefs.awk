# Reproduce Wallet Core 4.0.27 protobuf-plugin/c_typedef.cc without libprotoc.
# Emits typedefs for top-level messages only.
function sanitize(s,    out,i,c,n,nextc) {
    out=""
    n=length(s)
    in_string=0
    escape=0
    for (i=1; i<=n; i++) {
        c=substr(s,i,1)
        nextc=(i<n ? substr(s,i+1,1) : "")
        if (in_block) {
            if (c=="*" && nextc=="/") { in_block=0; i++ }
            continue
        }
        if (in_string) {
            out=out " "
            if (escape) { escape=0; continue }
            if (c=="\\") { escape=1; continue }
            if (c=="\"") in_string=0
            continue
        }
        if (c=="/" && nextc=="*") { in_block=1; i++; continue }
        if (c=="/" && nextc=="/") break
        if (c=="\"") { in_string=1; out=out " "; continue }
        out=out c
    }
    return out
}
function brace_delta(s,    i,c,d) {
    d=0
    for (i=1; i<=length(s); i++) {
        c=substr(s,i,1)
        if (c=="{") d++
        else if (c=="}") d--
    }
    return d
}
BEGIN { depth=0; package=""; count=0; in_block=0; in_string=0; escape=0 }
{
    clean=sanitize($0)
    if (package=="" && match(clean, /^[[:space:]]*package[[:space:]]+[A-Za-z0-9_.]+[[:space:]]*;/)) {
        tmp=substr(clean,RSTART,RLENGTH)
        sub(/^[[:space:]]*package[[:space:]]+/,"",tmp)
        sub(/[[:space:]]*;[[:space:]]*$/,"",tmp)
        package=tmp
    }
    if (depth==0 && match(clean, /^[[:space:]]*message[[:space:]]+[A-Za-z_][A-Za-z0-9_]*/)) {
        tmp=substr(clean,RSTART,RLENGTH)
        sub(/^[[:space:]]*message[[:space:]]+/,"",tmp)
        names[++count]=tmp
    }
    depth += brace_delta(clean)
}
END {
    print "// SPDX-License-Identifier: Apache-2.0"
    print "//"
    print "// Copyright © 2017 Trust Wallet."
    print "//"
    print "// This is a GENERATED FILE, changes made here WILL BE LOST."
    print ""
    print "#pragma once"
    print ""
    print "#include \"TWData.h\""
    print ""
    n=split(package,parts,".")
    if (n < 2 || parts[1] != "TW") {
        print "ERROR: invalid proto package '" package "'" > "/dev/stderr"
        exit 2
    }
    prefix=package
    gsub(/\./,"_",prefix)
    for (i=1; i<=count; i++)
        print "typedef TWData *_Nonnull " prefix "_" names[i] ";"
}
