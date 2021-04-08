#pragma once

#include "defines.h"

#include <itkImage.h>
#include <itkSubtractImageFilter.h>
#include <itkClampImageFilter.h>

itk::SmartPointer<kImageType> SubtractConstantClamped(itk::SmartPointer<kImageType> img, kPixelType constant)
{
    using SubtractImageFilterType = itk::SubtractImageFilter<kImageType, kImageType, kImageType>;
    SubtractImageFilterType::Pointer subtract_filter = SubtractImageFilterType::New();
    subtract_filter->SetInput(img);
    subtract_filter->SetConstant2(constant);
    subtract_filter->Update();

    using ClampFilterType = itk::ClampImageFilter<kImageType, kImageType>;
    ClampFilterType::Pointer clamp_filter = ClampFilterType::New();
    clamp_filter->SetInput(subtract_filter->GetOutput());
    clamp_filter->SetBounds(0.0, 1.0);
    clamp_filter->Update();

    return clamp_filter->GetOutput();
}

itk::SmartPointer<kImageType> SubtractConstant(itk::SmartPointer<kImageType> img, kPixelType constant)
{
    using SubtractImageFilterType = itk::SubtractImageFilter<kImageType, kImageType, kImageType>;
    SubtractImageFilterType::Pointer subtract_filter = SubtractImageFilterType::New();
    subtract_filter->SetInput(img);
    subtract_filter->SetConstant2(constant);
    subtract_filter->Update();

    return subtract_filter->GetOutput();
}