function [imMetadata] = GetMetadata(imageDir, textPath)
    
    if (~exist('textPath','var'))
        rootDir = fullfile(imageDir,'..');
        settingsName = dir(fullfile(rootDir,'*.txt'));
        textPath = fullfile(rootDir,settingsName(1).name);
    else
        [rootDir,~,~] = fileparts(textPath);
    end

    fileList = dir(imageDir);
    klb = false(length(fileList),1);
    tif = false(length(fileList),1);
    for i=1:length(fileList)
        klb(i) = ~isempty(strfind(fileList(i).name,'.klb'));
        tif(i) = ~isempty(strfind(fileList(i).name,'.tif'));
    end
    if (nnz(klb)>nnz(tif))
        klbs = fileList(klb);
        pixelFormat = MicroscopeData.Helper.GetPixelTypeKLB(fullfile(imageDir,klbs(1).name));
    else
        tifs = fileList(tif);
    	pixelFormat = MicroscopeData.Helper.GetPixelTypeTIF(fullfile(imageDir,tifs(1).name));
    end
    
    datasetName = LLSM.ParseSettingsFileNames(rootDir);
    metadata = LLSM.ParseSettingsFile(textPath);
    
    imMetadata = MicroscopeData.GetEmptyMetadata();
    imMetadata.DatasetName = datasetName;

    if (length(metadata.laserWaveLengths)==1)
        imMetadata.ChannelColors = [1,1,1];
    else
        colrs = jet(7);
        imMetadata.ChannelColors = colrs(round((metadata.laserWaveLengths-400)/300*7+1),:);
    end
    imMetadata.ChannelNames = arrayfun(@(x)(num2str(x)),metadata.laserWaveLengths,'uniformoutput',false);
    imMetadata.PixelPhysicalSize = [0.104, 0.104, metadata.zOffset];

    imMetadata.NumberOfChannels = metadata.numChan;
    imMetadata.NumberOfFrames = metadata.numStacks(1);
    imMetadata.StartCaptureDate = metadata.startCaptureDate;
    imMetadata.PixelFormat = pixelFormat;
end
    