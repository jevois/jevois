#!/bin/bash

# Usage: round-image-corners.sh icon.png
#
# Rounds corners of an image; useful for module icons

# you need to have imagemagick installed
# code from: http://www.imagemagick.org/Usage/thumbnails/#rounded

convert $1 \
	\( +clone  -alpha extract \
        -draw 'fill black polygon 0,0 0,15 15,0 fill white circle 15,15 15,0' \
        \( +clone -flip \) -compose Multiply -composite \
        \( +clone -flop \) -compose Multiply -composite \
	\) -alpha off -compose CopyOpacity -composite $1

