#!/bin/sh

for file in ../test/*
do
    if test -f $file
    then
        valgrind --log-fd=1 --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ./xscript $file | grep -q "ERROR SUMMARY: 0 errors"
        if [ $? -ne 0 ] ;then
            echo "ERROR" $file
        else
            echo "OK" $file
        fi
    fi
done
