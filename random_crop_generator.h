#ifndef __RANDOM_CROP_GENERATOR_H__
#define __RANDOM_CROP_GENERATOR_H__
#include <iostream>
#include <vector>

class CropWindow
{
public:
    CropWindow():anchor(2, 0), shape(2, 0) {}

std::vector<unsigned int> anchor;
std::vector<unsigned int> shape;
};

class RandomCropGenerator
{
public:
    RandomCropGenerator() {}
    void GenerateCropWindow(std::vector<unsigned int> shape, CropWindow *crop);
};


#endif

