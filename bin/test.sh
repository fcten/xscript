#!/bin/sh

function mk_test() {
    for file in $1/*
    do
        if test -f $file
        then
            ./xscript $file > /dev/null
            if [ $? -ne 0 ] ;then
                echo "ERROR" $file
            else
                echo "OK" $file
            fi
        elif test -d $file
        then
            mk_test $file 
        fi
    done
}

if [ $# -eq 1 ] ; then
    mk_test $1
fi