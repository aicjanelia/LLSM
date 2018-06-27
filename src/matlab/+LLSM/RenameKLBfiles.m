function RenameKLBfiles(root)
    datasetName = LLSM.ParseSettingsFileNames(root);
    settingsName = dir(fullfile(root,'*.txt'));
    metadata = LLSM.ParseSettingsFile(fullfile(root,settingsName.name));

    imD = MicroscopeData.GetEmptyMetadata();
    imD.DatasetName = datasetName;

    if (length(metadata.laserWaveLengths)==1)
        imD.ChannelColors = [1,1,1];
    else
        colrs = jet(7);
        imD.ChannelColors = colrs(round((metadata.laserWaveLengths-400)/300*7+1),:);
    end
    imD.ChannelNames = arrayfun(@(x)(num2str(x)),metadata.laserWaveLengths,'uniformoutput',false);
    imD.PixelPhysicalSize = [0.104, 0.104, metadata.zOffset];

    dirList = dir(fullfile(root));
    dirMask = [dirList.isdir];
    dirNames = {dirList(dirMask).name};
    klbDirMask = cellfun(@(x)(~isempty(x)),regexpi(dirNames,'KLB'));
    dirNames = dirNames(klbDirMask);

    imD.NumberOfChannels = metadata.numChan;
    imD.NumberOfFrames = metadata.numStacks(1);
    imD.StartCaptureDate = metadata.startCaptureDate;
    imD = rmfield(imD,'PixelFormat');

    for i = 1:length(dirNames)
        subDir = dirNames{i};
        disp(subDir);
        suffix = '';
        if (~isempty(regexpi(subDir,'decon')))
            suffix = '_decon';
        elseif (~isempty(regexpi(subDir,'deskewed')))
            suffix = '_deskewed';
        end
        tempD = imD;
        tempD.DatasetName = [tempD.DatasetName,suffix];
        curFiles = dir(fullfile(root,subDir,'*.klb'));
        tempIm = MicroscopeData.KLB.readKLBheader(fullfile(root,subDir,curFiles(1).name));
        tempD.Dimensions = tempIm.xyzct([2,1,3]);
        
        curFileNames = {curFiles.name};
        curMask = cellfun(@(x)(~isempty(x)),regexpi(curFileNames,'\.klb'));
        curFileNames = {curFiles(curMask).name}';
        curFileNames = regexpi(curFileNames,'(.*).klb','tokens');
        curFileNames = cellfun(@(x)(x{:}),curFileNames);
        [datasetName,chans,cams,stacks,wavelengths,secs,fileSuffixs] = LLSM.ParseFileNames(curFileNames);

        prgs = Utils.CmdlnProgress(imD.NumberOfFrames,true,'Renaming');
        for t=1:imD.NumberOfFrames
            timeMask = stacks==t-1;
            for c=1:imD.NumberOfChannels
                chanMask = chans==c-1;
                inName = [curFileNames{timeMask & chanMask}];
                outName = sprintf('%s%s_c%d_t%04d',imD.DatasetName,suffix,c,t);
                if (exist(fullfile(root,subDir,[inName,'.klb']),'file'))
                    movefile(fullfile(root,subDir,[inName,'.klb']),fullfile(root,subDir,[outName,'.klb']));
                end
            end
            prgs.PrintProgress(t);
        end

        MicroscopeData.CreateMetadata(fullfile(root,subDir),tempD);
        prgs.ClearProgress(true);
    end
end
