#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>


static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           tag,
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}


int remux(char *in_filename, char *out_filename)
{
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVBitStreamFilterContext *bsfc = NULL;
    AVPacket pkt;
    int ret, i;

    av_register_all();

    
    bsfc = av_bitstream_filter_init("h264_mp4toannexb");
    if (!bsfc) {
      fprintf(stderr, "Cannot open the h264_mp4toannexb BSF!");
    }
    
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        goto end;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }

    //av_dump_format(ifmt_ctx, 0, in_filename, 0);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    ofmt = ofmt_ctx->oformat;
    
    AVCodecContext *avctx;
    int video_stream_ind = -1;
    uint8_t *dummy_p;
    int dummy_int;
    
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *in_stream = ifmt_ctx->streams[i];
	if(in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO){
	  video_stream_ind = i;
	  avctx = in_stream->codec;
	  av_bitstream_filter_filter(bsfc, avctx, NULL, &dummy_p,
				     &dummy_int, NULL, 0, 0);
	}
	
	AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
        if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
            goto end;
        }
        out_stream->codec->codec_tag = 0;
	out_stream->time_base = av_add_q(in_stream->codec->time_base, (AVRational){0, 1});
	// ignore (not build for ffmpeg from CentOS 7)
	//if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
	//  out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    //av_dump_format(ofmt_ctx, 0, out_filename, 1);

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            goto end;
        }
    }

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }

    while (1) {
        AVStream *in_stream, *out_stream;
	
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;

	uint8_t *in_data  = pkt.data;
	uint8_t *prev_data = pkt.data;
	int len            = pkt.size;
	
	
	in_stream  = ifmt_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];

	// apply filter to video data 
	if(pkt.stream_index == video_stream_ind){
	  ret = av_bitstream_filter_filter(bsfc, avctx, NULL,
					   &in_data, &len,
					   pkt.data, len, 0);
	  
	  pkt.data = in_data;
	  pkt.size = len;
	}
	//else
	//  log_packet(ifmt_ctx, &pkt, "in AUDIO");

        /* copy packet */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        
        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }

	// clear old prev data
	if(pkt.data != prev_data){
	  //if(prev_data)
	  //  av_free(prev_data);
	}
	
	
        av_packet_unref(&pkt);
    }

    av_write_trailer(ofmt_ctx);
end:

    avformat_close_input(&ifmt_ctx);

    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);

    avformat_free_context(ofmt_ctx);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}

int remux_cmd(char* cmd, char *in_filename, char *out_filename){
  //
  //system(cmd, in_filename, out_filename);
  extern char **environ;
  int status;
  
  pid_t pid;
  char* args[3];
  args[0] = cmd;
  args[1] = in_filename;
  args[2] = out_filename;
  args[3] = NULL;
  //execv(cmd, args);
  //execvp(args[0], args);
  //posix_spawn(0, args[0], args);
  status = posix_spawn(&pid, cmd, NULL, NULL, args, environ);
  waitpid(pid, &status, 0);
   
  return 0;
}

