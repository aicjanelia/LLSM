
#include <iostream>
#include <complex>
#include <stdexcept>

#include <stdio.h>

#include <fftw3.h>

#define cimg_use_openmp
#define cimg_use_tiff
#define cimg_use_cpp11 1
#include "CImg.h"
using namespace cimg_library;

#ifdef _WIN32
#define _USE_MATH_DEFINES
#define rint(fp) (int)((fp) >= 0 ? (fp) + 0.5 : (fp) - 0.5)
#endif

#include <math.h>


float fitparabola(float a1, float a2, float a3);
void determine_center_and_background(float *stack5phases, int nx, int ny, int nz, float *xc, float *yc, float *zc, float *background);
void shift_center(std::complex<float> *bands, int nx, int ny, int nz, float xc, float yc, float zc);
void cleanup(std::complex<float> *otfkxkz, int nx, int nz, float dkr, float dkz, float linespacing, int lambdanm, int twolens, float NA, float NIMM);
void radialft(std::complex<float> *bands, int nx, int ny, int nz, std::complex<float> *avg);

bool fixorigin(std::complex<float> *otfkxkz, int nx, int nz, int kx2);
void rescale(std::complex<float> *otfkxkz, int nx, int nz);


CImg<> otfgen(std::string &PSF, float dr, float dz, int interpkr)
{
  fftwf_plan rfftplan3d;

  // float NA, NIMM;
  // int lambdanm;
  // int krmax=0;

  CImg<> rawtiff(PSF.c_str());

  const auto nz = rawtiff.depth();
  const auto ny = rawtiff.height();
  const auto nx = rawtiff.width();

  printf("In otfgen() ...\n");

  // printf("nx=%d, ny=%d, nz=%d\n", nx, ny, nz);

  // const float dkr = 1/(ny*dr);
  // const float dkz = 1/(nz*dz);

  CImg<> fftBuf(nx+2, ny, nz, 1);
  std::complex<float> *bands = (std::complex<float> *) fftBuf.data();

  float xcofm, ycofm, zcofm, estBackground;
  // Before FFT, estimate bead center position:
  determine_center_and_background(rawtiff.data(), nx, ny, nz, &xcofm, &ycofm, &zcofm, &estBackground);

  // printf("Center of mass is (%.3f, %.3f, %.3f)\n\n", xcofm, ycofm, zcofm);

  float background;
  background = 100.f; // TODO: estimated background in LLSM PSF, which is usually small FOV, is not accurate in genral.
  // printf("PSF Background is %.3f\n", background);

  rawtiff -= background;
  rfftplan3d = fftwf_plan_dft_r2c_3d(nz, ny, nx, rawtiff, (fftwf_complex *) fftBuf.data(), FFTW_ESTIMATE);
  fftwf_execute(rfftplan3d);
  fftwf_destroy_plan(rfftplan3d);

  /* modify the phase of bands, so that it corresponds to FFT of a bead at origin */
  shift_center(bands, nx, ny, nz, xcofm, ycofm, zcofm);

  CImg<> output(nz*2, nx/2+1, 1, 1, 0.f);
  std::complex<float> * avg_output = (std::complex<float> *) output.data();

  radialft(bands, nx, ny, nz, avg_output);

  // if (bDoCleanup)
  //   cleanup(avg_output, nx, nz, dkr, dkz, 1.0, lambdanm, krmax, NA, NIMM);

  if (interpkr > 0)
    while (!fixorigin(avg_output, nx, nz, interpkr)) {
      interpkr --;
      if (interpkr < 5)
        throw std::runtime_error("#pixels < 5 used in kr=0 extrapolation");
    }
  rescale(avg_output, nx, nz);

  return output;
}

