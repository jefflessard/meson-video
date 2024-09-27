#!/bin/bash

FFMPEG_OPTS="-hide_banner -loglevel warning -benchmark -stats"
#FFMPEG_OPTS="-hide_banner -loglevel debug -benchmark -stats"


h264_decode() {
	ffmpeg $FFMPEG_OPTS -c:v h264_v4l2m2m -i sample_h264.mp4 -f rawvideo -pix_fmt nv12 /dev/null -y
#	ffmpeg $FFMPEG_OPTS -c:v h264_v4l2m2m -i sample_h264.mp4 -map 0:v:0 -f null -
}

h264_encode() {
#	ffmpeg $FFMPEG_OPTS -i sample_h264.mp4 -c:v h264_v4l2m2m -f mpegts /dev/null -y
	ffmpeg $FFMPEG_OPTS -i sample_h264.mp4 -frames:v 96 -map 0:v:0 -pix_fmt nv12 -g 12 -c:v h264_v4l2m2m output.ts -y
#	ffmpeg $FFMPEG_OPTS -c:v rawvideo -pix_fmt nv12 -i sample.nv12 -c:v h264_v4l2m2m -f ts  /dev/null -y
}

h264_encode_ref() {
	ffmpeg $FFMPEG_OPTS -i sample_h264.mp4 -f rawvideo -pix_fmt nv12 /dev/null -y
}

h264_transcode() {
	ffmpeg $FFMPEG_OPTS -c:v h264_v4l2m2m -i sample_h264.mp4 -c:v h264_v4l2m2m -f mpegts /dev/null -y
}

probe() {
	ffprobe -v warning -select_streams v:0 -show_frames -show_entries frame=display_picture_number,coded_picture_number,pts,pict_type,key_frame -read_intervals "%+2" -print_format csv=p=0 output.ts
}

exec_time() {
	func=$1

	start_ns=$(date +%s%N)
	$func
	end_ns=$(date +%s%N)

	dur_ns=$((($end_ns - $start_ns)/1000000))

	echo "$func executed in $dur_ns ms"
}

test_fn=$1
if [ -z "$test_fn" ]
then
	#time h264_decode
	time h264_encode
	#time h264_transcode
else
	time $test_fn
fi

