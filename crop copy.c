#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>

#include <jpeglib.h>

#define input_file "test_windmill.jpg"
#define output_file "out2.ppm"

#define CROP_X 100
#define CROP_Y 100
#define CROP_W 100
#define CROP_H 100

int main (int argc, char *argv[]) {
	int rc, i, j;

    unsigned int crop_x=CROP_X, crop_y=CROP_Y, crop_width=CROP_W, crop_height=CROP_H;

	char *syslog_prefix = (char*) malloc(1024);
	sprintf(syslog_prefix, "%s", argv[0]);
	openlog(syslog_prefix, LOG_PERROR | LOG_PID, LOG_USER);


	// Variables for the source jpg
	struct stat file_info;
	unsigned long jpg_size;
	unsigned char *jpg_buffer;

	// Variables for the decompressor itself
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	// Variables for the output buffer, and how long each row is
	unsigned long bmp_size;
	unsigned char *bmp_buffer;
	int row_stride, width, height, pixel_size;

	rc = stat(input_file, &file_info);
	if (rc) {
		syslog(LOG_ERR, "FAILED to stat source jpg");
		exit(EXIT_FAILURE);
	}
	jpg_size = file_info.st_size;
	jpg_buffer = (unsigned char*) malloc(jpg_size + 100);

	int fd = open(input_file, O_RDONLY);
	i = 0;
	while (i < jpg_size) {
		rc = read(fd, jpg_buffer + i, jpg_size - i);
		syslog(LOG_INFO, "Input: Read %d/%lu bytes", rc, jpg_size-i);
		i += rc;
	}
	close(fd);


	cinfo.err = jpeg_std_error(&jerr);	
    //  默认参数
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, jpg_buffer, jpg_size);

    // TRUE  ?
	rc = jpeg_read_header(&cinfo, TRUE);

	if (rc != 1) {
		syslog(LOG_ERR, "File does not seem to be a normal JPEG");
		exit(EXIT_FAILURE);
	}

	syslog(LOG_INFO, "Proc: Initiate JPEG decompression");

	jpeg_start_decompress(&cinfo);
	
	width = cinfo.output_width;
	height = cinfo.output_height;
	pixel_size = cinfo.output_components;

	syslog(LOG_INFO, "Proc: Image is %d by %d with %d components", 
			width, height, pixel_size);

	bmp_size =  width * height * pixel_size;
	bmp_buffer = (unsigned char*) malloc(bmp_size);


	row_stride = width * pixel_size;

    jpeg_crop_scanline(&cinfo, &crop_x, &crop_width);

    unsigned int tmp;

    if ((tmp = jpeg_skip_scanlines(&cinfo, crop_y)) != crop_y) {
      printf("jpeg_skip_scanlines() returned %d rather than %d\n", tmp, crop_y);
      return -1;
    }

    printf("tmp = %d \n", tmp);

	while (cinfo.output_scanline < crop_y + crop_height) {
		unsigned char *buffer_array[1];
		buffer_array[0] = bmp_buffer + (cinfo.output_scanline - tmp) * row_stride;

		jpeg_read_scanlines(&cinfo, buffer_array, 1);
        printf("output_scanline = %d, \n", cinfo.output_scanline);

	}
	syslog(LOG_INFO, "Proc: Done reading scanlines");



	jpeg_finish_decompress(&cinfo);


	jpeg_destroy_decompress(&cinfo);

	free(jpg_buffer);
	 
	fd = open(output_file, O_CREAT | O_WRONLY, 0666);
	char buf[1024];

	rc = sprintf(buf, "P6 %d %d 255\n", width, height);
	write(fd, buf, rc);
	write(fd, bmp_buffer, bmp_size); 

	close(fd);
	// free(bmp_buffer);

	syslog(LOG_INFO, "End of decompression");
	return EXIT_SUCCESS;
}