#!/bin/bash

DEVICE="/dev/video0"

show_formats() {
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
}

stream_file() {
	in_file=$1
	in_format=$2
	width=$3
	height=$4
	out_file=$5
	out_format=$6

	v4l2-ctl --set-fmt-video=width=$width,height=$height,pixelformat=$in_format --set-fmt-video-out=pixelformat=$out_format --device $DEVICE --stream-mmap --stream-from=$in_file --stream-to=$out_file --stream-count=100
}

#show_formats

stream_file sample.hevc HEVC 1920 800 output.h264 H264
