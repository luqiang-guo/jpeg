#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <opencv2/opencv.hpp>

#define W 192
#define H 192

void write_ppm_file(char * file_name, unsigned char * buff, 
                     int width, int height)
{
    int rc; 
    int fd = open(file_name, O_CREAT | O_WRONLY, 0666);
	char buf[1024];
    unsigned long size = width * height * 3;
	rc = sprintf(buf, "P6 %d %d 255\n", width, height);
	write(fd, buf, rc);
	write(fd, buff, size); 

	close(fd);
}

void write_jpeg_file(char * file_name, unsigned char * buff, 
                     int len)
{
    int rc; 
    int fd = open(file_name, O_CREAT | O_WRONLY, 0666);
	write(fd, buff, len); 
	close(fd);
}


int main(void)
{
    uint8_t* raw_data = (uint8_t*)malloc(W*H*3);

    for (int i=0; i < H; i++)
    {
        for(int j=0; j < W; j++)
        {
            uint8_t r,g,b;
            if(i < W/2 && j < H/2)
            {
                r = 255;
                g = 0;
                b = 0;
            }
            else if((i >= W/2 && j < H/2) || (i < W/2 && j >= H/2))
            {
                //green
                r = 0;
                g = 255;
                b = 0;
            }
            else if(i >= W/2 && j >= H/2)
            {
                //blue
                r = 0;
                g = 0;
                b = 255;
            }

            raw_data[3*(i*W+j)] = b;
            raw_data[3*(i*W+j)+1] = g;
            raw_data[3*(i*W+j)+2] = r;
        }
    }

    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(100);

    cv::Mat raw(H, W, CV_8UC3, raw_data, cv::Mat::AUTO_STEP);

    // cv::imencode
    std::vector<uint8_t> img_encode;
    cv::imencode(".jpg", raw, img_encode);
    
    write_jpeg_file("gtest.jpg", img_encode.data(), img_encode.size());
    // cv::imwrite("123.jpg", raw, compression_params);

    return 0;
}