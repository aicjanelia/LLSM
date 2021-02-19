#include "linearDecon.h"
#include "klb_Cwrapper.h"
#include <exception>

#include <time.h>
#include <string.h>



void writeKLBversion(std::string inputFileName, CImg<unsigned short> &uint16Img, std::string dirName, std::string fileSuffix)
{
	makeResultsDir(dirName);
	std::string fName = makeOutputFilePath(inputFileName, dirName, fileSuffix);
	uint32_t dims[5] = { uint16Img.width(),uint16Img.height(),uint16Img.depth(),1,1 };
	float32_t pixelSize[5] = { 1,1,1,1,1 };
	uint32_t blockSize[5] = { 64,64,8,1,1 };
	fName = fName.substr(0, fName.size() - 3);
	fName += "klb";
	writeKLBstack(uint16Img.data(), fName.c_str(), dims, KLB_DATA_TYPE::UINT16_TYPE, 1, pixelSize, blockSize, KLB_COMPRESSION_TYPE::BZIP2, "");
}


int main(int argc, char *argv[])
{
  int napodize, nZblend;
  float background;
  float NA=1.2;
  ImgParams imgParams;
  float dz_psf, dr_psf;

  int RL_iters=0;
  int nthreads = 4;
  bool bSaveDeskewedRaw = false;
  float deskewAngle=32.8;
  bool bDeskew = false;
  bool bRotate = false;
  unsigned outputWidth;
  int extraShift=0;
  std::vector<int> final_CropTo_boundaries;
  bool bSaveUshort = false;
  std::vector<bool> bDoMaxIntProj;
  std::vector< CImg<> > MIprojections;

  TIFFSetWarningHandler(NULL);

  time_t tic = time(NULL);
  std::string inputFileName, psffiles;
  po::options_description progopts;
  progopts.add_options()
    ("drdata", po::value<float>(&imgParams.dr)->default_value(.104), "image x-y pixel size (um)")
    ("dzdata,z", po::value<float>(&imgParams.dz)->default_value(.25), "image z step (um)")
    ("drpsf", po::value<float>(&dr_psf)->default_value(.104), "PSF x-y pixel size (um)")
    ("dzpsf,Z", po::value<float>(&dz_psf)->default_value(.1), "PSF z step (um)")
    // ("wavelength,l", po::value<float>(&imgParams.wave)->default_value(.525), "emission wavelength (um)")
    ("background,b", po::value<float>(&background)->default_value(80.f), "user-supplied background")
    ("napodize,e", po::value<int>(&napodize)->default_value(15), "number of pixels to soften edge with")
    ("nzblend,E", po::value<int>(&nZblend)->default_value(0), "number of top and bottom sections to blend in to reduce axial ringing")
    ("NA,n", po::value<float>(&NA)->default_value(1.2), "numerical aperture")
    ("RL,i", po::value<int>(&RL_iters)->default_value(15), "run Richardson-Lucy how-many iterations")
    ("deskew,D", po::bool_switch(&bDeskew)->default_value(false), "To perform deskewing before deconv")
    ("angle,a", po::value<float>(&deskewAngle)->default_value(32.8), "The deskewing angle")
    ("rotate,R", po::bool_switch(&bRotate)->default_value(false), "To perform rotation around y axis after deconv by deskewAngle")
    ("width,w", po::value<unsigned>(&outputWidth)->default_value(0), "If deskewed, the output image's width")
    ("shift,x", po::value<int>(&extraShift)->default_value(0), "If deskewed, the output image's extra shift in X (positive->left")
    ("saveDeskewedRaw,S", po::bool_switch(&bSaveDeskewedRaw)->default_value(false), "save deskewed raw data to files")
    ("crop,C", fixed_tokens_value< std::vector<int> >(&final_CropTo_boundaries, 6, 6), "takes 6 integers separated by space: x1 x2 y1 y2 z1 z2; crop final image size to [x1:x2, y1:y2, z1:z2]")
    ("MIP,M", fixed_tokens_value< std::vector<bool> >(&bDoMaxIntProj, 3, 3), "takes 3 binary numbers separated by space to indicate whether save a max-intensity projection along x, y, or z axis")
    ("uint16,u", po::bool_switch(&bSaveUshort)->implicit_value(true), "whether to save result in uint16 format; this should be used only if no actual decon is performed")
    // ("ncores,T", po::value<int>(&nthreads)->default_value(4), "number of cores to use")
    ("input-file", po::value<std::string>(&inputFileName)->required(), "input file name")
    ("psf-file", po::value<std::string>(&psffiles)->required(), "PSF file")
    ("help,h", "produce help message")
    ;
  po::positional_options_description p;
  p.add("input-file", 1);
  p.add("psf-file", 1);

// Parse commandline option:
  po::variables_map varsmap;
  try {
    store(po::command_line_parser(argc, argv).
          options(progopts).positional(p).run(), varsmap);
    if (varsmap.count("help")) {
      std::cout << progopts << "\n";
      return 0;
    }

    notify(varsmap);

    // if (varsmap.count("crop")) {
    //   if (final_CropTo_boundaries.size() != 6)
    //     throw std::runtime_error("Exactly 6 integers are required for the -C or --crop flag!");
    //   std::copy(final_CropTo_boundaries.begin(), final_CropTo_boundaries.end(), std::ostream_iterator<int>(std::cout, ", "));
    //   std::cout << std::endl;
    // }
    // std::cout << bDoMaxIntProj.size() << std::endl;
  }
  catch (std::exception &e) {
    std::cout << "\n!!Error occurred: " << e.what() << std::endl;
    return 0;
  }

  // Obtain the data folder:
  getDataDir(inputFileName);
  
  // Set the maximum number of threads to use in OpenMP
#if defined (__linux__) || defined(TARGET_OS_MAC)
  char* nslots = getenv("NSLOTS");
  if ( !nslots )
    nthreads = sysconf(_SC_NPROCESSORS_CONF)/2; // Get number of processors; use half of them
  else
    nthreads = atoi(nslots);
#endif
#ifndef __clang__
  omp_set_num_threads(nthreads);
#endif
  std::cout << nthreads << " threads used\n";

  double deskewFactor=0;
  bool bCrop = false; // to indicate if cropping of raw images needs to happen before deconvolution
  unsigned new_ny, new_nz, new_nx;
  int deskewedXdim = 0;

  try {
    CImg<> raw_image, raw_deskewed;
    std::cout<< inputFileName << std::endl;
    raw_image.assign(inputFileName.c_str());

    // Initialize a bunch including:
    // 1. crop image to make dimensions nice factorizable numbers
    // 2. calculate deskew parameters, new X dimensions
    // 3. calculate rotation matrix
    // 4. create FFT plans
    // 5. transfer constants into GPU device constant memory
    // if (it == all_matching_files.begin()) {
    unsigned nx = raw_image.width();
    unsigned ny = raw_image.height();
    unsigned nz = raw_image.depth();

    printf("Original image size: nz=%d, ny=%d, nx=%d\n", nz, ny, nx);

    if (RL_iters>0) {
      new_ny = findOptimalDimension(ny);
      if (new_ny != ny) {
        printf("new ny=%d\n", new_ny);
        bCrop = true;
      }

      new_nz = findOptimalDimension(nz);
      if (new_nz != nz) {
        printf("new nz=%d\n", new_nz);
        bCrop = true;
      }

      // only if no deskewing is happening do we want to change image width here
      new_nx = nx;
      if ( !bDeskew || !(fabs(deskewAngle) > 0.0) ) {
        new_nx = findOptimalDimension(nx);
        if (new_nx != nx) {
          printf("new nx=%d\n", new_nx);
          bCrop = true;
        }
      }
    }
    else {
      new_nx = nx;
      new_ny = ny;
      new_nz = nz;
    }

    fftwf_plan_with_nthreads(nthreads);
    // Generate OTF (assuming 3D rotationally averaged OTF):
    CImg<> complexOTF = otfgen(psffiles, dr_psf, dz_psf, 7);
    // complexOTF.save_tiff("otf5.tif");
    // return 0;
    unsigned nr_otf = complexOTF.height();
    unsigned nz_otf = complexOTF.width() / 2; // "/2" because real and imaginary parts
    const float dkr_otf = 1/((nr_otf-1)*2 * dr_psf);
    const float dkz_otf = 1/(nz_otf * dz_psf);

    // Construct deskew matrix:
    deskewedXdim = new_nx;
    if (bDeskew && fabs(deskewAngle) > 0.0) {
      if (deskewAngle <0) deskewAngle += 180.;
      deskewFactor = cos(deskewAngle * M_PI/180.) * imgParams.dz / imgParams.dr;
      if (outputWidth ==0)
        deskewedXdim += floor(new_nz * imgParams.dz * 
                              fabs(cos(deskewAngle * M_PI/180.)) / imgParams.dr);
      // TODO: sometimes deskewedXdim calc'ed this way is too large
      else
        deskewedXdim = outputWidth; // use user-provided output width if available

      deskewedXdim = findOptimalDimension(deskewedXdim);

      // update z step size:
      imgParams.dz *= sin(deskewAngle * M_PI/180.);

      printf("deskewFactor=%f, new nx=%d\n", deskewFactor, deskewedXdim);

      raw_deskewed.assign(deskewedXdim, new_ny, new_nz);
      if (bSaveDeskewedRaw) {
        makeResultsDir("Deskewed");
      }
    }

    // Construct rotation matrix:
    CImg<> rotMatrix;
    if ( bRotate ) {
      rotMatrix.assign(4);
      float rotationAngle = deskewAngle * M_PI/180;
      float stretch = imgParams.dr / imgParams.dz;
      rotMatrix[0] = cos(rotationAngle) * stretch;
      rotMatrix[1] = sin(rotationAngle) * stretch;
      rotMatrix[2] = -sin(rotationAngle);
      rotMatrix[3] = cos(rotationAngle);
    }

    CImg<> raw_imageFFT((deskewedXdim/2+1)*2, new_ny, new_nz);

    // Create FFTW plans:
    if (!fftwf_init_threads()) { // one-time initialization required to use threads
      throw std::runtime_error("fftwf_init_threads() failed");             
    }

    const fftwf_plan rfftplan = fftwf_plan_dft_r2c_3d(new_nz, new_ny, deskewedXdim,
                                                      raw_image.data(),
                                                      (fftwf_complex *) raw_imageFFT.data(),
                                                      FFTW_ESTIMATE);
    const fftwf_plan rfftplan_inv = fftwf_plan_dft_c2r_3d(new_nz, new_ny, deskewedXdim,
                                                          (fftwf_complex *) raw_imageFFT.data(),
                                                          raw_image.data(),
                                                          FFTW_ESTIMATE);

    // Obtain a bunch of float constants:
    const float dkx = 1.0/(imgParams.dr * deskewedXdim);
    const float dky = 1.0/(imgParams.dr * new_ny);
    const float dkz = 1.0/(imgParams.dz * new_nz);
    // const float rdistcutoff = 2*NA/(imgParams.wave); // lateral resolution limit in 1/um

    if (bCrop) {
      raw_image.crop(0, 0, 0, 0, new_nx-1, new_ny-1, new_nz-1, 0);
      // If deskew is to happen, it'll be performed inside RichardsonLucy();
      // but here raw data's x dimension is still just "new_nx"
    }

    printf("background=%f\n", background);
    raw_image -= background;
	cimg_forXYZ(raw_image, x, y, z)
		raw_image(x, y, z) = raw_image(x, y, z) >= 0. ? raw_image(x, y, z) : 0.;

    if (RL_iters || raw_deskewed.size() || rotMatrix.size()) {
      RichardsonLucy(raw_image, raw_imageFFT, 
                     dkx/dkr_otf, dky/dkr_otf, dkz/dkz_otf,
                     complexOTF,
                     dkr_otf, dkz_otf,
                     RL_iters, deskewFactor,
                     deskewedXdim, extraShift,
                     napodize, nZblend, rotMatrix,
                     raw_deskewed,
                     rfftplan, rfftplan_inv);

      // raw_deskewed.display();
    }
    else {
      std::cerr << "Nothing is performed\n";
    }

    if (! final_CropTo_boundaries.empty()) {
      raw_image.crop(final_CropTo_boundaries[0], final_CropTo_boundaries[2],
                     final_CropTo_boundaries[4], 0,
                     final_CropTo_boundaries[1], final_CropTo_boundaries[3],
                     final_CropTo_boundaries[5], 0);
      if (raw_deskewed.size())
        raw_deskewed.crop(final_CropTo_boundaries[0], final_CropTo_boundaries[2],
                          final_CropTo_boundaries[4], 0,
                          final_CropTo_boundaries[1], final_CropTo_boundaries[3],
                          final_CropTo_boundaries[5], 0);
    }

    if (bSaveDeskewedRaw) {

#pragma omp parallel for
      for (auto ind = 0; ind<raw_deskewed.size(); ind++)
      {
		  // set negative values to zero
		  raw_deskewed[ind] = raw_deskewed[ind] >=0 ? raw_deskewed[ind] : 0;
	  }
      
	  // save out a tif version
	  CImg<unsigned short> uint16Img(raw_deskewed);
      uint16Img.save(makeOutputFilePath(inputFileName, "Deskewed", "_deskewed").c_str());

	  // save out a klb version
	  writeKLBversion(inputFileName, uint16Img,"DeskewedKLB","_deskewed");
    }

    makeResultsDir("CPPdecon");
    if (RL_iters || rotMatrix.size()) {
      if (! bSaveUshort)
        raw_image.save(makeOutputFilePath(inputFileName).c_str());
      else {
        CImg<unsigned short> uint16Img(raw_image);
        uint16Img.save(makeOutputFilePath(inputFileName).c_str());

		writeKLBversion(inputFileName, uint16Img, "CPPdeconKLB", "_decon");
      }
    }

    if (bDoMaxIntProj.size() == 3) {
      std::string MIPsubdir("CPPdecon/MIPs");
      makeResultsDir(MIPsubdir);
      if (bDoMaxIntProj[0]) {
        CImg<> proj = MaxIntProj(raw_image, 0);
             proj.save(makeOutputFilePath(inputFileName, MIPsubdir, "_MIP_x").c_str());
      }
      if (bDoMaxIntProj[1]) {
        CImg<> proj = MaxIntProj(raw_image, 1);
        proj.save(makeOutputFilePath(inputFileName, MIPsubdir, "_MIP_y").c_str());
      }
      if (bDoMaxIntProj[2]) {
        CImg<> proj = MaxIntProj(raw_image, 2);
        proj.save(makeOutputFilePath(inputFileName, MIPsubdir, "_MIP_z").c_str());
      }
    }
  } // try {} block
  catch (std::exception &e) {
    std::cerr << "\n!!Error occurred: " << e.what() << std::endl;
    return 0;
  }
  time_t toc=time(NULL);
  std::cout << "Elapsed seconds: " << toc-tic << std::endl;
  return 0; 
}


