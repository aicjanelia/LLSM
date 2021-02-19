#include "linearDecon.h"

#include <vector>
#include <algorithm>  // max(), min()
#include <limits>  // epsilon()

bool notGoodDimension(unsigned num)
/*! Good dimension is defined as one that can be fatorized into 2s, 3s, 5s, and 7s
  According to CUFFT manual, such dimension would warranty fast FFT
*/
{
  if (num==2 || num==3 || num==5 || num==7)
    return false;
  else if (num % 2 == 0) return notGoodDimension(num / 2);
  else if (num % 3 == 0) return notGoodDimension(num / 3);
  else if (num % 5 == 0) return notGoodDimension(num / 5);
  else if (num % 7 == 0) return notGoodDimension(num / 7);
  else
    return true;
}

unsigned findOptimalDimension(unsigned inSize, int step)
/*!
  "step" can be positive or negative
*/
{
  unsigned outSize = inSize;
  while (notGoodDimension(outSize))
    outSize += step;

  return outSize;
}

// // static globals for photobleach correction:
// static double intensity_overall0 = 0.;
// static bool bFirstTime = true;

void RichardsonLucy(CImg<> & raw, CImg<> & rawFFT,
                    float kxscale, float kyscale, float kzscale, 
                    const CImg<> & otf, float dkr_otf, float dkz_otf,
                    int nIter, float deskewFactor,
                    int deskewedXdim, int extraShift,
                    int napodize, int nZblend,
                    const CImg<> & rotMatrix,
                    CImg<> & raw_deskewed,
                    fftwf_plan rfftplan, fftwf_plan rfftplan_inv)
{
  // "raw" contains the raw image, also used as the initial guess X_0

  if (nIter < 0) throw std::runtime_error("nIter is < 0 in RichardsonLucy");

  unsigned int nx = raw.width();
  const unsigned int ny = raw.height();
  const unsigned int nz = raw.depth();

  // unsigned int nxy = nx * ny;
  // unsigned int nxy2 = (nx+2)*ny;

  CImg<> X_k(raw);
  if (nIter > 0 || raw_deskewed.size()>0 || rotMatrix.size()) {
    apodize(X_k, napodize);

    if (fabs(deskewFactor) > 0.0) { //then deskew raw data along x-axis first:

      if (nIter > 0)
        deskew(X_k, deskewFactor, raw_deskewed, extraShift); // fill-in value for the void is 0
      else {
        deskew(raw, deskewFactor, raw_deskewed, extraShift);
        return;
      }
      // update X_k and its dimension variables.
      // X_k.assign();  // not sure if this is necessary
      X_k.assign(raw_deskewed);

      nx = deskewedXdim;
      // nxy = nx*ny;
      // nxy2 = (nx+2)*ny;

      raw.assign(nx, ny, nz, 1);
    }

    if (nZblend > 0)
      zBlend(X_k, nZblend);
  }

  CImg<> rawbuf(X_k);  // make a copy of (deskewed) raw image
  CImg<> X_kminus1;
    // std::cout << "Line:" << __LINE__ << std::endl;
  CImg<> Y_k;
  CImg<> G_kminus1(nx, ny, nz);
  CImg<> G_kminus2;
  CImg<> CC;

  double lambda=0;
  float eps = std::numeric_limits<float>::epsilon();

  // R-L iteration
  for (auto k = 0; k < nIter; k++) {
    std::cout << "Iteration " << k << std::endl;
    // a. Make an image predictions for the next iteration    
    if (k > 1) {
      lambda = calcAccelFactor(G_kminus1, G_kminus2, eps);
      lambda = std::max(std::min(lambda, 1.), 0.); // stability enforcement
#ifndef NDEBUG
      printf("labmda = %lf\n", lambda);
#endif
      updatePrediction(Y_k, X_k, X_kminus1, lambda);
    }
    else 
      Y_k.assign(X_k);

    X_kminus1.assign(X_k);
    if (k>0)
      G_kminus2.assign(G_kminus1);
    
    // b.  Make core for the LR estimation ( raw/reblurred_current_estimation )
    CC.assign(Y_k);
    // CC.display();
    filter(CC, otf, kxscale, kyscale, kzscale,
           rfftplan, rfftplan_inv, rawFFT, false);
    // CC.display();
    calcLRcore(CC, rawbuf, eps);

    // c. Determine next iteration image & apply positivity constraint
    // X_kminus1 = X_k;
    filter(CC, otf, kxscale, kyscale, kzscale,
           rfftplan, rfftplan_inv, rawFFT, true);

    updateCurrEstimate(X_k, CC, Y_k);

    calcCurrPrevDiff(X_k, Y_k, G_kminus1);
  }

  // Rotate decon result if requested:
  
  if (rotMatrix.size()) {
    CImg<> rotatedResult(nx, ny, nz);

    rotate(X_k, rotMatrix, rotatedResult);
    // Copy rotated decon result back to "raw":
    raw = rotatedResult;
  }

  else {
    // Copy decon result back to "raw":
    raw = X_k;
  }

  // result is returned in "raw"
}


// unsigned output_ny, output_nz, output_nx;
// bool bCrop;
// CImg<> complexOTF;
// double deskewFactor;
// unsigned deskewedXdim;
// CImg<> rotMatrix;
// cufftHandle rfftplanGPU, rfftplanInvGPU;
// GPUBuffer d_interpOTF(0);

// unsigned get_output_nx()
// {
//   return deskewedXdim; //output_nx;
// }

// unsigned get_output_ny()
// {
//   return output_ny;
// }
// unsigned get_output_nz()
// {
//   return output_nz;
// }

