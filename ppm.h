
#ifndef __PPM_H__
#define __PPM_H__
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


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

#endif