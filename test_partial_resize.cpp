#include <iostream>
#include <vector>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <opencv2/opencv.hpp>

#define IOU 0.5


int read_jpeg_file(char *file_name, unsigned char ** buf, unsigned long * size)
{

    int rc, i, j;
    unsigned long jpg_size;
	unsigned char *jpg_buffer;
    struct stat file_info;
    rc = stat(file_name, &file_info);
	if (rc) {
		printf("FAILED to stat source %s jpg\n", file_name);
		exit(-1);
	}
	jpg_size = file_info.st_size;
	jpg_buffer = (unsigned char*) malloc(jpg_size + 100);

	int fd = open(file_name, O_RDONLY);
	i = 0;
	while (i < jpg_size) {
		rc = read(fd, jpg_buffer + i, jpg_size - i);
		printf( "Input: Read %d/%lu bytes \n", rc, jpg_size-i);
		i += rc;
	}
	close(fd);

    *buf = jpg_buffer;
    *size = jpg_size;
    return 0;
}

void write_ppm_file(char * file_name, unsigned char * buff, 
                     int width, int height)
{
    int rc; 
    int fd = fd = open(file_name, O_CREAT | O_WRONLY, 0666);
	char buf[1024];
    unsigned long size = width * height * 3;
	rc = sprintf(buf, "P6 %d %d 255\n", width, height);
	write(fd, buf, rc);
	write(fd, buff, size); 

	close(fd);
}





// data     jpeg压缩数据
// length   jpeg压缩数据长度   
// 
// out_w    部分解码之后的宽
// out_h    部分解码之后的高

int JpegPartialDecode(const unsigned char* data, size_t length, int crop_generator,
                      unsigned char* workspace, size_t workspace_size, unsigned char* dst,
                      int *out_w, int *out_h, int target_width, int target_height) {

  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  int row_stride, width, height, pixel_size, crop_x, crop_y, crop_w, crop_h, rc;
  unsigned int tmp;
  unsigned char* crop_buf;

  cinfo.err = jpeg_std_error(&jerr);
  printf("0 cinfo -> %d \n", cinfo.err->msg_code);
  jpeg_create_decompress(&cinfo);
  printf("1 cinfo -> %d \n", cinfo.err->msg_code);
  jpeg_mem_src(&cinfo, data, length);
  printf("2 cinfo -> %d \n", cinfo.err->msg_code);
  jpeg_destroy_decompress(&cinfo);

  rc = jpeg_read_header(&cinfo, TRUE);
  // if (rc != 1) { return -1; }
  printf("3 cinfo -> %d \n", cinfo.err->msg_code);

  jpeg_start_decompress(&cinfo);
  width = cinfo.output_width;
  height = cinfo.output_height;
  pixel_size = cinfo.output_components;

  if (width * height * pixel_size > workspace_size) {
    std::vector<unsigned char> tmp_buf(width * height * pixel_size);
    crop_buf = tmp_buf.data();
  } else {
    crop_buf = workspace;
  }

  if (crop_generator) {
    crop_w = sqrt(IOU) * width;
    crop_h = sqrt(IOU) * height;
    crop_x = (width - crop_w) / 2;
    crop_y = (height - crop_h) / 2;
  } else {
    crop_x = 0;
    crop_y = 0;
    crop_w = width;
    crop_h = height;
  }

  unsigned int u_crop_x = crop_x, u_crop_y = crop_y, u_crop_w = crop_w, u_crop_h = crop_h;

  u_crop_x = 200;
  
  jpeg_crop_scanline(&cinfo, &u_crop_x, &u_crop_w);
  row_stride = u_crop_w * pixel_size;
  if ((tmp = jpeg_skip_scanlines(&cinfo, u_crop_y)) != u_crop_y) { return -2; }

  while (cinfo.output_scanline < u_crop_y + u_crop_h) {
    unsigned char* buffer_array[1];
    buffer_array[0] = crop_buf + (cinfo.output_scanline - u_crop_y) * row_stride;
    jpeg_read_scanlines(&cinfo, buffer_array, 1);
  }

  jpeg_skip_scanlines(&cinfo, cinfo.output_height - u_crop_y - u_crop_h);
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  cv::Mat image(u_crop_h, u_crop_w, CV_8UC3, crop_buf, cv::Mat::AUTO_STEP);

  cv::Rect roi;
  cv::Mat cropped;

  if (u_crop_w != crop_w) {
    roi.x = u_crop_w - crop_w;
    roi.y = 0;
    roi.width = crop_w;
    roi.height = crop_h;
    image(roi).copyTo(cropped);
  } else {
    cropped = image;
  }

  cv::Mat dst_mat(target_height, target_width, CV_8UC3, dst, cv::Mat::AUTO_STEP);

  cv::resize(cropped, dst_mat, cv::Size(target_width, target_height), 0, 0, cv::INTER_LINEAR);

  *out_w = u_crop_w;
  *out_h = u_crop_h;

  return 0;
}


#define OUTBUFF_SIZE (1024*1024*1024)
#define FOR_LOOP 320000
#define THREAD_NUM 64
int main(void)
{
    int rc, i, j;
    unsigned long jpg_buffer_size;
	unsigned char *jpg_buffer;
    int width, height;



    unsigned char *out_buffer = (unsigned char*) malloc(OUTBUFF_SIZE);
    memset(out_buffer, 0, OUTBUFF_SIZE);

    unsigned char *workspace = (unsigned char*) malloc(OUTBUFF_SIZE);

    read_jpeg_file("/home/guoluqiang/jpeg/test_windmill.jpg" ,&jpg_buffer, &jpg_buffer_size);

    JpegPartialDecode(jpg_buffer, jpg_buffer_size, 1,
                      workspace, OUTBUFF_SIZE, out_buffer,
                      &width, &height, 141, 141);

    write_ppm_file("test_partial_1_step.ppm", workspace, width, height);
    write_ppm_file("test_partial_resize.ppm", out_buffer, 141, 141);

    return 0;
}