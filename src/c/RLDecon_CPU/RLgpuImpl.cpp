#include "linearDecon.h"


std::complex<float> otfinterpolate(std::complex<float> * otf, float kx, float ky, float kz, int nzotf, int nrotf)
// Use sub-pixel coordinates (kx,ky,kz) to linearly interpolate a rotationally-averaged 3D OTF ("otf").
// otf has 2 dimensions: fast dimension is kz with length "nzotf" while the slow dimension is kr.
{
  int irindex, izindex, indices[2][2];
  float krindex, kzindex;
  float ar, az;

  krindex = sqrt(kx*kx + ky*ky);
  kzindex = (kz<0 ? kz+nzotf : kz);

  if (krindex < nrotf-1 && kzindex < nzotf) {
    irindex = floor(krindex);
    izindex = floor(kzindex);

    ar = krindex - irindex;
    az = kzindex - izindex;  // az is always 0 for 2D case, and it'll just become a 1D interp

    if (izindex == nzotf-1) {
      indices[0][0] = irindex*nzotf+izindex;
      indices[0][1] = irindex*nzotf+0;
      indices[1][0] = (irindex+1)*nzotf+izindex;
      indices[1][1] = (irindex+1)*nzotf+0;
    }
    else {
      indices[0][0] = irindex*nzotf+izindex;
      indices[0][1] = irindex*nzotf+(izindex+1);
      indices[1][0] = (irindex+1)*nzotf+izindex;
      indices[1][1] = (irindex+1)*nzotf+(izindex+1);
    }

    return (1-ar)*(otf[indices[0][0]]*(1-az) + otf[indices[0][1]]*az) +
      ar*(otf[indices[1][0]]*(1-az) + otf[indices[1][1]]*az);
  }
  else
    return std::complex<float>(0, 0);
}


void filter(CImg<> &img, const CImg<> &otf,
            float kxscale, float kyscale, float kzscale,
            fftwf_plan rfftplan, fftwf_plan rfftplanInv,
            CImg<> &fftBuf,
            bool bConj)
// "img" is of dimension (nx, ny, nz) and of float type
// "otf" is of dimension (nzotf, nrotf) and of complex type
{

  const auto nx = img.width();
  const auto ny = img.height();
  const auto nz = img.depth();

  fftwf_execute_dft_r2c(rfftplan, img.data(), (fftwf_complex *) fftBuf.data());

#pragma omp parallel for
  for (auto z=0; z<nz; z++) {
    int kz = ( z>nz/2 ? z-nz : z );
    for (auto y=0; y<ny; y++) {
      int ky = ( y > ny/2 ? y-ny : y );
      for (auto x=0; x<nx/2+1; x++) {
        auto kx = x;
        // float kr = sqrt(kx*kx + ky*ky);
        std::complex<float> otfval =
          otfinterpolate((std::complex<float> *)otf.data(),
                         kx*kxscale, ky*kyscale, kz*kzscale,
                         otf.width()/2, otf.height());
        std::complex<float> result;
        if (bConj)
          result = std::conj(otfval) * std::complex<float>(fftBuf(2*x, y, z), fftBuf(2*x+1, y, z));
        else
          result = otfval * std::complex<float>(fftBuf(2*x, y, z), fftBuf(2*x+1, y, z));
        fftBuf(2*x, y, z) = result.real();
        fftBuf(2*x+1, y, z) = result.imag();
      }
    }
  }
  fftwf_execute_dft_c2r(rfftplanInv, (fftwf_complex *) fftBuf.data(), img.data());

  img /= (float) (nx*ny*nz);
}

void calcLRcore(CImg <> &reblurred, const CImg<> &raw, float eps)
// calculate raw image divided by reblurred, a key step in R-L;
// Both input, "reblurred" and "raw", are of dimension (nx, ny, nz) and of floating type;
// "reblurred" is updated upon return.
{
  const auto nxyz = reblurred.width() * reblurred.height() * reblurred.depth();
  
#pragma omp parallel for
  for (auto ind = 0; ind<nxyz; ind++) {
    reblurred[ind] = fabs(reblurred[ind]) > 0 ? reblurred[ind] : eps;
    reblurred[ind] = raw[ind]/reblurred[ind] + eps;
    // The following thresholding is necessary for occasional very high
    // DR data and incorrectly high background value specified (-b flag).
    if (reblurred[ind] > 10) reblurred[ind] = 10;
    if (reblurred[ind] < -10) reblurred[ind] = -10;
  }
}

