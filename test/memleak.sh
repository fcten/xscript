#!/bin/sh

for file in ./basic/*
do
    if test -f $file
    then
        echo $file
        valgrind --log-fd=1 --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ../xscript $file | grep -q "All heap blocks were freed -- no leaks are possible"
        if [ $? -ne 0 ] ;then
            echo "ERROR"
        else
            echo "OK"
        fi
    fi
done
