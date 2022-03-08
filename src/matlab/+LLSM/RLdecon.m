function RLdecon(input_tiff, psf, background, nIter, dz_psf, dz_data, ...
    zalign, zalignParams, rotateByAngle, xypixelsize, bRotFinal, ...
    bSaveUint16, cropFinal, bFlipZ, axesMaxIntProj, resizeFactor, scalingThresh, varargin)
%RLdecon Summary of this function goes here
%   input_tiff: input TIFF file name
%   psf: psf array in 'double'
%   background: backgroud to subtract
%   nIter: number of iterations
%   zalign: is 1 when de-skewing is needed for sample-scan data
%   zalignParams: parameters related to de-skewing [extrashift reverse mean saveAligned]
%   rotateByAngle: angle used in deskewing and rotating final results
%   bRotFinal: 1 to rotate the final result to make horizontal plane
%   correspond to coverslip plane
%   CropFinal: [lower upper] to crop the final result in Z; if
%   upper==lower then no cropping will be done

if ischar(dz_psf)
    dz_psf=str2double(dz_psf);
end
if ischar(dz_data)
    dz_data=str2double(dz_data);
end

if ischar(zalign)
    bZAlign = logical(str2double(zalign));
else
    bZAlign = logical(zalign);
end

if ischar(rotateByAngle)
    rotateByAngle = str2double(rotateByAngle);
end

if ischar(xypixelsize)
    xypixelsize = str2double(xypixelsize);
end

bReverse = false;
if bZAlign
    if ischar(zalignParams)
        zalignParams = str2num(zalignParams);
    end
    outputWidth = zalignParams(1);
    extraShift = zalignParams(2);
    bReverse = logical(zalignParams(3));
    fillval = zalignParams(4);
    nphases = zalignParams(5);
    bSaveRawT = logical(zalignParams(6));
    
    dz_data_ratio = sin(rotateByAngle*pi/180);
else
    dz_data_ratio = 1;
end

if ischar(psf)
    [a,b,suffix]=fileparts(psf);
    if strcmp(suffix, '.mat')
        load(psf, 'psf');
    elseif strcmp(suffix, '.tif')
        psf=psf_gen(psf, dz_psf, dz_data*dz_data_ratio, 48);
    else
        psf = [];  % dummy PSF
    end
end

if ischar(background)
    background=str2double(background);
end
if ischar(nIter)
    nIter=str2num(nIter);
end


nTapering = 0;
nZblend = 0;
txyz = [0 0 0];
for k = 1 : length(varargin);
    switch k
        case 1
            % number of pixels for x-y tapering
            if ischar(varargin{k})
                nTapering = str2num(varargin{k});
            else
                nTapering = varargin{k};
            end
        case 2
            % number of pixels for z padding
            if ischar(varargin{k})
                nZblend = str2num(varargin{k});
            else
                nZblend = varargin{k};
            end
        case 3
            % if any of the three numbers are non-zero, then translate the decon results
            if ischar(varargin{k})
                txyz = str2num(varargin{k});
            else
                txyz = varargin{k};
            end
        otherwise
            disp('Unknown varargin index')
    end
end

rawdata = loadtiff(input_tiff);
[datafolder, inputfile, suffix] = fileparts(input_tiff);

% rawdata = hpbuster(rawdata, background, 2, 1);
% tic
nz = size(rawdata, 3);

rawdata = single(rawdata) - background;

% soften the x-y borders if requested
if nTapering > 0
    taperKernel = fspecial('gaussian', nTapering+30, nTapering);
    for iz=1:nz
        rawdata(:,:,iz) = edgetaper(rawdata(:,:,iz), taperKernel);
    end
end

% de-skew data if requested
if bZAlign
    size(rawdata)
    rawdata = shear3DinDim2(rawdata, rotateByAngle, bReverse, ...
                dz_data, xypixelsize, 0, outputWidth, extraShift, nphases);
    size(rawdata)
    if bSaveRawT
        if isempty(datafolder)
            if ~exist('deskewed', 'dir')
                mkdir('deskewed');
            end
            write3Dtiff(rawdata, ['deskewed/', inputfile, '_deskewed', suffix]);
        else
            if ~exist([datafolder, '/deskewed'], 'dir')
                mkdir([datafolder, '/deskewed']);
            end
            write3Dtiff(uint16(rawdata), [datafolder, '/deskewed/', inputfile, '_deskewed', suffix]);
        end
%        return
    end
end

