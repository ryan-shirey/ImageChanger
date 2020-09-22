#include <inttypes.h>

#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"
#include "msrledec.h"

static int spff_decode_frame(AVCodecContext *avctx,
                            void *data, int *got_frame,
                            AVPacket *avpkt)
{
  //the buffer pointer that is used to determine where in the file data the reader is
  const uint8_t *buf = avpkt->data;
  int buf_size = avpkt->size;
  AVFrame *p = data;
  //fileSize is the total amount of bytes in the spff file
  //dataStart is when the pixel information starts, which is after the header and color pallete
  unsigned int fileSize, dataStart;
  int width, height;
  //The amount of bits in each pixel
  int const bitsPerPix = 8;
  //n is the linesize with a pad to be a multiple of for
  int n, linesize, ret;
  //pointer used in copy operations
  uint8_t *ptr;
  //temp value created to remember the start of the buffer for use after reading head file
  const uint8_t *buf0 = buf;
  //Checks that the file is spff type
  if (bytestream_get_byte(&buf) != 'S' ||
      bytestream_get_byte(&buf) != 'P' ||
      bytestream_get_byte(&buf) != 'F' ||
      bytestream_get_byte(&buf) != 'F') {
    av_log(avctx, AV_LOG_ERROR, "bad magic number\n");
    return AVERROR_INVALIDDATA;
  }
  //reads the total size of the file
  fileSize = bytestream_get_le32(&buf);
  //reads where the data starts in the file
  dataStart  = bytestream_get_le32(&buf); 
  //reads the width and height of the image
  width  = bytestream_get_le32(&buf);
  height = bytestream_get_le32(&buf);
  //sets the width, height, and pixel format for the context
  avctx->width  = width;
  avctx->height = height;    
  avctx->pix_fmt = AV_PIX_FMT_RGB8;
  
  if ((ret = ff_get_buffer(avctx, p, 0)) < 0)
    return ret;
  //sets the buffer to the start of the file + the header, which is the start of the pixel data
  buf   = buf0 + dataStart;
  
  //gets a linesize then increments it to the next multiple of four
  n = ((avctx->width * bitsPerPix + 31) / 8) & ~3;
  //checks if height is negative and if it is flip the linesize
  if (height > 0) {
    ptr      = p->data[0];
    linesize = p->linesize[0];
  } else {
    ptr      = p->data[0];
    linesize = -p->linesize[0];
  }

  //goes through each row of the pixel data from top down
  for (int i = 0; i < avctx->height; i++) {
    //copys the data to the pointer from the buffer
    memcpy(ptr, buf, n);
    //increment buffer and pointer
    buf += n;
    ptr += linesize;
  }
  
  //sets got_frame to true
  *got_frame = 1;
  
  return buf_size;
}
//Struct for method access
AVCodec ff_spff_decoder = {
  .name           = "spff",
  .long_name      = NULL_IF_CONFIG_SMALL("BMP (Windows and OS/2 bitmap)"),
  .type           = AVMEDIA_TYPE_VIDEO,
  .id             = AV_CODEC_ID_SPFF,
  .decode         = spff_decode_frame,
  .capabilities   = AV_CODEC_CAP_DR1,
};
