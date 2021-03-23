#pragma once

#include "defines.h"
#include "utils.h"

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageFileWriter.h>

#include <string>

template <class TImageIn, class TImageOut>
bool WriteImageFile(itk::SmartPointer<TImageIn> image_in, std::string file_path_out)
{
  itk::SmartPointer<TImageOut> image_output = ConvertImage<TImageIn,TImageOut>(image_in);

  using WriterType = itk::ImageFileWriter<TImageOut>;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(file_path_out);
  writer->SetInput(image_output);

  try
  {
    writer->Update();
  }
  catch (itk::ExceptionObject & error)
  {
    std::cerr << "Error: " << error << std::endl;
    return false;
  }

  return true;
}
