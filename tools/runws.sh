#!/bin/sh

Whitespace/whitespace.out $1 -t > $1.c
cc $1.c -o $1.c.exe
./$1.c.exe
