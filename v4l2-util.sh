#!/bin/bash

DEVICE="/dev/video0"

# List input formats and store them in a variable
input_formats=$(v4l2-ctl --list-formats-ext --device $DEVICE)

# Print the input formats
echo "Capture formats:"
echo "$input_formats"
echo ""

# Read the input formats from the variable
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

		echo "$format Output Formats (${min_width}x${min_height}):"
		v4l2-ctl --set-fmt-video=width=$min_width,height=$min_height,pixelformat=$format --device $DEVICE --list-formats-out-ext
		echo ""
	fi
done <<< "$input_formats"