/*  locate peak pixel to subpixel accuracy by fitting parabolas  */
void determine_center_and_background(float *stack3d, int nx, int ny, int nz, float *xc, float *yc, float *zc, float *background)
{
  int i, j, k, maxi, maxj, maxk, ind, nxy, infocus_sec;
  int iminus, iplus, jminus, jplus, kminus, kplus;
  float maxval, reval, valminus, valplus;
  double sum;

  nxy = nx*ny;

  // Search for the peak pixel:
  maxval=-1e10;
  for(k=0;k<nz;k++)
    for(i=0;i<ny;i++)
      for(j=0;j<nx;j++) {
        ind=k*nxy+i*nx+j;
        reval=stack3d[ind];
        if( reval > maxval ) {
          maxval = reval;
          maxi=i; maxj=j;
          maxk=k;
        }
      }

  iminus = maxi-1; iplus = maxi+1;
  if( iminus<0 ) iminus+=ny;
  if( iplus>=ny ) iplus-=ny;
  jminus = maxj-1; jplus = maxj+1;
  if( jminus<0 ) jminus+=nx;
  if( jplus>=nx ) jplus-=nx;
  kminus = maxk-1; kplus = maxk+1;
  if( kminus<0 ) kminus+=nz;
  if( kplus>=nz ) kplus-=nz;

  valminus = stack3d[kminus*nxy+maxi*nx+maxj];
  valplus  = stack3d[kplus *nxy+maxi*nx+maxj];
  *zc = maxk + fitparabola(valminus, maxval, valplus);

  // *zc += 0.6;

  valminus = stack3d[maxk*nxy+iminus*nx+maxj];
  valplus  = stack3d[maxk*nxy+iplus *nx+maxj];
  *yc = maxi + fitparabola(valminus, maxval, valplus);

  valminus = stack3d[maxk*nxy+maxi*nx+jminus];
  valplus  = stack3d[maxk*nxy+maxi*nx+jplus];
  *xc = maxj + fitparabola(valminus, maxval, valplus);

  sum = 0;
  int nPixels = 0;
  infocus_sec = floor(*zc);
  for (i=0; i<*yc-20; i++)
    for (j=0; j<nx; j++) {
      sum += stack3d[infocus_sec*nxy + i*nx + j];
      nPixels ++;
    }
  
  for (i=*yc-20; i<ny; i++)
    for (j=0; j<nx; j++) {
      sum += stack3d[infocus_sec*nxy + i*nx + j];
      nPixels ++;
    }
  *background = sum / nPixels;
}

/***************************** fitparabola **********************************/
/*     Fits a parabola to the three points (-1,a1), (0,a2), and (1,a3).     */
/*     Returns the x-value of the max (or min) of the parabola.             */
/****************************************************************************/

float fitparabola( float a1, float a2, float a3 )
{
 float slope,curve,peak;

 slope = 0.5* (a3-a1);         /* the slope at (x=0). */
 curve = (a3+a1) - 2*a2;       /* (a3-a2)-(a2-a1). The change in slope per unit of x. */
 if( curve == 0 )
 {
   printf("no peak: a1=%f, a2=%f, a3=%f, slope=%f, curvature=%f\n",a1,a2,a3,slope,curve);
   return( 0.0 );
 }
 peak = -slope/curve;          /* the x value where slope = 0  */
 if( peak>1.5 || peak<-1.5 )
 {
   printf("bad peak position: a1=%f, a2=%f, a3=%f, slope=%f, curvature=%f, peak=%f\n",a1,a2,a3,slope,curve,peak);
   return( 0.0 );
 }
 return( peak );
}


/* To get rid of checkerboard effect in the OTF bands */
/* (xc, yc, zc) is the estimated center of the point source, which in most cases is the bead */
/* Converted from Fortran code. kz is treated differently than kx and ky. don't know why */
void shift_center(std::complex<float> *bands, int nx, int ny, int nz, float xc, float yc, float zc)
{
  int kin, iin, jin, indin, nxy, kz, kx, ky, kycent, kxcent, kzcent;
  std::complex<float> exp_iphi;
  float phi1, phi2, phi, dphiz, dphiy, dphix;

  kycent = ny/2;
  kxcent = nx/2;
  kzcent = nz/2;
  nxy = (nx/2+1)*ny;

  dphiz = 2*M_PI*zc/nz;
  dphiy = 2*M_PI*yc/ny;
  dphix = 2*M_PI*xc/nx;

  for (kin=0; kin<nz; kin++) {    /* the origin of Fourier space is at (0,0) */
    kz = kin;
    if (kz>kzcent) kz -= nz;
    phi1 = dphiz*kz;      /* first part of phi */
    for (iin=0; iin<ny; iin++) {
      ky = iin;
      if (iin>kycent) ky -= ny;
      phi2 = dphiy*ky;   /* second part of phi */
      for (jin=0; jin<kxcent+1; jin++) {
        kx = jin;
        indin = kin*nxy+iin*(nx/2+1)+jin;
        phi = phi1+phi2+dphix*kx;  /* third part of phi */
        /* kz part of Phi has a minus sign, I don't know why. */
        exp_iphi = std::complex<float> (cos(phi), sin(phi));
        bands[indin] = bands[indin] * exp_iphi;
      }
    }
  }
}

