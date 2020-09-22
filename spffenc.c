
#include "libavutil/imgutils.h"
#include "libavutil/avassert.h"
#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"



static av_cold int spff_encode_init(AVCodecContext *avctx){
  //Get the pixel format
  //The only format supported by our spff is RGB8
  switch(avctx->pix_fmt){
  case AV_PIX_FMT_RGB8:
    //Set the bits per pixel to 8
    avctx->bits_per_coded_sample = 8;
    break;
  default:
    //Can't support anything but RGB8					
    av_log(avctx, AV_LOG_INFO, "unsupported pixel format \n");
    return AVERROR(EINVAL);
  }
  
    return 0;
}

static int spff_encode_frame(AVCodecContext *avctx, AVPacket *pkt,
                            const AVFrame *pict, int *got_packet)
{
  //Pointer to the image frame
  const AVFrame * const p = pict;
  int imgTotalBytes , bytesPerRow, totalBytes, headerSize, ret;

  //The color table that holds the color values
  //Initializes an array of 256 because it's the maximum amount of colors 8 bits can make
  const uint32_t *palette = NULL;
  uint32_t palette256[256];

  //The amount of bits per pixel
  int bitCount= 8;

  //Padding makes the row a multiple of 4
  int padding = 0;

  //Initialize the number of colors to 256
  int numColors = 256;

  //ptr is the pointer to the location where we are taking data from
  //buf is the pointer to the location where we are placing data
  uint8_t *ptr, *buf;

  avpriv_set_systematic_pal2(palette256, avctx->pix_fmt);

  //sets a pointer to the palette with 256 slots
  palette = palette256;

  //Calculates the row size
  bytesPerRow = (((int64_t)avctx->width * bitCount + 31) / 32) * 4;

  //Calculates the padding for the row
  padding = (4 - bytesPerRow) & 3;

  //The total amount of bytes in the image
  imgTotalBytes = avctx->height * (bytesPerRow + padding);
  
  //Includes "SPFF"(4), totalBytes(4), headerSize(4), width(4), height (4)
#define SIZE_HEADER 20 
  
  headerSize = SIZE_HEADER + numColors;

  //Total file size
  totalBytes = imgTotalBytes + headerSize;

  if ((ret = ff_alloc_packet2(avctx, pkt, totalBytes, 0)) < 0)
    return ret;

  //Sets the buf pointer to the start of the file data
  buf = pkt->data;

  //SPFF shows that it's a .spff file
  bytestream_put_byte(&buf, 'S');                   
  bytestream_put_byte(&buf, 'P');                
  bytestream_put_byte(&buf, 'F');
  bytestream_put_byte(&buf, 'F');

  //Encodes header info
  bytestream_put_le32(&buf, totalBytes);
  bytestream_put_le32(&buf, headerSize);
  bytestream_put_le32(&buf, avctx->width);
  bytestream_put_le32(&buf, avctx->height);

  //Fill the color table with all the possible 8 bit colors
  for (int i = 0; i < numColors; i++){
    bytestream_put_le32(&buf, palette[i] & 0xFFFFFF);
  }

  //Set the ptr pointer to start of the image data
  ptr = p->data[0];

  //Points the buffer to the start of the data
  buf = pkt->data + headerSize;

  //Fill the buffer with data starting from the top and moving down
  for(int i = 0; i < avctx->height; i++) {
    //Copies the data from ptr and puts it into buf
    memcpy(buf, ptr, bytesPerRow);
    //Make buf point to the end of the data in the row
    buf += bytesPerRow;
    //Fills the rest of the row until it's a multiple of 4
    memset(buf, 0, padding);
    //Makes the buffer point to the end of the "row"    
    buf += padding;
    //Make the pointer point to the next row in data
    ptr += p->linesize[0];
  }
  
  pkt->flags |= AV_PKT_FLAG_KEY;
  *got_packet = 1;
  return 0;
}

AVCodec ff_spff_encoder = {
  .name           = "spff",
  .long_name      = NULL_IF_CONFIG_SMALL("BMP (Windows and OS/2 bitmap)"),
  .type           = AVMEDIA_TYPE_VIDEO,
  .id             = AV_CODEC_ID_SPFF,
  .init           = spff_encode_init,
  .encode2        = spff_encode_frame,
  .pix_fmts       = (const enum AVPixelFormat[]){
    AV_PIX_FMT_RGB8,
    AV_PIX_FMT_NONE
  },
};
