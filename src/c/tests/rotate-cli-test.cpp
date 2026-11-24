// End-to-end tests for the rotate binary: write a synthetic TIFF, run `rotate`, read the result back.
// ROTATE_EXECUTABLE is defined by CMake as the path of the built binary.
#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "writer.h"

#include <itkImageRegionIteratorWithIndex.h>

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#ifndef _WIN32
#include <sys/wait.h>
#endif

#ifndef ROTATE_EXECUTABLE
#error "ROTATE_EXECUTABLE must be defined"
#endif

namespace
{

int Run(const std::string &args)
{
  const std::string cmd = std::string("\"") + ROTATE_EXECUTABLE + "\" " + args + " > /dev/null 2>&1";
  const int status = std::system(cmd.c_str());
#ifndef _WIN32
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#else
  return status;
#endif
}

constexpr double kPi = 3.14159265358979323846;

bool Near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

void WritePhantom(const std::string &path, double xy, double z)
{
  kImageType::Pointer img = kImageType::New();
  kImageType::SizeType size;
  size[0] = 80;
  size[1] = 4;
  size[2] = 60;
  kImageType::RegionType region;
  region.SetSize(size);
  img->SetRegions(region);
  kImageType::SpacingType spacing;
  spacing[0] = xy;
  spacing[1] = xy;
  spacing[2] = z;
  img->SetSpacing(spacing);
  img->Allocate();
  itk::ImageRegionIteratorWithIndex<kImageType> it(img, region);
  for (it.GoToBegin(); !it.IsAtEnd(); ++it)
    it.Set(0.25 + 0.5 * (it.GetIndex()[0] % 7) / 6.0); // textured, within [0,1]
  using Out = itk::Image<unsigned short, kDimensions>;
  WriteImageFile<kImageType, Out>(img, path, false, false);
}

} // namespace

