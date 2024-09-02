#!/bin/bash

DEVICE="/dev/video0"

# List output formats and store them in a variable
output_formats=$(v4l2-ctl --list-formats-out-ext --device $DEVICE)

# Print the output formats
echo "Output formats:"
echo "$output_formats"
echo ""

# Read the output formats from the variable
while read -r line; do

	if [[ $line =~ \[[0-9]+\]: ]]; then
		format_index=${line%%]*}
		format_index=${format_index#[}
		format=${line#*\'}
		format=${format%%\'*}
	elif [[ $line == *"Size:"* ]]; then
		size=${line#*Continuous }
		min_width=${size%%x*}
		min_height=${size#*x}
		min_height=${min_height%% *}

		echo "$format Capture Formats (${min_width}x${min_height}):"
		v4l2-ctl --set-fmt-video-out=width=$min_width,height=$min_height,pixelformat=$format --device $DEVICE --list-formats-ext
		echo ""
	fi
done <<< "$output_formats"

