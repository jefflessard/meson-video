#!/bin/bash

#FFMPEG_OPTS="-hide_banner -loglevel warning -benchmark -stats"
FFMPEG_OPTS="-hide_banner -loglevel info -benchmark -stats"
#FFMPEG_OPTS="-hide_banner -loglevel debug -benchmark -stats"


h264_decode() {
	 local src_file="${1:-sample_h264.mp4}"
#		-pix_fmt nv12 \
#		-pix_fmt yuv420p \
#		-f rawvideo \
#		-y /dev/null
#		-vf select="gte(n\, 2)" \
	ffmpeg $FFMPEG_OPTS \
		-c:v h264_v4l2m2m \
		-i "${src_file}" \
		-frames:v 120 \
		-preset ultrafast \
		-map 0:v:0 \
		-c:v libx264 \
		-y output.ts
#	ffmpeg $FFMPEG_OPTS -c:v h264_v4l2m2m -i sample_h264.mp4 -map 0:v:0 -f null -
}

# Supported ffmpeg options for v4l2_m2m_enc
#   -pix_fmt : src_fmt
#   -qmin    : min_qp
#   -qmax    : max_qp
#   -g       : gop_size
#   -b:v     : bitrate
#   -force_key_frames ??

h264_encode() {
#	ffmpeg $FFMPEG_OPTS -i sample_h264.mp4 -c:v h264_v4l2m2m -f mpegts /dev/null -y

		#-frames:v 96 \
		#-pix_fmt nv12 \
	ffmpeg $FFMPEG_OPTS \
		-i sample_h264.mp4 \
		-pix_fmt yuv420p \
		-map 0:v:0 \
		-qmin 22 \
		-qmax 28 \
		-b:v 3000000 \
		-force_key_frames source \
		-g 300 \
		-c:v h264_v4l2m2m \
		-y output.ts

#	ffmpeg $FFMPEG_OPTS -c:v rawvideo -pix_fmt nv12 -i sample.nv12 -c:v h264_v4l2m2m -f ts  /dev/null -y
}

h264_encode_ref() {
	ffmpeg $FFMPEG_OPTS -i sample_h264.mp4 -f rawvideo -pix_fmt nv12 /dev/null -y
}

h264_transcode() {
	ffmpeg $FFMPEG_OPTS -c:v h264_v4l2m2m -i sample_h264.mp4 -c:v h264_v4l2m2m -f mpegts /dev/null -y
}

frames() {
	ffprobe -v error -select_streams v:0 -show_entries stream=codec_type,field_order -of default=noprint_wrappers=1 output.ts
	ffprobe -hide_banner -v warning -select_streams v:0 -show_frames -show_entries frame=display_picture_number,coded_picture_number,pts,pict_type,key_frame -read_intervals "%+1" -print_format csv=p=0 output.ts
}

headers() {
	ffmpeg -hide_banner -loglevel info -i output.ts -c copy -bsf:v trace_headers -f null - 2>&1
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
	time h264_decode $@
	#time h264_encode $@
	#time h264_transcode $@
else
	time $@
fi

