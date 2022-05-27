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
		printf("FAILED to stat source jpg\n");
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

int JpegPartialDecode(const unsigned char* data, size_t length,int crop_generator, 
                      const std::string& color_space, unsigned char* dst, int& c_w, int& c_h) {
  struct jpeg_decompress_struct cinfo = {};
  struct jpeg_error_mgr jerr = {};
  int rc = 0;
  unsigned char* crop_buf = nullptr;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, data, length);

  rc = jpeg_read_header(&cinfo, TRUE);
  if (rc != 1) { return -1; }

  jpeg_start_decompress(&cinfo);
  int width = cinfo.output_width;
  int height = cinfo.output_height;
  int pixel_size = cinfo.output_components;

  std::vector<unsigned char> tmp_buf(width * height * pixel_size);
  crop_buf = tmp_buf.data();

  unsigned int u_crop_x = 0, u_crop_y = 0, tmp_u_crop_w = 0, u_crop_w = 0, u_crop_h = 0;

  if (crop_generator) {
    u_crop_w = sqrt(IOU) * width;
    tmp_u_crop_w = u_crop_w;
    u_crop_h = sqrt(IOU) * height;
    u_crop_x = (width - u_crop_w) / 2;
    u_crop_y = (height - u_crop_h) / 2;
  } else {
    u_crop_x = 0;
    u_crop_y = 0;
    u_crop_w = width;
    u_crop_h = height;
    tmp_u_crop_w = u_crop_w;
  }

  printf("u_crop_x = %d, tmp_u_crop_w = %d \n", u_crop_x, tmp_u_crop_w);
  jpeg_crop_scanline(&cinfo, &u_crop_x, &tmp_u_crop_w);
  int row_stride = tmp_u_crop_w * pixel_size;
  if (jpeg_skip_scanlines(&cinfo, u_crop_y) != u_crop_y) { return -2; }

  while (cinfo.output_scanline < u_crop_y + u_crop_h) {
    unsigned char* buffer_array[1];
    buffer_array[0] = crop_buf + (cinfo.output_scanline - u_crop_y) * row_stride;
    jpeg_read_scanlines(&cinfo, buffer_array, 1);
  }

  jpeg_skip_scanlines(&cinfo, cinfo.output_height - u_crop_y - u_crop_h);
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  cv::Mat image(u_crop_h, tmp_u_crop_w, CV_8UC3, crop_buf, cv::Mat::AUTO_STEP);

  cv::Rect roi;
  cv::Mat cropped;

  if (u_crop_w != tmp_u_crop_w) {
    roi.x = tmp_u_crop_w - u_crop_w;
    roi.y = 0;
    roi.width = u_crop_w;
    roi.height = u_crop_h;
    printf("x=%d, y=%d, w=%d, h=%d\n", roi.x, roi.y, roi.width, roi.height);
    image(roi).copyTo(cropped);
  } else {
    cropped = image;
  }


  c_w = u_crop_w;
  c_h = u_crop_h;

  std::cout <<  cropped.channels() << std::endl;
  std::cout << cropped.total() * cropped.elemSize() << std::endl;

  memcpy(dst, cropped.ptr(), cropped.total() * cropped.elemSize());

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

    read_jpeg_file("test_windmill.jpg" ,&jpg_buffer, &jpg_buffer_size);

    JpegPartialDecode(jpg_buffer, jpg_buffer_size, 0,
                        "RGB", out_buffer, width, height);

    write_ppm_file("test_nngraph_1_step.ppm", out_buffer, width, height);

    return 0;
}