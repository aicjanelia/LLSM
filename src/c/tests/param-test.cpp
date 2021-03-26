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

void ParamSetter(itk::SmartPointer<kImageType> img, double angle, double stage_move_distance, double xy_res, unsigned short fill_value)
{
    // set spacing
    kImageType::SpacingType spacing;

    spacing[0] = xy_res;
    spacing[1] = xy_res;
    spacing[2] = stage_move_distance * sin(angle * M_PI/180.0);

    img->SetSpacing(spacing);

    // set origin
    kImageType::PointType newOrigin;
    newOrigin.Fill(0.0);
    img->SetOrigin(newOrigin);

    // set direction
    kImageType::DirectionType direction;
    direction.SetIdentity();
    direction[0][2] = stage_move_distance * cos(angle * M_PI/180.0) / spacing[2];
    img->SetDirection(direction);
}

int main()
{
    // parameters
    std::string in_path("idk.tif");

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

    // set parameters
    ParamSetter(img, angle, stage_move_distance, xy_res, fill_value);
    const kImageType::SpacingType & sp = img->GetSpacing();
    std::cout << "Spacing = ";
    std::cout << sp[0] << ", " << sp[1] << ", " << sp[2] << std::endl;

    const kImageType::PointType & origin = img->GetOrigin();
    std::cout << "Origin = ";
    std::cout << origin[0] << ", " << origin[1] << ", " << origin[2] << std::endl;

    const kImageType::DirectionType & direct = img->GetDirection();
    std::cout << "Direction = " << std::endl;
    std::cout << direct << std::endl;

    std::cout << "Direction set val: " << stage_move_distance * cos(angle * M_PI/180.0) << std::endl;

    // write image
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;

    using FilterType = itk::ResampleImageFilter<kImageType, kImageType>;
    FilterType::Pointer filter = FilterType::New();

    using TransformType = itk::AffineTransform<double, kDimensions>;
    TransformType::Pointer transform = TransformType::New();
    filter->SetTransform(transform);

    // using InterpolatorType = itk::LinearInterpolateImageFunction<kImageType, double>; //change to linear
    // InterpolatorType::Pointer interpolator = InterpolatorType::New();
    // filter->SetInterpolator(interpolator);

    filter->SetDefaultPixelValue(0);

    // pixel spacing in millimeters along X and Y
    filter->SetOutputSpacing(sp);
    // Physical space coordinate of origin for X and Y
    const double originOut[kDimensions] = { 0.0, 0.0, 0.0 };
    filter->SetOutputOrigin(originOut);

    kImageType::DirectionType direction;
    direction.SetIdentity();
    filter->SetOutputDirection(direction);

    // Calculate output size
    double shift = img->GetDirection()[0][2];
    kImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
    std::cout << "Old Size:" << size[0] << ", " << size[1] << ", " << size[2] << std::endl;
    //const int deskewed_width = ceil(width + (fabs(shift) * (nslices-1)));
    size[0] = ceil(size[0] + (fabs(shift*sp[2]) / sp[0] * (size[2]-1)));

    std::cout << "New Size:" << size[0] << ", " << size[1] << ", " << size[2] << std::endl;

    // kImageType::SizeType size;
    // size[0] = 512; // number of pixels along X
    // size[1] = 650; // number of pixels along Y
    // size[2] = 60; // number of pixels along Y
    filter->SetSize(size);

    filter->SetInput(img);
    filter->Update();

    WriteImageFile<kImageType, ImageTypeOut>(filter->GetOutput(),"param-set-test.tif");

    return EXIT_SUCCESS;
}
