#!/bin/sh

FILES=*.svg
for f in $FILES
do
    FULL_FILENAME=$f
    FILENAME=${FULL_FILENAME##*/}
    echo ${FILENAME%%.*}
    convert -background transparent -density 256 -resize 256x $FULL_FILENAME ${FILENAME%%.*}"_256.png"
    convert -background transparent -density 128 -resize 128x $FULL_FILENAME ${FILENAME%%.*}"_128.png"
    convert -background transparent -density 96 -resize 96x $FULL_FILENAME ${FILENAME%%.*}"_96.png"
    convert -background transparent -density 64 -resize 64x $FULL_FILENAME ${FILENAME%%.*}"_64.png"
    convert -background transparent -density 48 -resize 48x $FULL_FILENAME ${FILENAME%%.*}"_48.png"
    convert -background transparent -density 40 -resize 40x $FULL_FILENAME ${FILENAME%%.*}"_40.png"
    convert -background transparent -density 32 -resize 32x $FULL_FILENAME ${FILENAME%%.*}"_32.png"
    convert -background transparent -density 24 -resize 24x $FULL_FILENAME ${FILENAME%%.*}"_24.png"
    convert -background transparent -density 20 -resize 20x $FULL_FILENAME ${FILENAME%%.*}"_20.png"
    convert -background transparent -density 16 -resize 16x $FULL_FILENAME ${FILENAME%%.*}"_16.png"
    convert -background transparent ${FILENAME%%.*}"_256.png" ${FILENAME%%.*}"_128.png" ${FILENAME%%.*}"_96.png" ${FILENAME%%.*}"_64.png" ${FILENAME%%.*}"_48.png" ${FILENAME%%.*}"_40.png" ${FILENAME%%.*}"_32.png" ${FILENAME%%.*}"_24.png" ${FILENAME%%.*}"_20.png" ${FILENAME%%.*}"_16.png" ${FILENAME%%.*}.ico
done