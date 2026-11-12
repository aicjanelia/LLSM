#pragma once

// Pure geometry for the rotate module (no ITK dependency, so it can be unit
// tested on its own).
//
// Convention: rotating by theta (degrees) matches BigStitcher's "Rotation around
// axis -> y-axis" applied to calibrated (physical) coordinates:
//
//     [ cos(t)  0  sin(t) ]
//     [   0     1    0    ]     applied to the content (x, y, z)
//     [-sin(t)  0  cos(t) ]
//
// so x' = x cos(t) + z sin(t) and z' = -x sin(t) + z cos(t). MOSAIC data use
// t = -32.45. Coordinates are physical (um) with the pivot at input voxel (0,0,0)
// and voxel centres at index * spacing. The y axis is the rotation axis, so the
// output y grid is the input y grid.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace llsm_rotate
{

enum class CropMode
{
  KeepAll, // bounding box of the valid-data parallelogram
  Strict   // largest full-height rectangle containing only valid data
};

struct Point2
{
  double x;
  double z;
};

// Corners in order: row z=0 start, row z=0 end, row z=nz-1 end, row z=nz-1 start.
// Edges (0,3) and (1,2) are the "scan edges" that follow the stage motion.
using Polygon = std::array<Point2, 4>;

struct OutputGrid
{
  std::array<long, 3> first;      // index of the first output voxel on each axis
  std::array<std::size_t, 3> size; // number of output voxels on each axis
  std::array<double, 3> origin;   // physical position of the first voxel centre
  std::array<double, 3> spacing;  // output spacing (um)
};

constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
constexpr double kSnapTolerance = 1e-6;       // in output voxels, absorbs floating point noise
constexpr double kScanEdgeTolerance = 1.75e-3; // ~0.1 degrees: |dz| / length of a "horizontal" edge

inline std::array<double, 9> RotationMatrix(double theta_degrees)
{
  const double t = theta_degrees * kDegToRad;
  const double c = std::cos(t);
  const double s = std::sin(t);
  return {c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c};
}

// Signed deskew shift in pixels per z plane, as computed by Deskew().
inline double DeskewShiftPixels(double step_um, double deskew_angle_degrees, double xy_um)
{
  return step_um * std::cos(deskew_angle_degrees * kDegToRad) / xy_um;
}

// Valid-data parallelogram of a deskewed volume, in physical (x, z) coordinates.
// shift == 0 gives the full array. Extents use voxel centres (nx-1, nz-1).
inline Polygon ValidPolygon(std::size_t nx, std::size_t nz, double xy, double z_spacing, double shift)
{
  if (nx < 1 || nz < 1)
    throw std::invalid_argument("image dimensions must be positive");
  if (!(xy > 0.0) || !(z_spacing > 0.0))
    throw std::invalid_argument("spacing must be positive");

  const double rows = static_cast<double>(nz - 1);
  const double abs_shift = std::fabs(shift);
  const double width = static_cast<double>(nx - 1) - abs_shift * rows; // valid voxel-centre span per row
  if (width < 0.0)
    throw std::invalid_argument("shift is too large for the image width");

  // x start of row z (in pixels), see Deskew(): shift>0 -> shift*z, shift<0 -> |shift|*(nz-1-z)
  const auto start = [&](double z) { return shift >= 0.0 ? shift * z : abs_shift * (rows - z); };

  const double zmax = rows * z_spacing;
  return {Point2{start(0.0) * xy, 0.0},
          Point2{(start(0.0) + width) * xy, 0.0},
          Point2{(start(rows) + width) * xy, zmax},
          Point2{start(rows) * xy, zmax}};
}

inline Polygon RotatePolygon(const Polygon &poly, double theta_degrees)
{
  const auto m = RotationMatrix(theta_degrees);
  Polygon out = poly;
  for (auto &p : out)
  {
    const double x = p.x, z = p.z;
    p.x = m[0] * x + m[2] * z;
    p.z = m[6] * x + m[8] * z;
  }
  return out;
}

// x extent of a convex polygon along the horizontal line at height z.
inline std::array<double, 2> HorizontalExtent(const Polygon &poly, double z)
{
  double lo = std::numeric_limits<double>::max(), hi = std::numeric_limits<double>::lowest();
  const double eps = 1e-9;
  for (std::size_t i = 0; i < poly.size(); ++i)
  {
    const Point2 &a = poly[i];
    const Point2 &b = poly[(i + 1) % poly.size()];
    if (std::fabs(a.z - z) <= eps)
    {
      lo = std::min(lo, a.x);
      hi = std::max(hi, a.x);
    }
    const double z0 = std::min(a.z, b.z), z1 = std::max(a.z, b.z);
    if (z1 - z0 > eps && z >= z0 - eps && z <= z1 + eps)
    {
      const double f = (z - a.z) / (b.z - a.z);
      const double x = a.x + f * (b.x - a.x);
      lo = std::min(lo, x);
      hi = std::max(hi, x);
    }
  }
  return {lo, hi};
}

inline long SnapDown(double value, double spacing) { return static_cast<long>(std::floor(value / spacing + kSnapTolerance)); }
inline long SnapUp(double value, double spacing) { return static_cast<long>(std::ceil(value / spacing - kSnapTolerance)); }

// Output grid for the rotated volume. Grid voxel centres sit at integer multiples of
// the output spacing (BigStitcher's frame). The box depends only on the arguments,
// never on pixel content, so every channel/timepoint gets the same grid.
//   spacing_in : input (x, y, z) spacing in um; x and y must match
//   shift      : signed deskew shift (px per z plane); 0 treats the array as fully valid
//   isotropic  : output voxels are xy x xy x xy instead of preserving the input spacing
inline OutputGrid ComputeOutputGrid(const std::array<std::size_t, 3> &size_in,
                                    const std::array<double, 3> &spacing_in,
                                    double theta_degrees,
                                    double shift,
                                    CropMode mode,
                                    bool isotropic)
{
  if (std::fabs(spacing_in[0] - spacing_in[1]) > 1e-9 * spacing_in[0])
    throw std::invalid_argument("rotate requires equal x and y spacing");

  const double sx = spacing_in[0];
  const double sz = isotropic ? spacing_in[0] : spacing_in[2];

  const Polygon valid = ValidPolygon(size_in[0], size_in[2], spacing_in[0], spacing_in[2], shift);
  const Polygon rot = RotatePolygon(valid, theta_degrees);

  double xmin = std::numeric_limits<double>::max(), xmax = std::numeric_limits<double>::lowest();
  double zmin = xmin, zmax = xmax;
  for (const auto &p : rot)
  {
    xmin = std::min(xmin, p.x);
    xmax = std::max(xmax, p.x);
    zmin = std::min(zmin, p.z);
    zmax = std::max(zmax, p.z);
  }

  long x0, x1, z0, z1;
  if (mode == CropMode::KeepAll)
  {
    x0 = SnapDown(xmin, sx);
    x1 = SnapUp(xmax, sx);
    z0 = SnapDown(zmin, sz);
    z1 = SnapUp(zmax, sz);
  }
  else
  {
    if (shift == 0.0)
      throw std::invalid_argument("strict cropping needs the deskew shift (it describes the empty deskew regions); "
                                  "provide it or use keep-all");
    // The scan edges (0,3) and (1,2) must be horizontal after rotation, i.e. theta
    // matches the deskew geometry; otherwise there is no full-height valid rectangle.
    const auto horizontal = [&](const Point2 &a, const Point2 &b)
    {
      const double len = std::hypot(b.x - a.x, b.z - a.z);
      return len > 0.0 && std::fabs(b.z - a.z) <= kScanEdgeTolerance * len;
    };
    if (!horizontal(rot[0], rot[3]) || !horizontal(rot[1], rot[2]))
    {
      std::ostringstream msg;
      msg << "strict cropping requires the rotation angle to match the deskew geometry (scan edges are not horizontal after rotating by "
          << theta_degrees << " degrees); use keep-all instead";
      throw std::invalid_argument(msg.str());
    }
    // bottom edge = the scan edge with the smaller mean z
    const double mean_a = 0.5 * (rot[0].z + rot[3].z);
    const double mean_b = 0.5 * (rot[1].z + rot[2].z);
    const bool a_bottom = mean_a <= mean_b;
    const Point2 &b0 = a_bottom ? rot[0] : rot[1];
    const Point2 &b1 = a_bottom ? rot[3] : rot[2];
    const Point2 &t0 = a_bottom ? rot[1] : rot[0];
    const Point2 &t1 = a_bottom ? rot[2] : rot[3];
    const double zlo = std::max(b0.z, b1.z);
    const double zhi = std::min(t0.z, t1.z);
    if (!(zhi >= zlo))
      throw std::invalid_argument("strict cropping found no fully valid region");

    const auto lo_ext = HorizontalExtent(rot, zlo);
    const auto hi_ext = HorizontalExtent(rot, zhi);
    const double rect_xmin = std::max(lo_ext[0], hi_ext[0]);
    const double rect_xmax = std::min(lo_ext[1], hi_ext[1]);
    if (!(rect_xmax >= rect_xmin))
      throw std::invalid_argument("strict cropping found no fully valid region");

    // snap inward so no output voxel centre lies outside the valid data
    x0 = SnapUp(rect_xmin, sx);
    x1 = SnapDown(rect_xmax, sx);
    z0 = SnapUp(zlo, sz);
    z1 = SnapDown(zhi, sz);
    if (x1 < x0 || z1 < z0)
      throw std::invalid_argument("strict cropping leaves no output voxels");
  }

  OutputGrid grid;
  grid.spacing = {sx, spacing_in[1], sz};
  grid.first = {x0, 0, z0};
  grid.size = {static_cast<std::size_t>(x1 - x0 + 1), size_in[1], static_cast<std::size_t>(z1 - z0 + 1)};
  grid.origin = {x0 * sx, 0.0, z0 * sz};
  return grid;
}

} // namespace llsm_rotate