int main()
{
  const double xy = 0.108, z = 0.26828;
  const fs::path dir = fs::temp_directory_path() / fs::unique_path("rotate-cli-%%%%-%%%%");
  fs::create_directories(dir);
  const std::string in = (dir / "in.tif").string();
  const std::string out = (dir / "out.tif").string();
  const std::string common = " -x 0.108 -z 0.26828 ";
  WritePhantom(in, xy, z);

  // --- successful run (preserving spacing): larger output, spacing read back from the file
  assert(Run("-a -32.45" + common + "--output-spacing input -o \"" + out + "\" \"" + in + "\"") == 0);
  {
    TiffStackBlockReader reader(out);
    assert(reader.GetHeight() == 4);
    assert(reader.GetWidth() > 80);
    assert(Near(reader.GetSpacing()[0], xy, 1e-4));
    assert(Near(reader.GetSpacing()[2], z, 1e-5));
  }

  // --- existing output is protected unless -w is given
  assert(Run("-a -32.45" + common + "-o \"" + out + "\" \"" + in + "\"") != 0);
  assert(Run("-a -32.45" + common + "-w -o \"" + out + "\" \"" + in + "\"") == 0);

  // --- isotropic output (the default) has xy-sized z voxels
  const std::string iso = (dir / "iso.tif").string();
  assert(Run("-a -32.45" + common + "-o \"" + iso + "\" \"" + in + "\"") == 0);
  {
    TiffStackBlockReader reader(iso);
    assert(Near(reader.GetSpacing()[2], xy, 1e-4));
  }

  // --- user output spacing: a single value (cubic) or x,y,z
  {
    const std::string cubic = (dir / "cubic.tif").string();
    assert(Run("-a -32.45" + common + "--output-spacing 0.2 -o \"" + cubic + "\" \"" + in + "\"") == 0);
    TiffStackBlockReader reader(cubic);
    assert(Near(reader.GetSpacing()[0], 0.2, 1e-4) && Near(reader.GetSpacing()[2], 0.2, 1e-4));
    const std::string aniso = (dir / "aniso.tif").string();
    assert(Run("-a -32.45" + common + "--output-spacing 0.2,0.1,0.3 -o \"" + aniso + "\" \"" + in + "\"") == 0);
    TiffStackBlockReader reader2(aniso);
    assert(Near(reader2.GetSpacing()[0], 0.2, 1e-4) && Near(reader2.GetSpacing()[1], 0.1, 1e-4) &&
           Near(reader2.GetSpacing()[2], 0.3, 1e-4));
  }

  // --- other bit depths
  for (const char *bits : {"8", "32"})
  {
    const std::string o = (dir / (std::string("b") + bits + ".tif")).string();
    assert(Run(std::string("-a 20") + common + "-b " + bits + " -o \"" + o + "\" \"" + in + "\"") == 0);
    assert(IsFile(o.c_str()));
  }

  // --- argument validation
  const std::string bad = (dir / "bad.tif").string();
  assert(Run("-a 20" + common + "-o \"" + bad + "\" \"" + (dir / "missing.tif").string() + "\"") != 0); // missing input
  assert(Run("-a 20" + common + "\"" + in + "\"") != 0);                                                // missing output
  assert(Run(common + "-o \"" + bad + "\" \"" + in + "\"") != 0);                                       // missing angle
  assert(Run("-a 20" + common + "-b 12 -o \"" + bad + "\" \"" + in + "\"") != 0);                        // bad bit depth
  assert(Run("-a 400" + common + "-o \"" + bad + "\" \"" + in + "\"") != 0);                             // angle out of range
  assert(Run("-a 20" + common + "-c sideways -o \"" + bad + "\" \"" + in + "\"") != 0);                  // bad crop mode
  for (const char *spacing : {"0", "-0.2", "abc", "0.2,0.2", "0.2,0.2,0.2,0.2", "0.2,,0.2", "nan", "inf"})
    assert(Run(std::string("-a 20") + common + "--output-spacing " + spacing + " -o \"" + bad + "\" \"" + in + "\"") != 0); // bad spacing
  assert(Run("-a 20" + common + "-c strict -o \"" + bad + "\" \"" + in + "\"") != 0);                    // strict needs --shift
  assert(!IsFile(bad.c_str()));
  assert(Run("--version") != 0 && Run("--help") != 0); // same convention as the other modules (EXIT_FAILURE)

  // --- strict works with a matching shift: MOSAIC-like geometry (shift < 0, angle -32.45)
  {
    const double step = 0.5;
    const double shift = step * std::cos(147.55 * kPi / 180.0) / xy;
    const double zsp = step * std::sin(147.55 * kPi / 180.0);
    // a phantom wide and deep enough for a fully valid rectangle to exist
    kImageType::Pointer img = kImageType::New();
    kImageType::SizeType size;
    size[0] = 700;
    size[1] = 2;
    size[2] = 100;
    kImageType::RegionType region;
    region.SetSize(size);
    img->SetRegions(region);
    img->Allocate();
    img->FillBuffer(0.5);
    const std::string deskewed = (dir / "deskewed.tif").string();
    WriteImageFile<kImageType, itk::Image<unsigned short, kDimensions>>(img, deskewed, false, false);
    const std::string strict_out = (dir / "strict.tif").string();
    const std::string args = "-a -32.45 -x 0.108 -z " + std::to_string(zsp) + " -d " + std::to_string(shift);
    assert(Run(args + " -c strict -o \"" + strict_out + "\" \"" + deskewed + "\"") == 0);
    // ...and refuses a mismatched angle
    assert(Run("-a -20 -x 0.108 -z " + std::to_string(zsp) + " -d " + std::to_string(shift) + " -c strict -o \"" +
               (dir / "mismatch.tif").string() + "\" \"" + deskewed + "\"") != 0);
  }

  fs::remove_all(dir);
  std::cout << "rotate CLI tests passed" << std::endl;
  return 0;
}
