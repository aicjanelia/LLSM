#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "writer.h"

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageFileWriter.h>
#include <itkResampleImageFilter.h>
#include "itkAffineTransform.h"
#include "itkLinearInterpolateImageFunction.h"

itk::SmartPointer<kImageType> Deskew(itk::SmartPointer<kImageType> img, double angle, double stage_move_distance, double xy_res, unsigned short fill_value)
{
    // compute shift
    const double shift = stage_move_distance * cos(angle * M_PI/180.0) / xy_res;

    // set up resample filter
    using FilterType = itk::ResampleImageFilter<kImageType, kImageType>;
    FilterType::Pointer filter = FilterType::New();

    using TransformType = itk::AffineTransform<double, kDimensions>;
    TransformType::Pointer transform = TransformType::New();
    transform->Shear(0, 2, -shift);
    filter->SetTransform(transform);

    using InterpolatorType = itk::LinearInterpolateImageFunction<kImageType, double>;
    InterpolatorType::Pointer interpolator = InterpolatorType::New();
    filter->SetInterpolator(interpolator);

    filter->SetDefaultPixelValue(0.0);

    // calculate output size
    kImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
    std::cout << "Old Size:" << size[0] << ", " << size[1] << ", " << size[2] << std::endl;
    size[0] = ceil(size[0] + (fabs(shift) * (size[2]-1)));
    std::cout << "New Size:" << size[0] << ", " << size[1] << ", " << size[2] << std::endl;

    filter->SetSize(size);

    // perform deskew
    filter->SetInput(img);
    filter->Update();

    // set spacing
    kImageType::SpacingType spacing;

    spacing[0] = xy_res;
    spacing[1] = xy_res;
    spacing[2] = stage_move_distance * sin(angle * M_PI/180.0);

    img->SetSpacing(spacing);

    std::cout << spacing[0] << ", " << spacing[1] << ", " << spacing[2] << std::endl;

    return filter->GetOutput();
}

int main()
{
    // parameters
    std::string in_path("/nrs/aic/instruments/llsm/pipeline-test/stage-scan/hobsonc-20210325/cell1/cell1_Iter_0003_ch0_CAM1_stack0000_488nm_0000000msec_0014278808msecAbs_000x_000y_000z_0003t.tif");

    double xy_res = 0.104;
    double stage_move_distance = 0.26667;
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

    // deskew
    itk::SmartPointer<kImageType> deskew_img = Deskew(img, angle, stage_move_distance, xy_res, fill_value);

    // write image
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;

    WriteImageFile<kImageType, ImageTypeOut>(deskew_img,"deskew-test.tif");

    return EXIT_SUCCESS;
}