if nZblend > 0
    diff = rawdata(:,:,nz) - rawdata(:,:,1);
    for iz=1:nZblend
        fact = 1 - sin((iz+0.5)/nZblend*pi*0.5);
        rawdata(:,:,iz) = rawdata(:,:,iz) + diff * fact;
        rawdata(:,:,nz-iz+1) = rawdata(:,:,nz-iz+1) - diff * fact;
    end
end

rawdata(rawdata<0.0) = 0.0;

% call Richardson-Lucy
if nIter>0
    size(psf)
    deconvolved = deconvlucy(rawdata, psf, nIter) * numel(rawdata);
else
    deconvolved = rawdata;
end

% Translate the decon result if requested
if any(abs(txyz)>0)
    deconvolved = translate3D(deconvolved, txyz(1), txyz(2), txyz(3));
end

% Rotate the decon result if requested
if ischar(bRotFinal)
    bRotFinal = logical(str2double(bRotFinal));
end

if bRotFinal
    if bReverse
        sign = -1;
    else
        sign = 1;
    end
    deconvolved = rotate3D(deconvolved, sign * rotateByAngle, ...
        dz_data*dz_data_ratio/xypixelsize);
end

if ischar(bFlipZ)
    bFlipZ = logical(str2double(bFlipZ));
end

if bFlipZ
    deconvolved = flip(deconvolved, 3);
end

% Z-crop the decon result if requested
if ischar(cropFinal)
    cropFinal = str2num(cropFinal);
end

if ~isempty(cropFinal)
    if cropFinal(2) > cropFinal(1)
        deconvolved = deconvolved(cropFinal(1):cropFinal(2),:,:);
    end
    if cropFinal(4) > cropFinal(3)
        deconvolved = deconvolved(:,cropFinal(3):cropFinal(4),:);
    end
    if cropFinal(6) > cropFinal(5)
        deconvolved = deconvolved(:,:,cropFinal(5):cropFinal(6));
    end
end
%toc

% construct output file name

decon_filename_tail = ['_decon',suffix];
decon_folder = 'matlab_decon/';
thumnail_folder = 'downsampled_data/';
MIPs_folder = 'MIPs/';

% output_tiff = strrep(input_tiff, '.tif', decon_filename_tail);
output_tiff = strcat(inputfile, decon_filename_tail);

if isempty(datafolder)
    output_tiff1 = strcat(decon_folder, output_tiff);
    output_tiff2 = strcat([decon_folder,thumnail_folder], output_tiff);
    MIPs_tiff = strcat([decon_folder,MIPs_folder], inputfile);
    if exist(decon_folder, 'dir') == 0
        mkdir(decon_folder);
    end
else
    output_tiff1 = strcat(datafolder, '/', decon_folder, output_tiff);
    output_tiff2 = strcat(datafolder, '/', [decon_folder, thumnail_folder], output_tiff);
    MIPs_tiff = strcat(datafolder, '/', [decon_folder, MIPs_folder], inputfile);
    if exist(strcat(datafolder, '/', decon_folder), 'dir') == 0
        mkdir(strcat(datafolder, '/', decon_folder));
    end
end

if ischar(bSaveUint16)
    bSaveUint16 = logical(str2double(bSaveUint16));
end

if bSaveUint16
    deconvolved = uint16(deconvolved);
end

write3Dtiff(deconvolved, output_tiff1);

% generate max-intensity projections if requested:
if ischar(axesMaxIntProj)
    axesMaxIntProj = str2num(axesMaxIntProj);
end

if length(axesMaxIntProj) ~= 3
    warning('Exactly 3 parameters are needed for -M')
else
    if any(axesMaxIntProj)
        mkdir([datafolder,'/',decon_folder,MIPs_folder]);
        axislabel = 'yxz';
        for d=1:3
            if axesMaxIntProj(d)
                write3Dtiff(max(deconvolved, [], d), strcat(MIPs_tiff, '_MIP_', axislabel(d), '.tif'))
            end
        end
    end
end

% generate thumbnails in "downsampled_data"
if ischar(resizeFactor)
    resizeFactor = str2double(resizeFactor);
end

if ischar(scalingThresh)
    scalingThresh = str2double(scalingThresh);
end

if resizeFactor ~= 1
    deconvolved = rescale_resample(deconvolved, resizeFactor, scalingThresh);
    mkdir([datafolder,'/',decon_folder,thumnail_folder]);
    write3Dtiff(deconvolved, output_tiff2);
end

end
