#!/bin/bash

PROJECTS=(
    "./examples/blank-screen/"
    "./examples/gradiants/"
    "./examples/load-jpg/"
    "./examples/mandelbrot/"
    "./examples/primitive-test/"
    "./examples/RoundedRectangle/"
    "./examples/font-test/"
    "./examples/truetype-font/"
    "./examples/X11/"
)

for t in ${PROJECTS[@]}; do
    cd "$t"
    appbuild -c x11 -r
    appbuild -c release -r
    cd -
done

