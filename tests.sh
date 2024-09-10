#!/bin/bash

LOGLEVEL=debug


h264_decode() {
	ffmpeg -loglevel $LOGLEVEL -c:v h264_v4l2m2m -i sample_h264.mp4 -f rawvideo -pix_fmt nv12 /dev/null -y
}

exec_time() {
	func=$1

	start_ns=$(date +%s%N)
	$func
	end_ns=$(date +%s%N)

	dur_ns=$((($end_ns - $start_ns)/1000000))

	echo "$func executed in $dur_ns ms"
}

exec_time h264_decode

