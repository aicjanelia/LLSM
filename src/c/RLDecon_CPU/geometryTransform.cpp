#include "linearDecon.h"


void deskew(const CImg<> &inBuf, double deskewFactor, CImg<> &outBuf, int extraShift)
{
  auto nz = inBuf.depth();
  auto ny = inBuf.height();
  auto nx = inBuf.width();
  auto nxOut = outBuf.width();
#pragma omp parallel for
  for (auto zout=0; zout < nz; zout++)
    for (auto yout=0; yout < ny; yout++)
      for (auto xout=0; xout < nxOut; xout++) {
        const double xin = (xout - nxOut/2. + extraShift) - deskewFactor*(zout-nz/2.) + nx/2.;

        if (xin >= 0 && floor(xin) <= nx-1) {
          const double offset = xin - floor(xin);
          const int floor_xin = floor(xin);
          outBuf(xout, yout, zout) = (1.0 - offset) * inBuf(floor_xin, yout, zout)
            + offset * inBuf(floor_xin+1, yout, zout);
        }
        else
          outBuf(xout, yout, zout) = 100.f;
  }
}



void rotate(const CImg<> &inBuf, const CImg<> &rotMat, CImg<> &outBuf)
{
  auto nz = inBuf.depth();
  auto ny = inBuf.height();
  auto nx = inBuf.width();
  auto nxOut = outBuf.width();
  auto nxy = nxOut * ny;

#pragma omp parallel for
  for (auto zout=0; zout < nz; zout++)
    for (auto yout=0; yout < ny; yout++)
      for (auto xout=0; xout < nxOut; xout++) {
    float xout_centered, zout_centered;
    xout_centered = xout - nxOut/2.;
    zout_centered = zout - nz/2.;

    float zin = rotMat[0] * zout_centered + rotMat[1] * xout_centered + nz/2.;
    float xin = rotMat[2] * zout_centered + rotMat[3] * xout_centered + nxOut/2.;

    unsigned indout = (nz-1-zout) * nxy + yout*nxOut + xout; // flip z indices

    if (xin >= 0 && xin <= nx-1 && zin >=0 && zin <= nz-1) {

      unsigned indin00 = (unsigned) floor(zin) * nx*ny + yout*nx + (unsigned) floor(xin);
      unsigned indin01 = indin00 + 1;
      unsigned indin10 = indin00 + nx*ny;
      unsigned indin11 = indin10 + 1;

      float xoffset = xin - floor(xin);
      float zoffset = zin - floor(zin);
      outBuf[indout] = (1-zoffset) * ( (1-xoffset) * inBuf[indin00] + xoffset * inBuf[indin01]) + 
        zoffset * ((1-xoffset) * inBuf[indin10] + xoffset * inBuf[indin11]);
    }
    else
      outBuf[indout] = 0.f;
  }
}

// __global__ void crop_kernel(float *in, int nx, int ny, int nz,
//                             int new_nx, int new_ny, int new_nz,
//                             float *out)
// {
//   unsigned xout = blockIdx.x * blockDim.x + threadIdx.x;
//   unsigned yout = blockIdx.y;
//   unsigned zout = blockIdx.z;

//   if (xout < new_nx) { 
//     // Assumption: new dimensions are <= old ones
//     unsigned xin = xout + nx - new_nx;
//     unsigned yin = yout + ny - new_ny;
//     unsigned zin = zout + nz - new_nz;
//     unsigned indout = zout * new_nx * new_ny + yout * new_nx + xout;
//     unsigned indin = zin * nx * ny + yin * nx + xin;
//     out[indout] = in[indin];
//   }
// }


// __host__ void cropGPU(GPUBuffer &inBuf, int nx, int ny, int nz,
//                       int new_nx, int new_ny, int new_nz,
//                       GPUBuffer &outBuf)
// {

//   dim3 block(128, 1, 1);
//   unsigned nxBlocks = (unsigned ) ceil(new_nx / (float) block.x);
//   dim3 grid(nxBlocks, new_ny, new_nz);

//   crop_kernel<<<grid, block>>>((float *) inBuf.getPtr(),
//                                nx, ny, nz,
//                                new_nx, new_ny, new_nz,
//                                (float *) outBuf.getPtr());

// #ifndef NDEBUG
//   std::cout<< "cropGPU(): " << cudaGetErrorString(cudaGetLastError()) << std::endl;
// #endif
// }
