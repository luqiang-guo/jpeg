#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>

#include <jpeglib.h>

#define input_file "test1.jpg"
#define output_file "partial_decoder.ppm"

#define CROP_X 140
#define CROP_Y 183
#define CROP_W 708
#define CROP_H 914

int read_jpeg_file(unsigned char ** buf, unsigned long * size)
{

    int rc, i, j;
    unsigned long jpg_size;
	unsigned char *jpg_buffer;
    struct stat file_info;
    rc = stat(input_file, &file_info);
	if (rc) {
		printf("FAILED to stat source jpg");
		exit(-1);
	}
	jpg_size = file_info.st_size;
	jpg_buffer = (unsigned char*) malloc(jpg_size + 100);

	int fd = open(input_file, O_RDONLY);
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

int decoder(unsigned char* jpg_buffer, unsigned long jpg_buffer_size, unsigned char* out_buffer, int *w, int *h)
{
    int rc;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	int row_stride, width, height, pixel_size;

	cinfo.err = jpeg_std_error(&jerr);	
    //  默认参数
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, jpg_buffer, jpg_buffer_size);

	rc = jpeg_read_header(&cinfo, TRUE);

	if (rc != 1) {
		printf("File does not seem to be a normal JPEG");
		exit(-1);
	}

    jpeg_start_decompress(&cinfo);
	
	width = cinfo.output_width;
	height = cinfo.output_height;
    *w = cinfo.output_width;
	*h = cinfo.output_height;
    // printf("width = %d, height= %d \n", width, height);
	pixel_size = cinfo.output_components;

	row_stride = width * pixel_size;

    // printf("tmp = %d \n", tmp);

	while (cinfo.output_scanline < height) {
		unsigned char *buffer_array[1];
		buffer_array[0] = out_buffer + cinfo.output_scanline * row_stride;

		jpeg_read_scanlines(&cinfo, buffer_array, 1);

	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

    return 0;
}

int partial_decoder(unsigned char* jpg_buffer, unsigned long jpg_buffer_size, unsigned char* out_buffer)
{
    int rc;
    unsigned int crop_x=CROP_X, crop_y=CROP_Y, crop_width=CROP_W, crop_height=CROP_H;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	int row_stride, width, height, pixel_size;

	cinfo.err = jpeg_std_error(&jerr);	
    //  默认参数
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, jpg_buffer, jpg_buffer_size);

	rc = jpeg_read_header(&cinfo, TRUE);

	if (rc != 1) {
		printf("File does not seem to be a normal JPEG");
		exit(-1);
	}

    jpeg_start_decompress(&cinfo);
	
	width = cinfo.output_width;
	height = cinfo.output_height;
    // printf("width = %d, height= %d \n", width, height);
	pixel_size = cinfo.output_components;

	row_stride = CROP_W * pixel_size;

    jpeg_crop_scanline(&cinfo, &crop_x, &crop_width);

    unsigned int tmp;

    if ((tmp = jpeg_skip_scanlines(&cinfo, crop_y)) != crop_y) {
      printf("jpeg_skip_scanlines() returned %d rather than %d\n", tmp, crop_y);
      return -1;
    }

    // printf("tmp = %d \n", tmp);

	while (cinfo.output_scanline < crop_y + crop_height) {
		unsigned char *buffer_array[1];
		buffer_array[0] = out_buffer + (cinfo.output_scanline - tmp) * row_stride;

		jpeg_read_scanlines(&cinfo, buffer_array, 1);
        // printf("output_scanline = %d, \n", cinfo.output_scanline);

	}
    jpeg_skip_scanlines(&cinfo, cinfo.output_height - crop_y - crop_height);

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

    return 0;
}

#define OUTBUFF_SIZE (1024*1024*1024)
#define FOR_LOOP 5000

int main(void)
{
    int rc, i, j;
    unsigned long jpg_buffer_size;
	unsigned char *jpg_buffer;
    int width, height;

    unsigned char *out_buffer = (unsigned char*) malloc(OUTBUFF_SIZE);
    memset(out_buffer, 0, OUTBUFF_SIZE);

    read_jpeg_file(&jpg_buffer, &jpg_buffer_size);

    float time1, time2;
    long  tmp;
    struct timeval t1;
    struct timeval t2;

    gettimeofday(&t1, NULL);
    for(i = 0; i < FOR_LOOP; i++)
    {
        decoder(jpg_buffer, jpg_buffer_size, out_buffer, &width, &height);
    }
    gettimeofday(&t2, NULL);
    tmp = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
    time1 = (tmp/1000000.0f);
    printf("all decoder time = %f us, %f FPS \n", time1, FOR_LOOP/time1);

    write_ppm_file("all_decoder.ppm", out_buffer, width, height);

    gettimeofday(&t1, NULL);
    for(i = 0; i < FOR_LOOP; i++)
    {
        partial_decoder(jpg_buffer, jpg_buffer_size, out_buffer);
    }
    gettimeofday(&t2, NULL);
    tmp = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
    time2 = (tmp/1000000.0f);
    printf("partial  decoder time = %f us %f FPS \n", time2, FOR_LOOP/time2);

    write_ppm_file(output_file, out_buffer, CROP_W, CROP_H);

    printf("Partial increase of %f %%. \n", (time1-time2)/time1);
    return 0;
}