function [imMetadata,dirNamesKLB] = GetRAWmetadata(root)
    datasetName = LLSM.ParseSettingsFileNames(root);
    settingsName = dir(fullfile(root,'*.txt'));
    metadata = LLSM.ParseSettingsFile(fullfile(root,settingsName(1).name));

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

    dirList = dir(fullfile(root));
    dirMask = [dirList.isdir];
    dirNamesKLB = {dirList(dirMask).name};
    klbDirMask = cellfun(@(x)(~isempty(x)),regexpi(dirNamesKLB,'KLB'));
    dirNamesKLB = dirNamesKLB(klbDirMask);

    imMetadata.NumberOfChannels = metadata.numChan;
    imMetadata.NumberOfFrames = metadata.numStacks(1);
    imMetadata.StartCaptureDate = metadata.startCaptureDate;
    imMetadata = rmfield(imMetadata,'PixelFormat');
end
    