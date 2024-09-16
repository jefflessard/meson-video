#!/bin/bash

#FFMPEG_OPTS="-hide_banner -loglevel warning -benchmark -stats"
FFMPEG_OPTS="-hide_banner -loglevel debug -benchmark -stats"


h264_decode() {
	ffmpeg $FFMPEG_OPTS -c:v h264_v4l2m2m -i sample_h264.mp4 -f rawvideo -pix_fmt nv12 /dev/null -y
#	ffmpeg $FFMPEG_OPTS -c:v h264_v4l2m2m -i sample_h264.mp4 -map 0:v:0 -f null -
}

h264_encode() {
	ffmpeg $FFMPEG_OPTS -i sample_h264.mp4 -c:v h264_v4l2m2m -f mpegts /dev/null -y
#	ffmpeg $FFMPEG_OPTS -c:v rawvideo -pix_fmt nv12 -i sample.nv12 -c:v h264_v4l2m2m -f ts  /dev/null -y
}

exec_time() {
	func=$1

	start_ns=$(date +%s%N)
	$func
	end_ns=$(date +%s%N)

	dur_ns=$((($end_ns - $start_ns)/1000000))

	echo "$func executed in $dur_ns ms"
}

#time h264_decode
time h264_encode

