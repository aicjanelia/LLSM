function [metadata, names] = GetMetadataFromFileStruct(imageDir,imageExt,textPath)
    if (~exist('imageExt','var') || isempty(imageExt))
        fileList = dir(imageDir);
        klb = false(length(fileList),1);
        tif = false(length(fileList),1);
        for i=1:length(fileList)
            klb(i) = ~isempty(strfind(fileList(i).name,'.klb'));
            tif(i) = ~isempty(strfind(fileList(i).name,'.tif'));
        end
        if (nnz(klb)>nnz(tif))
            imageExt = 'klb';
        else
        imageExt = 'tif';
        end
    end

    metadata = MicroscopeData.ReadMetadata(imageDir,false);
    writeJson = false;
    if (isempty(metadata))
        if (~exist('textPath','var') || isempty(textPath))
            metadata = LLSM.GetMetadata(imageDir);
            if (isempty(metadata))
                error('Cannot find metadata file, json or text!');
            end
        else
            metadata = LLSM.GetMetadata(imageDir,textPath);
        end
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
        im = LLSM.ReadImageOrgName(metadata, names, 1, 1);
        metadata.Dimensions = Utils.SwapXY_RC(size(im));
        
        MicroscopeData.CreateMetadata(imageDir,metadata);
    end
end
