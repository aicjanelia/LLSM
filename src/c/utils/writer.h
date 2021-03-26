#pragma once

#include "defines.h"
#include "utils.h"

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageFileWriter.h>

template <class TImageIn, class TImageOut>
void WriteImageFile(itk::SmartPointer<TImageIn> image_in, std::string file_path_out)
{
  itk::SmartPointer<TImageOut> image_output = ConvertImage<TImageIn,TImageOut>(image_in);

  using WriterType = itk::ImageFileWriter<TImageOut>;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(file_path_out);
  writer->SetInput(image_output);
  writer->Update();
}

template <class TImage>
void WriteImageFile(itk::SmartPointer<TImage> image, std::string file_path_out)
{
  using WriterType = itk::ImageFileWriter<TImage>;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(file_path_out);
  writer->SetInput(image);
  writer->Update();
}
