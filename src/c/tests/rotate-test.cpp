// Synthetic tests for the rotate module (ITK required, no file I/O).
#include "rotate.h"
#include "deskew.h"
#include "defines.h"

#include <itkImageRegionConstIteratorWithIndex.h>
#include <itkImageRegionIteratorWithIndex.h>

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>

using llsm_rotate::CropMode;
using Image = kImageType;

namespace
{

bool Near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

Image::Pointer MakeImage(std::size_t nx, std::size_t ny, std::size_t nz, double xy, double z, double value = 0.0)
{
  Image::Pointer img = Image::New();
  Image::SizeType size;
  size[0] = nx;
  size[1] = ny;
  size[2] = nz;
  Image::RegionType region;
  region.SetSize(size);
  img->SetRegions(region);
  Image::SpacingType spacing;
  spacing[0] = xy;
  spacing[1] = xy;
  spacing[2] = z;
  img->SetSpacing(spacing);
  img->Allocate();
  img->FillBuffer(value);
  return img;
}

// Gaussian blob centred at a physical position (um), sigma in um (isotropic in physical space).
Image::Pointer MakeBlob(std::size_t nx, std::size_t ny, std::size_t nz, double xy, double z,
                        std::array<double, 3> centre, double sigma)
{
  Image::Pointer img = MakeImage(nx, ny, nz, xy, z);
  itk::ImageRegionIteratorWithIndex<Image> it(img, img->GetLargestPossibleRegion());
  for (it.GoToBegin(); !it.IsAtEnd(); ++it)
  {
    const auto idx = it.GetIndex();
    const double dx = idx[0] * xy - centre[0];
    const double dy = idx[1] * xy - centre[1];
    const double dz = idx[2] * z - centre[2];
    it.Set(std::exp(-(dx * dx + dy * dy + dz * dz) / (2.0 * sigma * sigma)));
  }
  return img;
}

struct Moments
{
  double mass = 0.0;
  std::array<double, 3> mean{};
  std::array<double, 3> variance{};
};

Moments ComputeMoments(Image::Pointer img)
{
  Moments m;
  itk::ImageRegionConstIteratorWithIndex<Image> it(img, img->GetLargestPossibleRegion());
  Image::PointType p;
  for (it.GoToBegin(); !it.IsAtEnd(); ++it)
  {
    const double v = it.Get();
    img->TransformIndexToPhysicalPoint(it.GetIndex(), p);
    m.mass += v;
    for (int d = 0; d < 3; ++d)
      m.mean[d] += v * p[d];
  }
  for (int d = 0; d < 3; ++d)
    m.mean[d] /= m.mass;
  for (it.GoToBegin(); !it.IsAtEnd(); ++it)
  {
    const double v = it.Get();
    img->TransformIndexToPhysicalPoint(it.GetIndex(), p);
    for (int d = 0; d < 3; ++d)
      m.variance[d] += v * (p[d] - m.mean[d]) * (p[d] - m.mean[d]);
  }
  for (int d = 0; d < 3; ++d)
    m.variance[d] /= m.mass;
  return m;
}

Image::SizeType SizeOf(Image::Pointer img) { return img->GetLargestPossibleRegion().GetSize(); }

Image::Pointer Rot(Image::Pointer img, double angle, double xy, double z, CropMode mode = CropMode::KeepAll,
                   bool isotropic = false, double shift = 0.0, double fill = 0.0)
{
  return Rotate(img, angle, xy, z, shift, mode, isotropic, fill);
}

template <class F>
bool Throws(F f)
{
  try { f(); } catch (const std::invalid_argument &) { return true; }
  return false;
}

} // namespace