// int RL_interface_init(int nx, int ny, int nz, // raw image dimensions
//                       float dr, float dz, // raw image pixel sizes
//                       float dr_psf, float dz_psf, // PSF image pixel sizes
//                       float deskewAngle, // deskew
//                       float rotationAngle,
//                       int outputWidth,
//                       char * OTF_file_name)
// {
//   // Find the optimal dimensions nearest to the originals to meet CUFFT demands
//   bCrop = false;
//   output_ny = findOptimalDimension(ny);
//   if (output_ny != ny) {
//     printf("output ny=%d\n", output_ny);
//     bCrop = true;
//   }

//   output_nz = findOptimalDimension(nz);
//   if (output_nz != nz) {
//     printf("output nz=%d\n", output_nz);
//     bCrop = true;
//   }

//   // only if no deskewing is happening do we want to change image width here
//   output_nx = nx;
//   if ( ! (fabs(deskewAngle) > 0.0) ){
//     output_nx = findOptimalDimension(nx);
//     if (output_nx != nx) {
//       printf("new nx=%d\n", output_nx);
//       bCrop = true;
//     }
//   }

//   // Load OTF and obtain OTF dimensions and pixel sizes, etc:
//   try {
//     complexOTF.assign(OTF_file_name);
//   }
//   catch (CImgIOException &e) {
//     std::cerr << e.what() << std::endl; //OTF_file_name << " cannot be opened\n";
//     return 0;
//   }
//   unsigned nr_otf = complexOTF.height();
//   unsigned nz_otf = complexOTF.width() / 2;
//   float dkr_otf = 1/((nr_otf-1)*2 * dr_psf);
//   float dkz_otf = 1/(nz_otf * dz_psf);

//   // Obtain deskew factor and new x dimension if deskew is run:
//   deskewFactor = 0.;
//   deskewedXdim = output_nx;
//   if (fabs(deskewAngle) > 0.0) {
//     if (deskewAngle <0) deskewAngle += 180.;
//     deskewFactor = cos(deskewAngle * M_PI/180.) * dz / dr;
//     if (outputWidth ==0)
//       deskewedXdim += floor(output_nz * dz * 
//                             fabs(cos(deskewAngle * M_PI/180.)) / dr)/4.; // TODO /4.
//     else
//       deskewedXdim = outputWidth; // use user-provided output width if available

//     deskewedXdim = findOptimalDimension(deskewedXdim);
//     // update z step size: (this is fine even though dz is a function parameter)
//     dz *= sin(deskewAngle * M_PI/180.);
//   }

//   // Construct rotation matrix:
//   if (fabs(rotationAngle) > 0.0) {
//     rotMatrix.resize(4*sizeof(float));
//     rotationAngle *= M_PI/180;
//     float stretch = dr / dz;
//     float *p = (float *)rotMatrix.getPtr();
//     p[0] = cos(rotationAngle) * stretch;
//     p[1] = sin(rotationAngle) * stretch;
//     p[2] = -sin(rotationAngle);
//     p[3] = cos(rotationAngle);
//   }

//   // Create reusable cuFFT plans
//   cufftResult cuFFTErr = cufftPlan3d(&rfftplanGPU, output_nz, output_ny, deskewedXdim, CUFFT_R2C);
//   if (cuFFTErr != CUFFT_SUCCESS) {
//     std::cerr << "cufftPlan3d() c2r failed\n";
//     return 0;
//   }
//   cuFFTErr = cufftPlan3d(&rfftplanInvGPU, output_nz, output_ny, deskewedXdim, CUFFT_C2R);
//   if (cuFFTErr != CUFFT_SUCCESS) {
//     std::cerr << "cufftPlan3d() c2r failed\n";
//     return 0;
//   }

//   // Pass some constants to CUDA device:
//   float dkx = 1.0/(dr * deskewedXdim);
//   float dky = 1.0/(dr * output_ny);
//   float dkz = 1.0/(dz * output_nz);
//   float eps = std::numeric_limits<float>::epsilon();
//   transferConstants(deskewedXdim, output_ny, output_nz, nr_otf, nz_otf,
//                     dkx/dkr_otf, dky/dkr_otf, dkz/dkz_otf,
//                     eps, complexOTF.data());

//   // make a 3D interpolated OTF array:
//   d_interpOTF.resize(output_nz * output_ny * (deskewedXdim+2) * sizeof(float));
//   // catch exception here
//   makeOTFarray(d_interpOTF, deskewedXdim, output_ny, output_nz);
//   return 1;
// }

// int RL_interface(const unsigned short * const raw_data,
//                  int nx, int ny, int nz,
//                  float * const result,
//                  float background,
//                  int nIters,
//                  int extraShift
//                  )
// {
//   CImg<> raw_image(raw_data, nx, ny, nz);

//   if (bCrop)
//     raw_image.crop(0, 0, 0, 0, output_nx-1, output_ny-1, output_nz-1, 0);

//   cudaDeviceProp deviceProp;
//   cudaGetDeviceProperties(&deviceProp, 0);

//   // Finally do calculation including deskewing, decon, rotation:
//   CImg<> raw_deskewed;
//   RichardsonLucy_GPU(raw_image, background, d_interpOTF, nIters,
//                      deskewFactor, deskewedXdim, extraShift, 15, 10, rotMatrix,
//                      rfftplanGPU, rfftplanInvGPU, raw_deskewed, &deviceProp);

//   // Copy deconvolved data, stored in raw_image, to "result" for return:
//   memcpy(result, raw_image.data(), raw_image.size() * sizeof(float));

//   return 1;
// }

// void RL_cleanup()
// {
//   d_interpOTF.resize(0);
//   cufftDestroy(rfftplanGPU);
//   cufftDestroy(rfftplanInvGPU);
// }
