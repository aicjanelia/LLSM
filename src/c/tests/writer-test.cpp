#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "writer.h"

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageFileWriter.h>
// #include <itkMinimumMaximumImageCalculator.h>

int main()
{
  // read image
  std::string image_path("examples/cell2_ch0_CAM1_stack0001_560nm_0004529msec_0009990533msecAbs_000x_000y_000z_0000t.tif");
  itk::SmartPointer<kImageType> image = ReadImageFile<kImageType>(image_path, true);

  using PixelTypeOut = unsigned short;
  using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;

  WriteImageFile<kImageType,ImageTypeOut>(image,"write-test-output.tif", true);

  return EXIT_SUCCESS;
}