int main()
{
  const double xy = 0.108, zs = 0.26828170298;
  const double mosaic = -32.45;

  // --- identity at 0 degrees reproduces the input exactly
  {
    Image::Pointer img = MakeBlob(40, 6, 30, xy, zs, {2.0, 0.3, 4.0}, 0.5);
    Image::Pointer out = Rot(img, 0.0, xy, zs);
    assert(SizeOf(out) == SizeOf(img));
    itk::ImageRegionConstIteratorWithIndex<Image> it(img, img->GetLargestPossibleRegion());
    for (it.GoToBegin(); !it.IsAtEnd(); ++it)
      assert(Near(out->GetPixel(it.GetIndex()), it.Get(), 1e-12));
  }

  // --- sign convention: a blob moves to R*c with R = BigStitcher's y rotation
  {
    const std::array<double, 3> c = {6.0, 0.3, 10.0};
    Image::Pointer img = MakeBlob(120, 6, 80, xy, zs, c, 0.5);
    Image::Pointer out = Rot(img, mosaic, xy, zs);
    const auto m = llsm_rotate::RotationMatrix(mosaic);
    const std::array<double, 3> expect = {m[0] * c[0] + m[2] * c[2], c[1], m[6] * c[0] + m[8] * c[2]};
    const Moments mo = ComputeMoments(out);
    assert(Near(mo.mean[0], expect[0], 0.01));
    // y is the rotation axis, so the y mean must equal the input's. The blob is truncated in y
    // (6 slices), so its mean is not c[1]; compare against the measured input mean instead.
    assert(Near(mo.mean[1], ComputeMoments(img).mean[1], 0.01));
    assert(Near(mo.mean[2], expect[2], 0.01));
    // for -32.45 deg, +x content moves toward +z
    assert(mo.mean[2] > 0.0 && expect[2] > c[2] * 0.0);
  }

  // --- anisotropy: a physical sphere stays a sphere (index-space rotation would not)
  {
    // sigma = 0.8 um: linear interpolation adds ~h^2/6 variance per pass (about 2% along z here)
    Image::Pointer img = MakeBlob(100, 60, 70, xy, zs, {5.0, 3.0, 9.0}, 0.8);
    const Moments before = ComputeMoments(img);
    for (double angle : {mosaic, 31.8, 60.0})
    {
      const Moments after = ComputeMoments(Rot(img, angle, xy, zs));
      assert(Near(after.variance[0] / before.variance[0], 1.0, 0.04));
      assert(Near(after.variance[1] / before.variance[1], 1.0, 0.04));
      assert(Near(after.variance[2] / before.variance[2], 1.0, 0.04));
      assert(Near(after.mass / before.mass, 1.0, 0.03)); // no clipping, same voxel volume
    }
  }

  // --- round trip +theta then -theta recovers the original in the interior
  {
    Image::Pointer img = MakeBlob(100, 6, 70, xy, zs, {5.0, 0.3, 9.0}, 0.6);
    Image::Pointer there = Rot(img, 32.45, xy, zs);
    // Rotate() ignores the input origin (it always pivots about voxel (0,0,0)) and resets it,
    // so capture there's origin o1 first. The second pass then pivots about there's corner, which
    // displaces its content by R(-32.45) * o1 on top of back's own origin.
    const auto o1 = there->GetOrigin();
    Image::Pointer back = Rot(there, -32.45, xy, zs);
    const auto r = llsm_rotate::RotationMatrix(-32.45);
    Image::PointType back_origin = back->GetOrigin();
    back_origin[0] += r[0] * o1[0] + r[2] * o1[2];
    back_origin[2] += r[6] * o1[0] + r[8] * o1[2];
    back->SetOrigin(back_origin); // back is now registered to img's physical frame
    // The offset is not a whole number of voxels, so compare by interpolating back at img's voxels.
    auto interp = itk::LinearInterpolateImageFunction<Image, double>::New();
    interp->SetInputImage(back);
    double max_diff = 0.0;
    std::size_t compared = 0;
    Image::PointType p;
    itk::ImageRegionConstIteratorWithIndex<Image> it(img, img->GetLargestPossibleRegion());
    for (it.GoToBegin(); !it.IsAtEnd(); ++it)
    {
      img->TransformIndexToPhysicalPoint(it.GetIndex(), p);
      if (!interp->IsInsideBuffer(p))
        continue;
      ++compared;
      max_diff = std::max(max_diff, std::fabs(interp->Evaluate(p) - it.Get()));
    }
    assert(compared > img->GetLargestPossibleRegion().GetNumberOfPixels() / 4); // not vacuous
    assert(max_diff < 0.1);
  }

  // --- 360 == 0, 180 flips
  {
    Image::Pointer img = MakeBlob(30, 4, 20, xy, zs, {1.0, 0.2, 2.0}, 0.3);
    Image::Pointer a = Rot(img, 0.0, xy, zs);
    Image::Pointer b = Rot(img, 360.0, xy, zs);
    assert(SizeOf(a) == SizeOf(b));
    itk::ImageRegionConstIteratorWithIndex<Image> it(a, a->GetLargestPossibleRegion());
    for (it.GoToBegin(); !it.IsAtEnd(); ++it)
      assert(Near(b->GetPixel(it.GetIndex()), it.Get(), 1e-9));
    Image::Pointer c = Rot(img, 180.0, xy, zs);
    assert(SizeOf(c) == SizeOf(img));
    const Moments m0 = ComputeMoments(img), m1 = ComputeMoments(c);
    assert(Near(m1.mean[0], -m0.mean[0], 0.01) && Near(m1.mean[2], -m0.mean[2], 0.01));
  }

  // --- y is the rotation axis: planes keep their y position and relative amplitude
  {
    Image::Pointer img = MakeImage(30, 5, 20, xy, zs);
    itk::ImageRegionIteratorWithIndex<Image> it(img, img->GetLargestPossibleRegion());
    for (it.GoToBegin(); !it.IsAtEnd(); ++it)
      it.Set(1.0 + it.GetIndex()[1]);
    Image::Pointer out = Rot(img, mosaic, xy, zs);
    assert(SizeOf(out)[1] == 5);
    Image::IndexType c;
    c[0] = SizeOf(out)[0] / 2;
    c[2] = SizeOf(out)[2] / 2;
    for (int y = 0; y < 5; ++y)
    {
      c[1] = y;
      const double v = out->GetPixel(c);
      assert(Near(v, 1.0 + y, 1e-9)); // interior of a constant-per-plane image
    }
  }

  // --- fill value outside the data, data inside
  {
    Image::Pointer img = MakeImage(40, 4, 40, xy, zs, 1.0);
    Image::Pointer out = Rot(img, mosaic, xy, zs, CropMode::KeepAll, false, 0.0, 7.0);
    Image::IndexType corner;
    corner.Fill(0);
    assert(Near(out->GetPixel(corner), 7.0, 1e-12));
    Image::IndexType centre;
    centre[0] = SizeOf(out)[0] / 2;
    centre[1] = 2;
    centre[2] = SizeOf(out)[2] / 2;
    assert(Near(out->GetPixel(centre), 1.0, 1e-9));
  }

  // --- spacing: preserved by default, isotropic on request; origin is an integer multiple
  {
    Image::Pointer img = MakeBlob(60, 4, 40, xy, zs, {3.0, 0.2, 4.0}, 0.4);
    Image::Pointer out = Rot(img, mosaic, xy, zs);
    assert(Near(out->GetSpacing()[0], xy, 1e-12) && Near(out->GetSpacing()[2], zs, 1e-12));
    assert(Near(out->GetOrigin()[0] / xy, std::round(out->GetOrigin()[0] / xy), 1e-9));
    assert(Near(out->GetOrigin()[2] / zs, std::round(out->GetOrigin()[2] / zs), 1e-9));
    Image::Pointer iso = Rot(img, mosaic, xy, zs, CropMode::KeepAll, true);
    assert(Near(iso->GetSpacing()[2], xy, 1e-12));
    assert(SizeOf(iso)[2] > SizeOf(out)[2]); // z voxels are finer, so more of them
  }

  // --- the valid region predicted for a deskewed volume matches what Deskew() produces
  for (double deskew_angle : {147.55, 31.8})
  {
    const double step = 0.5, pix = 0.108;
    Image::Pointer raw = MakeImage(60, 3, 40, pix, step, 1.0);
    Image::Pointer deskewed = Deskew(raw, deskew_angle, step, pix, 0.0);
    const double shift = llsm_rotate::DeskewShiftPixels(step, deskew_angle, pix);
    const auto size = SizeOf(deskewed);
    const auto poly = llsm_rotate::ValidPolygon(size[0], size[2], pix, 1.0, shift); // unit z spacing -> z in planes
    for (std::size_t z = 0; z < size[2]; ++z)
    {
      const auto ext = llsm_rotate::HorizontalExtent(poly, static_cast<double>(z));
      long first = -1, last = -1;
      for (std::size_t x = 0; x < size[0]; ++x)
      {
        Image::IndexType j;
        j[0] = x;
        j[1] = 1;
        j[2] = z;
        if (deskewed->GetPixel(j) > 0.5)
        {
          if (first < 0) first = x;
          last = x;
        }
      }
      assert(first >= 0);
      assert(std::labs(first - std::lround(ext[0] / pix)) <= 1);
      assert(std::labs(last - std::lround(ext[1] / pix)) <= 1);
    }
  }

  // --- invalid parameters are rejected
  {
    Image::Pointer img = MakeImage(10, 2, 10, xy, zs);
    assert(Throws([&] { Rot(img, 10.0, 0.0, zs); }));
    assert(Throws([&] { Rot(img, 10.0, xy, -1.0); }));
    assert(Throws([&] { Rot(img, 10.0, xy, zs, CropMode::Strict); })); // no deskew shift
  }

  std::cout << "rotate ITK tests passed" << std::endl;
  return 0;
}
