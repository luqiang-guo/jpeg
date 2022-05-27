#include "random_crop_generator.h"
#include <math.h>

#define IOU 0.5
//random_crop_gen->GenerateCropWindow({height, width}, &crop);
// u_crop_y = crop.anchor.At(0);
// u_crop_x = crop.anchor.At(1);
// u_crop_h = crop.shape.At(0);
// u_crop_w = crop.shape.At(1);

void RandomCropGenerator::GenerateCropWindow(std::vector<unsigned int> shape, CropWindow *crop)
{

    // width
    crop->shape[1] = sqrt(IOU) * shape[1];
    //
    crop->shape[0] = sqrt(IOU) * shape[0];
    crop_h = sqrt(IOU) * height;
    crop_x = (width - crop_w) / 2;
    crop_y = (height - crop_h) / 2;
}