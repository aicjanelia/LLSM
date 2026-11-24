// Geometry tests for the rotate module (no ITK needed).
#include "rotate_geometry.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace llsm_rotate;

static bool Near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

template <class F>
static bool Throws(F f)
{
  try { f(); } catch (const std::invalid_argument &) { return true; }
  return false;
}

int main()
{
  // --- Rotation matrix equals the transform BigStitcher logged for -32.45 degrees
  {
    const auto m = RotationMatrix(-32.45);
    assert(Near(m[0], 0.8438600069755025, 1e-12));
    assert(Near(m[2], -0.5365634059710976, 1e-12));
    assert(Near(m[6], 0.5365634059710976, 1e-12));
    assert(Near(m[8], 0.8438600069755025, 1e-12));
    assert(m[4] == 1.0 && m[1] == 0.0 && m[3] == 0.0 && m[5] == 0.0 && m[7] == 0.0);
  }

  // --- MOSAIC example (ExampleBigStitcherOutput): 2075 x 1500 x 401, 0.108 um xy, stage step 0.5,
  //     deskew angle 147.55 (negative shift), rotation -32.45.
  const std::array<std::size_t, 3> size = {2075, 1500, 401};
  const double xy = 0.108, step = 0.5;
  const double shift = DeskewShiftPixels(step, 147.55, xy);
  const double zsp = std::fabs(step * std::sin(147.55 * kDegToRad));
  assert(shift < 0.0 && Near(shift, -3.9068, 1e-3));
  assert(Near(zsp, 0.26828170298, 1e-9));
  const std::array<double, 3> spacing = {xy, xy, zsp};

  const auto strict = ComputeOutputGrid(size, spacing, -32.45, shift, CropMode::Strict, spacing);
  const auto keep = ComputeOutputGrid(size, spacing, -32.45, shift, CropMode::KeepAll, spacing);

  // BigStitcher log: box x -101..1319 (1421 voxels), z 337..449 (113 voxels) in output units.
  // Ours uses exact voxel-centre extents and snaps inward, so allow a couple of voxels.
  std::cout << "strict: first x " << strict.first[0] << " size " << strict.size[0] << " x " << strict.size[1]
            << " x " << strict.size[2] << " z first " << strict.first[2] << "\n";
  std::cout << "keep-all: first x " << keep.first[0] << " size " << keep.size[0] << " x " << keep.size[1]
            << " x " << keep.size[2] << " z first " << keep.first[2] << "\n";
  assert(std::labs(strict.first[0] - (-101)) <= 2);
  assert(std::labs(static_cast<long>(strict.size[0]) - 1421) <= 2);
  // z: BigStitcher snaps outward (337..449), strict here snaps inward so no output voxel lies
  // outside the valid data; that costs up to one voxel at each end, plus the exact-extent difference.
  assert(std::labs(static_cast<long>(strict.size[2]) - 113) <= 3);
  assert(strict.size[2] <= 113);
  assert(strict.size[1] == 1500 && keep.size[1] == 1500);
  // macro: keep-all width ~2284 xy px, same z extent as strict
  assert(std::labs(static_cast<long>(keep.size[0]) - 2285) <= 3);
  assert(std::labs(static_cast<long>(keep.size[2]) - static_cast<long>(strict.size[2])) <= 2);
  // keep-all contains strict
  assert(keep.first[0] <= strict.first[0]);
  assert(keep.first[0] + static_cast<long>(keep.size[0]) >= strict.first[0] + static_cast<long>(strict.size[0]));
  assert(keep.first[2] <= strict.first[2]);

  // Output preserves the input spacing and sits on integer multiples of it
  assert(strict.spacing[0] == xy && strict.spacing[1] == xy && strict.spacing[2] == zsp);
  assert(Near(strict.origin[0], strict.first[0] * xy, 1e-12));
  assert(Near(strict.origin[2], strict.first[2] * zsp, 1e-12));

  // --- Isotropic output: z voxels become xy-sized, so there are more of them
  const auto iso = ComputeOutputGrid(size, spacing, -32.45, shift, CropMode::Strict, std::array<double, 3>{xy, xy, xy});
  assert(iso.spacing[2] == xy);
  assert(std::labs(static_cast<long>(iso.size[2]) - static_cast<long>(strict.size[2] * zsp / xy)) <= 3);
  assert(iso.size[0] == strict.size[0]);
  assert(iso.size[1] == size[1]); // y spacing equals the input, so y is unchanged

  // --- User spacing: independent x, y, z; y is resampled over the same physical extent
  {
    const std::array<double, 3> user = {0.2, 0.3, 0.5};
    const auto g = ComputeOutputGrid(size, spacing, -32.45, shift, CropMode::Strict, user);
    assert(g.spacing[0] == 0.2 && g.spacing[1] == 0.3 && g.spacing[2] == 0.5);
    assert(Near(g.origin[0], g.first[0] * 0.2, 1e-12) && Near(g.origin[2], g.first[2] * 0.5, 1e-12));
    assert(g.origin[1] == 0.0 && g.first[1] == 0);
    const double y_extent = (size[1] - 1) * xy;
    assert(g.size[1] == static_cast<std::size_t>(std::floor(y_extent / 0.3)) + 1);
    assert(static_cast<double>(g.size[1] - 1) * 0.3 <= y_extent + 1e-9);
    // coarser x/z than the input gives fewer voxels than preserving the input spacing
    assert(g.size[0] < strict.size[0] && g.size[2] < strict.size[2]);
    // finer y than the input gives more voxels
    const auto fine_y = ComputeOutputGrid(size, spacing, -32.45, shift, CropMode::Strict, {xy, xy / 2, zsp});
    assert(fine_y.size[1] == 2 * (size[1] - 1) + 1);
  }

  // --- Non-positive or non-finite output spacing is rejected
  assert(Throws([&] { ComputeOutputGrid(size, spacing, -32.45, shift, CropMode::Strict, {xy, 0.0, zsp}); }));
  assert(Throws([&] { ComputeOutputGrid(size, spacing, -32.45, shift, CropMode::Strict, {xy, xy, -zsp}); }));
  assert(Throws([&] { ComputeOutputGrid(size, spacing, -32.45, shift, CropMode::Strict, {1.0e9, xy, zsp}); }));

  // --- Strict requires the angle to match the deskew geometry
  assert(Throws([&] { ComputeOutputGrid(size, spacing, -20.0, shift, CropMode::Strict, spacing); }));
  assert(Throws([&] { ComputeOutputGrid(size, spacing, 32.45, shift, CropMode::Strict, spacing); }));
  // ...and a deskew shift must be supplied at all (the array alone says nothing about empty regions)
  assert(Throws([&] { ComputeOutputGrid(size, spacing, 0.0, 0.0, CropMode::Strict, spacing); }));
  assert(Throws([&] { ComputeOutputGrid(size, spacing, 10.0, 0.0, CropMode::Strict, spacing); }));
  // ...but keep-all works for any angle
  ComputeOutputGrid(size, spacing, -20.0, shift, CropMode::KeepAll, spacing);
  ComputeOutputGrid(size, spacing, 77.0, shift, CropMode::KeepAll, spacing);

  // --- LLSM-style geometry: positive shift (deskew angle 31.8) matches rotation +31.8
  {
    const double llsm_xy = 0.104, llsm_step = 0.4;
    const double llsm_shift = DeskewShiftPixels(llsm_step, 31.8, llsm_xy);
    const double llsm_z = llsm_step * std::sin(31.8 * kDegToRad);
    assert(llsm_shift > 0.0);
    // strict needs scan depth (nz * step / xy) to exceed the raw width's projection (W * cos)
    const std::array<std::size_t, 3> llsm_size = {1800, 300, 300};
    const std::array<double, 3> llsm_spacing = {llsm_xy, llsm_xy, llsm_z};
    ComputeOutputGrid(llsm_size, llsm_spacing, 31.8, llsm_shift, CropMode::Strict, llsm_spacing);
    assert(Throws([&] { ComputeOutputGrid(llsm_size, llsm_spacing, -31.8, llsm_shift, CropMode::Strict, llsm_spacing); }));
  }

  // --- Degenerate/identity cases (no deskew shift)
  {
    const std::array<std::size_t, 3> s = {64, 8, 32};
    const std::array<double, 3> sp = {0.1, 0.1, 0.3};
    const auto id = ComputeOutputGrid(s, sp, 0.0, 0.0, CropMode::KeepAll, sp);
    assert(id.first[0] == 0 && id.first[2] == 0);
    assert(id.size[0] == 64 && id.size[1] == 8 && id.size[2] == 32);
    const auto full = ComputeOutputGrid(s, sp, 360.0, 0.0, CropMode::KeepAll, sp);
    assert(full.first == id.first && full.size == id.size);
    const auto half = ComputeOutputGrid(s, sp, 180.0, 0.0, CropMode::KeepAll, sp);
    assert(half.first[0] == -63 && half.size[0] == 64 && half.first[2] == -31 && half.size[2] == 32);
  }

  // --- z-trim removes output slices from the ends after the crop mode is applied
  {
    for (const auto mode : {CropMode::KeepAll, CropMode::Strict})
    {
      const auto base = ComputeOutputGrid(size, spacing, -32.45, shift, mode, spacing);
      const auto sym = ComputeOutputGrid(size, spacing, -32.45, shift, mode, spacing, {3, 3});
      assert(sym.size[2] == base.size[2] - 6 && sym.first[2] == base.first[2] + 3);
      assert(Near(sym.origin[2], sym.first[2] * sym.spacing[2], 1e-12));
      assert(sym.size[0] == base.size[0] && sym.first[0] == base.first[0] && sym.size[1] == base.size[1]);
      const auto asym = ComputeOutputGrid(size, spacing, -32.45, shift, mode, spacing, {0, 5});
      assert(asym.size[2] == base.size[2] - 5 && asym.first[2] == base.first[2]);
      const auto zero = ComputeOutputGrid(size, spacing, -32.45, shift, mode, spacing, {0, 0});
      assert(zero.size == base.size && zero.first == base.first);
      const long n = static_cast<long>(base.size[2]);
      assert(Throws([&] { ComputeOutputGrid(size, spacing, -32.45, shift, mode, spacing, {n, 0}); }));
      assert(Throws([&] { ComputeOutputGrid(size, spacing, -32.45, shift, mode, spacing, {n / 2, n - n / 2}); }));
      assert(Throws([&] { ComputeOutputGrid(size, spacing, -32.45, shift, mode, spacing, {-1, 0}); }));
      ComputeOutputGrid(size, spacing, -32.45, shift, mode, spacing, {n / 2, n - n / 2 - 1}); // leaves one slice
    }
  }

  // --- Input validation
  assert(Throws([&] { ComputeOutputGrid(size, {0.1, 0.2, 0.3}, 10.0, 0.0, CropMode::KeepAll, {0.1, 0.2, 0.3}); }));
  assert(Throws([&] { ValidPolygon(100, 10, 0.1, 0.0, 0.0); }));
  assert(Throws([&] { ValidPolygon(100, 10, -0.1, 0.3, 0.0); }));
  assert(Throws([&] { ValidPolygon(10, 100, 0.1, 0.3, 5.0); })); // shift swallows the width

  std::cout << "rotate geometry tests passed" << std::endl;
  return 0;
}
