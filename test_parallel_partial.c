#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <jpeglib.h>


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

int partial_decoder(unsigned char* jpg_buffer, unsigned long jpg_buffer_size, unsigned char* out_buffer,
                    unsigned int crop_x, unsigned int crop_y, unsigned int crop_width, unsigned int crop_height)
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
    // printf("width = %d, height= %d \n", width, height);
	pixel_size = cinfo.output_components;

	row_stride = crop_width * pixel_size;

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

int test (char * file_name, char *ppm_name, char* partial_ppm_name)
{
    int rc, i, j;
    unsigned long jpg_buffer_size;
	unsigned char *jpg_buffer;
    int width, height;


    unsigned char *out_buffer = (unsigned char*) malloc(OUTBUFF_SIZE);
    memset(out_buffer, 0, OUTBUFF_SIZE);

    read_jpeg_file(file_name, &jpg_buffer, &jpg_buffer_size);

    float time1, time2;
    long  tmp;
    struct timeval t1;
    struct timeval t2;

    gettimeofday(&t1, NULL);
    #pragma omp parallel for
    for(i = 0; i < FOR_LOOP; i++)
    {
        decoder(jpg_buffer, jpg_buffer_size, out_buffer, &width, &height);
    }
    gettimeofday(&t2, NULL);
    tmp = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
    time1 = (tmp/1000000.0f);
    printf("%s :w =  %d , h =  %d \n", file_name, width, height);
    printf("all decoder time = %f us, %f FPS \n", time1, FOR_LOOP/time1);

    write_ppm_file(ppm_name, out_buffer, width, height);

    unsigned int crop_x, crop_y, crop_width, crop_height;

    crop_width = sqrt(IOU) * width;
    crop_height = sqrt(IOU) * height;
    crop_x = (width - crop_width) / 2;
    crop_y = (height - crop_height) / 2;

    printf("crop x = %d, y = %d, w = %d, h = %d \n", crop_x, crop_y, crop_width, crop_height);
    gettimeofday(&t1, NULL);

    #pragma omp parallel for
    for(i = 0; i < FOR_LOOP; i++)
    {
        partial_decoder(jpg_buffer, jpg_buffer_size, out_buffer, crop_x, crop_y, crop_width, crop_height);
    }
    gettimeofday(&t2, NULL);
    tmp = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
    time2 = (tmp/1000000.0f);
    printf("partial  decoder time = %f us %f FPS \n", time2, FOR_LOOP/time2);

    write_ppm_file(partial_ppm_name, out_buffer, crop_width, crop_height);

    printf("Partial increase of %f %%. \n", (((FOR_LOOP/time2)/ (FOR_LOOP/time1)) -1)* 100.0);

    printf("%d * %d | %d * %d | %f fps, %f fps, %f %% \n", width, height, crop_width, 
            crop_height, FOR_LOOP/time1, FOR_LOOP/time2, (((FOR_LOOP/time2)/ (FOR_LOOP/time1)) -1)* 100.0);

    free(out_buffer);
    return 0;
}

int main()
{
    char jpeg_file[64];
    char ppm_file[64];
    char partial_ppm_file[64];

    // omp
    omp_set_num_threads(THREAD_NUM);

    for(int i = 0; i <= 18; i++)
    {
        sprintf(jpeg_file, "./jpeg/%d.JPEG", i);
        sprintf(ppm_file, "./jpeg/%d.ppm", i);
        sprintf(partial_ppm_file, "./jpeg/partial_%d.ppm", i);
        printf("test decoder file %s, %s, %s \n", jpeg_file, ppm_file, partial_ppm_file);
        test(jpeg_file, ppm_file, partial_ppm_file);
    }
}