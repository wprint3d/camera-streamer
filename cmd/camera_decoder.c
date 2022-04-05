#include "camera.h"

#include "hw/buffer.h"
#include "hw/buffer_list.h"
#include "hw/device.h"
#include "hw/links.h"
#include "hw/v4l2.h"
#include "hw/buffer_list.h"
#include "http/http.h"

int camera_configure_decoder(camera_t *camera)
{
  if (device_open_buffer_list(camera->camera, true, camera->width, camera->height, camera->format, 0, camera->nbufs, true) < 0) {
    return -1;
  }

  buffer_list_t *src = camera->camera->capture_list;

  camera->decoder.decoder = device_open("DECODER", "/dev/video10");
  camera->codec_h264 = device_open("H264", "/dev/video11");

  if (device_open_buffer_list_output(camera->decoder.decoder, src) < 0 ||
    device_open_buffer_list_capture(camera->decoder.decoder, src, 1.0, V4L2_PIX_FMT_YUV420, true) < 0) {
    return -1;
  }

  src = camera->decoder.decoder->capture_list;

  if (camera->format != V4L2_PIX_FMT_MJPEG && camera->format != V4L2_PIX_FMT_JPEG) {
    camera->codec_jpeg = device_open("JPEG", "/dev/video31");

    if (device_open_buffer_list_output(camera->codec_jpeg, src) < 0 ||
      device_open_buffer_list_capture(camera->codec_jpeg, src, 1.0, V4L2_PIX_FMT_JPEG, true) < 0) {
      return -1;
    }
  }

  if (device_open_buffer_list_output(camera->codec_h264, src) < 0 ||
    device_open_buffer_list_capture(camera->codec_h264, src, 1.0, V4L2_PIX_FMT_H264, true) < 0) {
    return -1;
  }

  link_t *links = camera->links;

  if (camera->format == V4L2_PIX_FMT_MJPEG || camera->format == V4L2_PIX_FMT_JPEG) {
    *links++ = (link_t){ camera->camera, { camera->decoder.decoder }, { http_jpeg_capture, http_jpeg_needs_buffer } };
    *links++ = (link_t){ camera->decoder.decoder, { camera->codec_h264 } };
    *links++ = (link_t){ camera->codec_h264, { }, { http_h264_capture, http_h264_needs_buffer } };
  } else {
    *links++ = (link_t){ camera->camera, { camera->decoder.decoder } };
    *links++ = (link_t){ camera->decoder.decoder, { camera->codec_jpeg, camera->codec_h264 } };
    *links++ = (link_t){ camera->codec_jpeg, { }, { http_jpeg_capture, http_jpeg_needs_buffer } };
    *links++ = (link_t){ camera->codec_h264, { }, { http_h264_capture, http_h264_needs_buffer } };
  }
  return 0;
}
