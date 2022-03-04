#!/bin/bash
export MCR_CACHE_ROOT=/scratch/shaol/mcr_cache_root.$JOB_ID

umask 0002

background=80
nIter=15
dz_psf=0.1
dz_data=0.25
deskew=0
nphases=1
finalWidth=0
extrashift=0
angle=32.8
reverse=0
pixelsize=0.104
nborder=10
nzblend=0
rotateFinal=0
croplimits=0,0,0,0,0,0
noisemean=90.0
saveAligned=0
flipz=0
resizeFactor=1
scalingThresh=1
saveuint16=0
maxintproj=0,0,0
transResult=0,0,0

# -b: background to subtract from rawdata (default: 80)
# -i: number of iterations (default: 15)
# -Z: z step of PSF (default: 0.1)
# -z: z step of data (default: 0.25)
# -D: de-skew sample-scan dataset (default is not doing it)
# -p: number of phases per z plane (default is 1)
# -w: the width of deskew output instead of using the value estimated by the program
# -x: extra horizontal translation in deskewed result; postive -> shift toward right (default: 0)
# -a: tilt angle: between the sample moving direction and z axis (default: 32.8)
# -r. reverse the deskewing direction (going nz:-1:1); default is going 1:nz
# -s: x-y pixel size in micron (default: 0.104)
# -R: rotate the final result around Y axis so the horizontal plane corresponds to coverslip plane
# -C int,int,int,int,int,int: lower and upper limits for x,y,z-cropping final data; default to 0,0,0,0,0,0
# -m: When only deskewing is performed (i.e., no deconv), mean of noise to fill in the void after deskew (default: 90.0)
# -S: to save the de-skewed raw data before deconv (default is not to save)
# -e: number of x-y edge pixels to soften (default is 10)
# -E: number of z section from top and bottom to blend in (default is 0)
# -n: no flipping Z (default is to flip Z)
# -t: resize (down or up-sampling) factor (default 1)
# -l: rescaling threshold (ask Wes; only has effect when -t is used and not 1) (default 1)
# -u: save uint16 TIFF instead of 32-bit float
# -M: binary,binary,binary: 0 or 1 value to indicate whether to save maximum-intensity projection alogn y, x, or z axis.
# -T float,float,float: number of pixels (tx,ty,tz in decimals) to translate the decon result, for example, for chromatic correction

while getopts ":b:i:z:Z:Dp:w:x:a:rs:RC:m:Se:nt:l:uM:T:" opt; do
  case $opt in
    b)
      background=$OPTARG
      ;;
    i)
      nIter=$OPTARG
      ;;
    z)
      dz_data=$OPTARG
      ;;
    Z)
      dz_psf=$OPTARG
      ;;
    D)
      deskew=1
      ;;
    p)
      nphases=$OPTARG
      ;;
    w)
      finalWidth=$OPTARG
      ;;
    x)
      extrashift=$OPTARG
      ;;
    a)
      angle=$OPTARG
      ;;
    r)
      reverse=1
      ;;
    s)
      pixelsize=$OPTARG
      ;;
    R)
      rotateFinal=1
      ;;
    C)
	  croplimits=$OPTARG
      ;;
    m)
      noisemean=$OPTARG
      ;;
    S)
      saveAligned=1
      ;;
    e)
      nborder=$OPTARG
      ;;
    E)
      nzblend=$OPTARG
      ;;
    n)
      flipz=0
      ;;
    t)
      resizeFactor=$OPTARG
      ;;
    l)
      scalingThresh=$OPTARG
      ;;
    u)
      saveuint16=1
      ;;
    M)
	  maxintproj=$OPTARG
      ;;
	T)
	  transResult=$OPTARG
	  ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

shift $((OPTIND-1))

/groups/betzig/home/shaol/matlab/RLdecon "$1" "$2" $background $nIter $dz_psf $dz_data $deskew [$finalWidth,$extrashift,$reverse,$noisemean,$nphases,$saveAligned] $angle $pixelsize $rotateFinal $saveuint16 [$croplimits] $flipz [$maxintproj] $resizeFactor $scalingThresh $nborder $nzblend [$transResult]


rm -rf /scratch/shaol/mcr_cache_root.$JOB_ID
