#ifndef INT64_C
#define UINT64_C(c) (c ## LL) 
#define INT64_C(c) (c ## LL) 
#endif
/* Ryan Shirey, Joey Weidman
*/

//Headers
extern "C"
{
#include "libavformat/avformat.h" 
#include "libswscale/swscale.h" 
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
}
#include <iostream>

void writeFrame(AVPacket *Packet,int linesize, int width, int height, int i){
  FILE *pFile;
  char filename[32];
  //open file
  
  
  sprintf(filename, "frame%d.spff",i);
  pFile = fopen(filename, "wb");
  //write packet
  for(int y = 0; y<=height; y++){
    fwrite(Packet->data+y*linesize,1,width*3,pFile);
  }
  fclose(pFile);
}
int main(int argc, char *argv[])
{
  if (argc !=2){
    std::cout<<"Incorrect amount of arguments given"<<std::endl;
    return 0; 
  }
  av_register_all();
  avcodec_register_all();
  //Get context
  AVFormatContext *pFormatCtx = NULL;
  AVPacket packet;
  int ret;
  AVCodecContext *CCtx = NULL;
  AVCodec *Codec = NULL;
  AVFrame *Frame = NULL;
  AVFrame *FrameColor = NULL;
  uint8_t *buffer = NULL;
  int numBytes;
  int i = 0;
  struct SwsContext *sws_ctx = NULL;
  //Opens the file and stores format in pointer
  avformat_open_input(&pFormatCtx, argv[1], NULL, 0);
 
  //Looks at the format's headerfile and organizes
  avformat_find_stream_info(pFormatCtx, NULL);
  //prints info to terminal
  av_dump_format(pFormatCtx, 0, argv[1], 0);
  //Finds the videostream for the data
  int videoStream =-1;
  for(int i = 0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
      videoStream = i;
      break;
    }
  if(videoStream==-1)
    return -1;
  //Get codec context and decoder
  Codec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
  CCtx = avcodec_alloc_context3(Codec);
  avcodec_parameters_to_context(CCtx, pFormatCtx -> streams[videoStream]->codecpar);
  //Allocate memory
  avcodec_open2(CCtx, Codec, NULL); 
  Frame = av_frame_alloc();
  FrameColor = av_frame_alloc();
 
  //Allocate memory for buffer
  numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24,CCtx->width, CCtx->height, 1);
  buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
  //create Frame space
  av_image_fill_arrays(FrameColor->data,FrameColor->linesize, buffer, AV_PIX_FMT_RGB24,CCtx->width, CCtx->height,1);
  
  //mpenkov-ffmpeg tutorial
  //github.com/mpenkov/ffmpeg-tutorial/blob/master/tutorial01.c 
  sws_ctx = sws_getContext(CCtx->width,
			   CCtx->height,
			   CCtx->pix_fmt,
			   CCtx->width,
			   CCtx->height,
			   AV_PIX_FMT_RGB24,
			   SWS_BILINEAR,
			   NULL,
			   NULL,
			   NULL
			   );
  int frameDone;
  //Create SPFF context
  AVCodecContext *targetCtx = NULL;
  AVFormatContext *spffFormat = NULL;
  AVCodec *targetCodec = NULL;
  AVPacket *targetPacket = NULL;
  targetCodec = avcodec_find_encoder_by_name("spff");
  targetCtx = avcodec_alloc_context3(targetCodec);
  targetCtx->width = CCtx->width;
  targetCtx->height = CCtx->height;
  targetCtx->time_base.den = 30;
  targetCtx->time_base.num = 1;
  targetCtx->pix_fmt = AV_PIX_FMT_RGB24;
  targetPacket = av_packet_alloc();
  av_init_packet(targetPacket);
 
  //open context
  avcodec_open2(targetCtx,targetCodec,NULL);
 
  //Get frame
  double radius = CCtx->width/20;
  av_read_frame(pFormatCtx, &packet);
  double x0 = CCtx->width/2;
  double y0 = CCtx->height/2;
  double Vx = CCtx->width;
  double g = 10000;
  double Vy = 0;
  double deltaT = 0.0166666;
  double pointRadCalc;
  double xH;
  double yH;
  double highlightRatio;
  double highlightRadius = radius/2;
  for(int f = 1; f<=300; f++){
    //move center point
    if(y0+radius>=CCtx->height){
      Vy = -Vy*.98;
      
    }
    if(x0+radius>=CCtx->width||x0-radius<=0){
      Vx = -Vx*.8;
      if(x0+radius>CCtx->width)
	x0 = CCtx->width-radius;
      else if(x0-radius<CCtx->width)
	x0 = radius;
    }
    x0 = x0 + Vx*deltaT;
    y0 = y0 + Vy*deltaT + .5*g*deltaT*deltaT;
    Vy = Vy +g*deltaT;
    xH = x0 - radius*.4;
    yH = y0 - radius*.4;
    avcodec_send_packet(CCtx,&packet);
    avcodec_receive_frame(CCtx,Frame);
    sws_scale(sws_ctx, (uint8_t const * const *)Frame->data,
	      Frame->linesize, 0, CCtx->height,
	      FrameColor->data, FrameColor->linesize);
    
    if(packet.stream_index==videoStream){
      
      //modify data
      for(int h = 0; h<CCtx->height; h++){
	for(int w = 0; w<CCtx->width; w++){
	  pointRadCalc = (w-x0)*(w-x0)+
	    (h-y0)*(h-y0);
	  highlightRatio =255*highlightRadius/(sqrt((w-xH)*(w-xH)+
						 (h-yH)*(h-yH)));
	  
	  uint8_t* rgb = FrameColor->data[0] + (h*CCtx->width + w)*3 ;
	  if(pointRadCalc<radius*radius)
	    {
	      rgb [0]= 255;
	      if(highlightRatio>255){
		rgb [1]= 255;
		rgb [2]= 255;
	      }
	      else{
		rgb [1]= highlightRatio;
		rgb [2] = highlightRatio;
	      }
	    }
	}
      }
    }
    
    //send frame
    FrameColor->width = CCtx->width;
    FrameColor->height = CCtx->height;
    FrameColor->format = 0;
    avcodec_send_frame(targetCtx, FrameColor);
    //allocate and initiate packet
    //receive packet
    avcodec_receive_packet(targetCtx, targetPacket);
    writeFrame(targetPacket, FrameColor->linesize[0],targetCtx->width,targetCtx->height,f);
	
  }
}

