#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "writer.h"

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageFileWriter.h>
#include <itkResampleImageFilter.h>
#include "itkAffineTransform.h"
#include "itkNearestNeighborInterpolateImageFunction.h"

void ParamSetter(itk::SmartPointer<kImageType> img, double angle, double step, double xy_res, unsigned short fill_value)
{
    // set spacing
    kImageType::SpacingType spacing;

    spacing[0] = xy_res;
    spacing[1] = xy_res;
    spacing[2] = step * sin(angle * M_PI/180.0);

    img->SetSpacing(spacing);

    // set origin
    kImageType::PointType newOrigin;
    newOrigin.Fill(0.0);
    img->SetOrigin(newOrigin);

    // set direction
    kImageType::DirectionType direction;
    direction.SetIdentity();
    // direction[0][2] = step * cos(angle * M_PI/180.0);
    img->SetDirection(direction);
}

int main()
{
    // parameters
    std::string in_path("skewed-image-c1.tif");

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

    const kImageType::PointType & origin = img->GetOrigin();
    std::cout << "Origin = ";
    std::cout << origin[0] << ", " << origin[1] << ", " << origin[2] << std::endl;

    const kImageType::DirectionType & direct = img->GetDirection();
    std::cout << "Direction = " << std::endl;
    std::cout << direct << std::endl;

    // write image
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;

    // itk::SmartPointer<ImageTypeOut> deskewed_img = ImageTypeOut::New();
    // ImageTypeOut::IndexType start;
    // start[0] = 0; // first index on X
    // start[1] = 0; // first index on Y
    // start[2] = 0; // first index on Z
    // ImageTypeOut::SizeType size;
    // size[0] = 200; // size along X
    // size[1] = 200; // size along Y
    // size[2] = 200; // size along Z
    // ImageTypeOut::RegionType region;
    // region.SetSize(size);
    // region.SetIndex(start);
    // deskewed_img->SetRegions(region);
    // deskewed_img->Allocate();

    using FilterType = itk::ResampleImageFilter<kImageType, kImageType>;
    FilterType::Pointer filter = FilterType::New();

    using TransformType = itk::AffineTransform<double, kDimensions>;
    TransformType::Pointer transform = TransformType::New();
    filter->SetTransform(transform);

    using InterpolatorType = itk::NearestNeighborInterpolateImageFunction<kImageType, double>; //change to linear
    InterpolatorType::Pointer interpolator = InterpolatorType::New();
    filter->SetInterpolator(interpolator);

    filter->SetDefaultPixelValue(0);

    // pixel spacing in millimeters along X and Y
    const double spacing[kDimensions] = { 1.0, 1.0, 1.0 };
    filter->SetOutputSpacing(spacing);
    // Physical space coordinate of origin for X and Y
    const double originOut[kDimensions] = { 0.0, 0.0, 0.0 };
    filter->SetOutputOrigin(originOut);

    kImageType::DirectionType direction;
    direction.SetIdentity();
    filter->SetOutputDirection(direction);

    kImageType::SizeType size;
    size[0] = 512; // number of pixels along X
    size[1] = 650; // number of pixels along Y
    size[2] = 60; // number of pixels along Y
    filter->SetSize(size);

    filter->SetInput(img);
    filter->Update();

    WriteImageFile<kImageType, ImageTypeOut>(filter->GetOutput(),"param-set-test.tif");

    return EXIT_SUCCESS;
}
