#!/bin/bash

DEVICE="/dev/video0"

show_formats() {
	# List input formats and store them in a variable
	input_formats=$(v4l2-ctl --list-formats-out-ext --device $DEVICE)

	# Print the input formats
	echo "Source Formats:"
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

			echo "$format Destination Formats (${min_width}x${min_height}):"
			v4l2-ctl --set-fmt-video-out=width=$min_width,height=$min_height,pixelformat=$format --device $DEVICE --list-formats-ext
			echo ""
		fi
	done <<< "$input_formats"
}

stream_file() {
	src_file=$1
	src_format=$2
	width=$3
	height=$4
	dst_file=$5
	dst_format=$6

	v4l2-ctl \
		--device $DEVICE \
		--set-fmt-video-out=width=$width,height=$height,pixelformat=$src_format \
		--set-fmt-video=pixelformat=$dst_format \
		--stream-out-mmap \
		--stream-mmap \
		--stream-from=$src_file \
		--stream-to=$dst_file \
		--stream-count=100
}

show_formats

#stream_file sample.hevc HEVC 1920 800 output.h264 H264
#stream_file sample.nv12 NV12 1920 800 output.h264 H264
#stream_file sample.hevc HEVC 1920 800 output.nv12 NV12
