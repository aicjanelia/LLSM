#how many cores to use; default to 4
ncores=4

#parse option flag "-c ncores" to see if more or less than 4 cores are requested
while getopts ":c:" opt; do
  case $opt in
    c)
      ncores=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

shift $((OPTIND-1))

# remember the data folder
datafolder=$1

umask 0002
# make a subfolder matlab_decon in $datafolder
# mkdir "$datafolder/matlab_decon" 2>/dev/null

# $pat stores the identifying pattern in the name of the files to be processed
pat=$2
# $psf stores the relative path to the PSF file
psf=$3

shift 3
# so as to pass the rest of the operands on to cpuDeconv (that's what $@ represents)

find "$datafolder" -maxdepth 1 -wholename "*$pat*" -exec bsub -o /dev/null -We 10 -n 4 ~/RLDecon_CPU/build-cluster/cpuDeconv -Z 0.1 -b 100 $@ {} $psf \;
