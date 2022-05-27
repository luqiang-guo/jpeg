#include <iostream>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <opencv2/opencv.hpp>

#define OUTBUFF_SIZE (2048*2048*16)

void opencv_decoder(unsigned char * data,size_t len, unsigned char * dst, int *width, int *height)
{
    cv::Mat image = cv::imdecode(cv::Mat(1, len, CV_8UC1, data), cv::IMREAD_COLOR);
	cv::Mat dst_mat(image.rows, image.cols, CV_8UC3, dst, cv::Mat::AUTO_STEP);
	cv::cvtColor(image, dst_mat, cv::COLOR_BGR2RGB);
	*width = dst_mat.cols;
	*height = dst_mat.rows;
}


void opencv_resize(unsigned char *buf, int width, int height,unsigned char * dst_buf, int dst_width, int dst_height)
{
	cv::Mat image(height, width, CV_8UC3, buf, cv::Mat::AUTO_STEP);
	cv::Mat dst(dst_height, dst_width, CV_8UC3, dst_buf, cv::Mat::AUTO_STEP);

	cv::resize(image, dst, cv::Size(dst_width, dst_height), 0, 0, cv::INTER_LINEAR);
}

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


int main()
{
    unsigned long jpg_buffer_size;
	unsigned char *jpg_buffer;
    int width, height;


    unsigned char *out_buffer = (unsigned char*) malloc(OUTBUFF_SIZE);
    memset(out_buffer, 0, OUTBUFF_SIZE);

    read_jpeg_file("test1.jpg", &jpg_buffer, &jpg_buffer_size);

    opencv_decoder(jpg_buffer, jpg_buffer_size, out_buffer, &width, &height);

	unsigned char *resize_buffer = (unsigned char*) malloc(OUTBUFF_SIZE);
	opencv_resize(out_buffer, width, height,resize_buffer,  2*width, 2*height);

	write_ppm_file("test_opencv.ppm", out_buffer, width, height);
	write_ppm_file("test_opencv_resize.ppm", resize_buffer, 2*width, 2*height);

}
