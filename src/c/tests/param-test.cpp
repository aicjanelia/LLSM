#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "writer.h"

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageFileWriter.h>

void ParamSetter(itk::SmartPointer<kImageType> img, double angle, double step, double xy_res, unsigned short fill_value)
{
    // set spacing
    kImageType::SpacingType spacing;

    spacing[0] = xy_res;
    spacing[1] = xy_res;
    spacing[2] = step * cos(angle * M_PI/180.0);

    img->SetSpacing(spacing);

    // set origin


    // set direction
}

int main()
{
    // parameters
    std::string in_path("/nrs/aic/instruments/llsm/pipeline-test/chad/20210222/scan1/pos1/pos1_Iter_0000_ch0_CAM1_stack0000_560nm_0000000msec_0002518314msecAbs_000x_000y_000z_0000t.tif");

    double xy_res = 0.104;
    double step = 0.4;
    double angle = 31.8;
    unsigned short fill_value = 0;
    unsigned int bit_depth = 16;
    
    // read image
    itk::SmartPointer<kImageType> img = ReadImageFile(in_path);

    if (img == nullptr)
    {
    std::cerr << "Unable to read " << in_path.c_str() << std::endl;
    return EXIT_FAILURE;
    }

    // set parameters
    ParamSetter(img, angle, step, xy_res, fill_value);
    const kImageType::SpacingType & sp = img->GetSpacing();
    std::cout << "Spacing = ";
    std::cout << sp[0] << ", " << sp[1] << ", " << sp[2] << std::endl;

    // write image
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;

    WriteImageFile<kImageType,ImageTypeOut>(img,"/nrs/aic/instruments/llsm/pipeline-test/chad/param-set-test.tif");

    return EXIT_SUCCESS;
}
