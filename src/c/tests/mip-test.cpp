#define _USE_MATH_DEFINES

#include "defines.h"
#include <cmath>
#include "reader.h"
#include "writer.h"
#include "mip.h"
#include "resampler.h"

int main()
{
  std::string input_file_name("examples/cell1_ch0_CAM1_stack0000_560nm_0000000msec_0007807218msecAbs_000x_000y_000z_0000t_deskew.tif");
  std::string out_path = ".tif";

  kImageType::Pointer image = ReadImageFile<kImageType>(input_file_name, true);

  if (image == nullptr)
  {
    std::cerr << "Unable to read " << input_file_name.c_str() << std::endl;
    return EXIT_FAILURE;
  }

  kImageType::SpacingType in_spacing;
  in_spacing[0] = 0.104;
  in_spacing[1] = 0.104;
  in_spacing[2] = 0.211;
  image->SetSpacing(in_spacing);

  image = Resampler(image, 0.104, true);
  std::string labels[] = {"_x", "_y", "_z"};

  for (unsigned int i = 0; i < 3; ++i) 
  {
    std::string axis_out_path = AppendPath(out_path, labels[i]);
    using ProjectionType = itk::Image<kPixelType, 2>;
    ProjectionType::Pointer mip_img = MaxIntensityProjection(image, i, true);

    // write file
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, 2>;
    WriteImageFile<ProjectionType, ImageTypeOut>(mip_img, axis_out_path);
  }

  return EXIT_SUCCESS;
}