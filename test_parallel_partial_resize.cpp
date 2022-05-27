#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <jpeglib.h>
#include <opencv2/opencv.hpp>

#include <omp.h>

#define OUTBUFF_SIZE (2048*2048*16)

#define THREAD_NUM 64
#define FOR_LOOP (THREAD_NUM*1000)
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


void opencv_decoder(const unsigned char* data, size_t length, int crop_generator , 
                    unsigned char* dst, int target_width, int target_height) {
    cv::Mat image = cv::imdecode(cv::Mat(1, length, CV_8UC1, const_cast<unsigned char*>(data)), cv::IMREAD_COLOR);
    cv::Mat cropped;

    if (crop_generator) {
        cv::Rect roi;
        roi.width = sqrt(IOU) * image.cols;
        roi.height = sqrt(IOU) * image.rows;
        roi.x = (image.cols - roi.width) / 2;
        roi.y = (image.rows - roi.height) / 2;
        image(roi).copyTo(cropped);
    } else {
        cropped = image;
    }

    cv::Mat resized;
    cv::resize(cropped, resized, cv::Size(target_width, target_height), 0, 0, cv::INTER_LINEAR);
    cv::Mat dst_mat(target_height, target_width, CV_8UC3, dst, cv::Mat::AUTO_STEP);
    cv::cvtColor(resized, dst_mat, cv::COLOR_BGR2RGB);
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
  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, data, length);

  rc = jpeg_read_header(&cinfo, TRUE);
  if (rc != 1) { return -1; }

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

  *out_w = width;
  *out_h = height;

  return 0;
}


int test (char * file_name, char *ppm_name, char* partial_ppm_name, char* opencv_ppm_name)
{
    int rc, i, j;
    unsigned long jpg_buffer_size;
	unsigned char *jpg_buffer;
    int width, height;

    unsigned char *workspace = (unsigned char*) malloc(OUTBUFF_SIZE);
    unsigned char *out_buffer = (unsigned char*) malloc(OUTBUFF_SIZE);
    memset(out_buffer, 0, OUTBUFF_SIZE);

    read_jpeg_file(file_name, &jpg_buffer, &jpg_buffer_size);

    float time1, time2;
    long  tmp;
    struct timeval t1;
    struct timeval t2;
	// ________________________ OpenCV decoder________________________
    gettimeofday(&t1, NULL);
    #pragma omp parallel for
    for(i = 0; i < FOR_LOOP; i++)
    {
        opencv_decoder(jpg_buffer, jpg_buffer_size, 1, out_buffer, 244, 244);
    }
    gettimeofday(&t2, NULL);
    tmp = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
    time1 = (tmp/1000000.0f);
    // printf("%s :w =  %d , h =  %d \n", file_name, width, height);
    printf("opencv decoder time = %f us, %f FPS \n", time1, FOR_LOOP/time1);

    write_ppm_file(opencv_ppm_name, out_buffer, 244, 244);
	

	// ________________________ partial decoder________________________

    gettimeofday(&t1, NULL);
	
    #pragma omp parallel for
    for(i = 0; i < FOR_LOOP; i++)
    {
        JpegPartialDecode(jpg_buffer, jpg_buffer_size, 1,
                      workspace, OUTBUFF_SIZE, out_buffer,
                      &width, &height, 244, 244);
    }
    gettimeofday(&t2, NULL);
    tmp = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
    time2 = (tmp/1000000.0f);
    printf("partial  decoder time = %f us %f FPS \n", time2, FOR_LOOP/time2);

    write_ppm_file(partial_ppm_name, out_buffer, 244, 244);

    printf("Partial increase of %f %%. \n", (((FOR_LOOP/time2)/ (FOR_LOOP/time1)) -1)* 100.0);

    printf("%d * %d | %f fps, %f fps, %f %% \n", width, height,
            FOR_LOOP/time1, FOR_LOOP/time2, (((FOR_LOOP/time2)/ (FOR_LOOP/time1)) -1)* 100.0);

    free(out_buffer);
    return 0;
}


int main()
{
    char jpeg_file[64];
    char ppm_file[64];
	char opencv_file[64];
    char partial_ppm_file[64];

    // omp
    omp_set_num_threads(THREAD_NUM);

    for(int i = 0; i <= 18; i++)
    {
        sprintf(jpeg_file, "./jpeg/%d.JPEG", i);
        sprintf(ppm_file, "./jpeg/%d.ppm", i);
		sprintf(opencv_file, "./jpeg/opencv_%d.ppm", i);
        sprintf(partial_ppm_file, "./jpeg/partial_%d.ppm", i);
        printf("test decoder file %s, %s, %s \n", jpeg_file, ppm_file, partial_ppm_file);
        test(jpeg_file, ppm_file, partial_ppm_file, opencv_file);
    }
}