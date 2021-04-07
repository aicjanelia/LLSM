#include "defines.h"
#include "reader.h"
#include "writer.h"
#include "resampler.h"

int main()
{
  std::string input_file_name("examples/560_PSF_piezoScan.tif");

  itk::SmartPointer<kImageType> image = ReadImageFile(input_file_name);

  if (image == nullptr)
  {
    std::cerr << "Unable to read " << input_file_name.c_str() << std::endl;
    return EXIT_FAILURE;
  }

  kImageType::SpacingType in_spacing;
  in_spacing[0] = 0.104;
  in_spacing[1] = 0.104;
  in_spacing[2] = 0.1;
  image->SetSpacing(in_spacing);

  kImageType::SpacingType out_spacing;
  out_spacing[0] = 0.104;
  out_spacing[1] = 0.104;
  out_spacing[2] = 0.4 * sin(31.8 * M_PI/180.0);
  itk::SmartPointer<kImageType> resampled_image = Resampler(image, out_spacing);

  using PixelTypeOut = unsigned short;
  using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
  WriteImageFile<kImageType,ImageTypeOut>(resampled_image,"resampler-test-output.tif");

  return EXIT_SUCCESS;
}