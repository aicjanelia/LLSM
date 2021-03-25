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
  std::string input_file_name("skewed-image-c1.tif");

  itk::SmartPointer<kImageType> image = ReadImageFile(input_file_name);

  using PixelTypeOut = unsigned short;
  using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;

  WriteImageFile<kImageType,ImageTypeOut>(image,"write-test-output.tif");

  return EXIT_SUCCESS;
}