CImg<> MaxIntProj(CImg<> &input, int axis)
{
  CImg <> out;

  if (axis==0) {
    CImg<> maxvals(input.height(), input.depth());
    maxvals = -1e10;
#pragma omp parallel for
    cimg_forYZ(input, y, z) for (int x=0; x<input.width(); x++) {
      if (input(x, y, z) > maxvals(y, z))
        maxvals(y, z) = input(x, y, z);
    }
    return maxvals;
  }
  else if (axis==1) {
    CImg<> maxvals(input.width(), input.depth());
    maxvals = -1e10;
#pragma omp parallel for
    cimg_forXZ(input, x, z) for (int y=0; y<input.height(); y++) {
      if (input(x, y, z) > maxvals(x, z))
        maxvals(x, z) = input(x, y, z);
    }
    return maxvals;
  }

  else if (axis==2) {
    CImg<> maxvals(input.width(), input.height());
    maxvals = -1e10;
#pragma omp parallel for
    cimg_forXY(input, x, y) for (int z=0; z<input.depth(); z++) {
      if (input(x, y, z) > maxvals(x, y))
        maxvals(x, y) = input(x, y, z);
    }
    return maxvals;
  }
  else {
    throw std::runtime_error("unknown axis number in MaxIntProj()");
  }
  return out;
}
