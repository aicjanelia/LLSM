#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "writer.h"
#include "deskew.h"

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageFileWriter.h>
#include <itkResampleImageFilter.h>
#include "itkAffineTransform.h"
#include "itkLinearInterpolateImageFunction.h"

int main()
{
    // read image
    std::string image_path("examples/cell2_ch0_CAM1_stack0001_560nm_0004529msec_0009990533msecAbs_000x_000y_000z_0000t.tif");
    itk::SmartPointer<kImageType> image = ReadImageFile(image_path, true);

    if (image == nullptr)
    {
        std::cerr << "Unable to read " << image_path.c_str() << std::endl;
        return EXIT_FAILURE;
    }

    // parameters
    double xy_res = 0.104;
    double stage_move_distance = 0.4;
    double angle = 31.8;
    unsigned short fill_value = 0;
    unsigned int bit_depth = 16;

    // deskew
    itk::SmartPointer<kImageType> deskew_img = Deskew(image, angle, stage_move_distance, xy_res, fill_value, true);

    // write image
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;

    WriteImageFile<kImageType, ImageTypeOut>(deskew_img,"deskew-test.tif", true);

    return EXIT_SUCCESS;
}