void radialft(std::complex<float> *band, int nx, int ny, int nz, std::complex<float> *avg_output)
{
  int kin, iin, jin, indin, indout, indout_conj, kz, kx, ky, kycent, kxcent, kzcent;
  int *count, nxz, nxy;
  float rdist;

  // printf("In radialft()\n");
  kycent = ny/2;
  kxcent = nx/2;
  kzcent = nz/2;
  nxy = (nx/2+1)*ny;
  nxz = (nx/2+1)*nz;

  count = (int *) calloc(nxz, sizeof(int));

  if (!count) {
    printf("No memory availale in radialft()\n");
    exit(-1);
  }

  for (kin=0; kin<nz; kin++) {
    kz = kin;
    if (kin>kzcent) kz -= nz;
    for (iin=0; iin<ny; iin++) {
      ky = iin;
      if (iin>kycent) ky -= ny;
      for (jin=0; jin<kxcent+1; jin++) {
        kx = jin;
        rdist = sqrt(kx*kx+ky*ky);
        if (rdist < nx/2+1) {
          indin = kin*nxy+iin*(nx/2+1)+jin;
          indout = rint(rdist)*nz+kin;
          if (indout < nxz) {
            avg_output[indout] += band[indin];
            count[indout] ++;
          }
        }
      }
    }
  }

  for (indout=0; indout<nxz; indout++) {
    if (count[indout]>0) {
      avg_output[indout] /= count[indout];
    }
  }

  /* Then complete the rotational averaging and scaling*/
  for (kx=0; kx<nx/2+1; kx++) {
    indout = kx*nz+0;
    avg_output[indout] = std::complex<float>(avg_output[indout].real(), 0);
    for (kz=1; kz<=nz/2; kz++) {
      indout = kx*nz+kz;
      indout_conj = kx*nz + (nz-kz);
      avg_output[indout] = (avg_output[indout] + conj(avg_output[indout_conj])) / 2.f;
      avg_output[indout_conj] = conj(avg_output[indout]);
    }
  }
  free(count);
}

void cleanup(std::complex<float> *otfkxkz, int nx, int nz, float dkr, float dkz, float linespacing, int lamdanm, int krmax_user, float NA, float NIMM)
{
  int ix, iz, kzstart, kzend, icleanup=nx/2+1;
  float lamda, sinalpha, cosalpha, kr, krmax, beta, kzedge;


  lamda = lamdanm * 0.001;
  sinalpha = NA/NIMM;
  cosalpha = cos(asin(sinalpha));
  krmax = 2*NA/lamda;
  if (krmax_user*dkr<krmax && krmax_user!=0)
    krmax = krmax_user*dkr;

  printf("krmax=%f\n", krmax);
  for (ix=0; ix<icleanup; ix++) {
    kr = ix * dkr;
    if ( kr <= krmax ) {
      beta = asin( ( NA - kr*lamda ) /NIMM );
      kzedge = (NIMM/lamda) * ( cos(beta) - cosalpha );
      /* kzstart = floor((kzedge/dkz) + 1.999); */ /* In fortran, it's 2.999 */
      kzstart = rint((kzedge/dkz) + 1);
      kzend = nz - kzstart;
      for (iz=kzstart; iz<=kzend; iz++)
        otfkxkz[ix*nz+iz] = 0;
    }
    else {   /* outside of lateral resolution limit */
      for (iz=0; iz<nz; iz++)
        otfkxkz[ix*nz+iz] = 0;
    }
  }
}

bool fixorigin(std::complex<float> *otfkxkz, int nx, int nz, int kx2)
{
  // linear fit the value at kx=0 using kx in [1, kx2]
  double mean_kx = (kx2+1)/2.; // the mean of [1, 2, ..., n] is (n+1)/2

  for (int z=0; z<nz; z++) {
    std::complex<double> mean_val=0;
    std::complex<double> slope_numerator=0;
    std::complex<double> slope_denominat=0;

    for (int x=1; x<=kx2; x++)
      mean_val += otfkxkz[x*nz + z];

    mean_val /= kx2;
    for (int x=1; x<=kx2; x++) {
      std::complex<double> complexval = otfkxkz [x*nz+z];
      slope_numerator += (x - mean_kx) * (complexval - mean_val);
      slope_denominat += (x - mean_kx) * (x - mean_kx);
    }
    std::complex<double> slope = slope_numerator / slope_denominat;
    otfkxkz[z] = mean_val - slope * mean_kx;  // intercept at kx=0
    if (z==0 && std::abs(otfkxkz[z]) <= std::abs(otfkxkz[nz+z])) {
      return false; // indicating kx2 may be too large
    }
  }
  return true;
}

void rescale(std::complex<float> *otfkxkz, int nx, int nz)
{
  int nxz, ind;
  float valmax=0, mag, scalefactor;

  nxz = (nx/2+1)*nz;
  for (ind=0; ind<nxz; ind++) {
    mag = abs(otfkxkz[ind]);
    if (mag > valmax)
      valmax = mag;
  }
  scalefactor = 1/valmax;

  for (ind=0; ind<nxz; ind++) {
    otfkxkz[ind] *= scalefactor;
  }
}

