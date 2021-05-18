#!/bin/bash

code="$PWD"
opts=-g
cd build > /dev/null
g++ $opts $code/code/nes.cpp -o nes
cd $code > /dev/null
