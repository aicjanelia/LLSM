function [metadata, names] = GetMetadataFromFileStruct(imageDir,imageExt,textPath)
    if (~exist('imageExt','var') || isempty(imageExt))
        imageExt = 'tif';
    end

    metadata = MicroscopeData.ReadMetadata(imageDir,false);
    writeJson = false;
    if (isempty(metadata))
        if (~exist('textPath','var') || isempty(textPath))
            metadata = LLSM.GetRAWmetadata(fullfile(imageDir,'..'));
            if (isempty(metadata))
                error('Cannot find metadata file, json or text!');
            end
        end
        metadata = LLSM.GetRAWmetadata(textPath);
        writeJson = true;
    end

    dList = dir(fullfile(imageDir,['*.',imageExt]));
    fileNames = {dList.name};
    [~,chans,cams,stacks,iter,wavelengths,secs] = LLSM.ParseFileNames(fileNames);

    [metadata.NumberOfFrames, useStacks] = LLSM.GetNumberOfFrames(iter,stacks);
    [metadata.NumberOfChannels, metadata.ChannelNames, wavelengths, useCams, uniqueCams, uniqueChans] = LLSM.GetChannelData(cams,chans,wavelengths);
    metadata.ChannelColors = Utils.GetColorByWavelength(wavelengths);
    metadata.TimeStampDelta = MicroscopeData.GetOrderedTimeStamps(secs,metadata.NumberOfFrames,metadata.NumberOfChannels);
    metadata.imageDir = imageDir;
    
    names.fileNames = fileNames;
    names.chans = chans;
    names.uniqueChans = uniqueChans;
    names.cams = cams;
    names.uniqueCams = uniqueCams;
    names.stacks = stacks;
    names.iter = iter;
    names.wavelength = wavelengths;
    names.sec = secs;
    names.useStacks = useStacks;
    names.useCams = useCams;
    
    if (writeJson)
        im = LLSM.ReadImage(metadata, names, 1, 1);
        metadata.Dimensions = Utils.SwapXY_RC(size(im));
        
        MicroscopeData.CreateMetadata(imageDir,metadata);
    end
end
