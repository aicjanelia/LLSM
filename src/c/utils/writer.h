#pragma once

#include "defines.h"
#include "utils.h"

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageFileWriter.h>

template <class TImageIn, class TImageOut>
void WriteImageFile(typename TImageIn::Pointer image_in, std::string out_path, bool verbose=false)
{
  typename TImageOut::Pointer image_output = ConvertImage<TImageIn,TImageOut>(image_in);

  // This is a hack because the ITK Tiff writer only writes out the XY resolution in inches
  typename TImageIn::SpacingType spacing = image_in->GetSpacing();
  spacing[0] = (spacing[0] / spacing[2]) * 25.4;
  spacing[1] = (spacing[1] / spacing[2]) * 25.4;
  image_output->SetSpacing(spacing);

  using WriterType = itk::ImageFileWriter<TImageOut>;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(out_path);
  writer->SetInput(image_output);
  writer->Update();

  if (verbose)
  {
    std::cout << "Wrote " << out_path.c_str() << std::endl;
  }
}
