#pragma once

#include "defines.h"
#include "utils.h"

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageFileWriter.h>

template <class TImageIn, class TImageOut>
void WriteImageFile(itk::SmartPointer<TImageIn> image_in, std::string out_path)
{
  itk::SmartPointer<TImageOut> image_output = ConvertImage<TImageIn,TImageOut>(image_in);

  using WriterType = itk::ImageFileWriter<TImageOut>;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(out_path);
  writer->SetInput(image_output);
  writer->Update();
}