void updateCurrEstimate(CImg<> &X_k, const CImg<> &CC, const CImg<> &Y_k)

// calculate updated current estimate: Y_k * CC plus positivity constraint
// All inputs are of dimension (nx+2, ny, nz) and of floating type;
// "X_k" is updated upon return.
{
  const auto nxyz = X_k.width() * X_k.height() * X_k.depth();
  
#pragma omp parallel for
  for (auto ind = 0; ind<nxyz; ind++) {
    X_k[ind] = CC[ind] * Y_k[ind];
    X_k[ind] = X_k[ind] > 0 ? X_k[ind] : 0;
  }
}

void calcCurrPrevDiff(const CImg<> &X_k, const CImg<> &Y_k, CImg<> &G_kminus1)
// calculate X_k - Y_k and assign the result to G_kminus1;
// All inputs are of dimension (nx+2, ny, nz) and of floating type;
// "X_k" is updated upon return.
{
  const auto nxyz = X_k.width() * X_k.height() * X_k.depth();
  
#pragma omp parallel for
  for (auto ind = 0; ind<nxyz; ind++) {
    G_kminus1[ind] = X_k[ind] - Y_k[ind];
  }
}

double calcAccelFactor(const CImg<> &G_km1, const CImg<> &G_km2, float eps)
// (G_km1 dot G_km2) / (G_km2 dot G_km2)
// All inputs are of dimension (nx, ny, nz) and of floating type;
{
  const auto nxyz = G_km1.width() * G_km1.height() * G_km1.depth();
  double numerator = 0;
  int ind;
#pragma omp parallel for reduction(+:numerator)
  for (ind = 0; ind<nxyz; ind++)
    numerator = numerator + G_km1[ind] * G_km2[ind];

  double denom = 0;
#pragma omp parallel for reduction(+:denom)
  for (ind = 0; ind<nxyz; ind++)
    denom = denom + G_km2[ind] * G_km2[ind];

  return numerator / (denom + eps);
}

void updatePrediction(CImg<> &Y_k, const CImg<> &X_k, const CImg<> &X_kminus1, double lambda)
{
  // Y_k = X_k + lambda * (X_k - X_kminus1)
  const auto nxyz = X_k.width() * X_k.height() * X_k.depth();
  
#pragma omp parallel for
  for (auto ind = 0; ind<nxyz; ind++) {
    Y_k[ind] = X_k[ind] + lambda * (X_k[ind] - X_kminus1[ind]);
    Y_k[ind] = (Y_k[ind] > 0) ? Y_k[ind] : 0;
  }
}


void apodize(CImg<> &image, const unsigned napodize)
{
  const auto nx = image.width();
  const auto ny = image.height();
  const auto nz = image.depth();
  
#pragma omp parallel for
  for (auto z=0; z < nz; z++) {
    for (auto x=0; x < nx; x++) {
      float diff = (image(x, ny - 1, z) - image(x, 0, z)) / 2.0;
      for (auto y = 0; y < napodize; ++y) {
        float fact = diff * (1.0 - sin(((y + 0.5) / napodize) * M_PI * 0.5));
        image(x, y, z) += fact;
        image(x, ny - 1 - y, z) -=  fact;
      }
    }

    for (auto y=0; y < ny; y++) {
      float diff = (image(nx - 1, y, z) - image(0, y, z)) / 2.0;
      for (auto x = 0; x < napodize; ++x) {
        float fact = diff * (1.0 - sin(((x + 0.5) / napodize) * M_PI * 0.5));
        image(x, y, z) += fact;
        image(nx-1-x, y, z) -= fact;
      }
    }
  }
}

void zBlend(CImg<> & image, int nZblend)
{
  const auto nx = image.width();
  const auto ny = image.height();
  const auto nz = image.depth();
  const unsigned nxy = nx*ny;
  
#pragma omp parallel for
  for (auto yidx=0; yidx < ny; yidx ++) {
    const auto row_offset = yidx * nx;
    for (auto xidx=0; xidx < nx; xidx ++) {
      float diff = image[(nz-1)*nxy + row_offset + xidx] -  image[row_offset + xidx];
      for (auto zidx = 0; zidx < nZblend; zidx ++) {
        float fact = diff * (1.0 - sin(((zidx + 0.5) / (float)nZblend) * M_PI * 0.5));
        image(xidx, yidx, zidx) += fact;
        image(xidx, yidx, nz-zidx-1) -= fact;
      }
    }
  }
}
