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

    tifNames = {dirList.name};
    tifMask = cellfun(@(x)(~isempty(x)),regexpi(tifNames,'\.tif'));
    tifNames = {dirList(tifMask).name}';
    tifNames = regexpi(tifNames,'(.*).tif','tokens');
    tifNames = cellfun(@(x)(x{:}),tifNames);

%     datasetNameSuffix = '_ch';
    chanPrefix = 'ch';
%     camsPrefix = 'CAM';
    stacksPrefix = 'stack';
%     wavelengthSuffix = 'nm';
%     secsSuffix = 'msec';

%     datasetName = regexpi(tifNames,['^(\w+)',datasetNameSuffix],'tokens');
%     datasetName = vertcat(datasetName{:});

    chans = regexpi(tifNames,[chanPrefix,'(\d)'],'tokens');
    chans = cellfun(@(x)(str2double(x{:})),chans)';

%     cams = regexpi(tifNames,[camsPrefix,'(\d)'],'tokens');
%     cams = cellfun(@(x)(str2double(x{:})),cams)';

    stacks = regexpi(tifNames,[stacksPrefix,'(\d+)'],'tokens');
    stacks = cellfun(@(x)(str2double(x{:})),stacks)';

%     wavelengths = regexpi(tifNames,['(\d+)',wavelengthSuffix],'tokens');
%     wavelengths = cellfun(@(x)(str2double(x{:})),wavelengths)';

%     secs = regexpi(tifNames,['(\d+)',secsSuffix,'_'],'tokens');
%     secs = cellfun(@(x)(str2double(x{:})),secs)';

%     fileSuffixs = regexpi(tifNames,'msec_(.*)','tokens');
%     fileSuffixs = cellfun(@(x)(x{:}),fileSuffixs);

    imD.NumberOfChannels = length(unique(chans));
    imD.NumberOfFrames = length(unique(stacks));

    im = LLSM.loadtiff(fullfile(root,[tifNames{1},'.tif']));
    w = whos('im');
    imD.PixelFormat = w.class;

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
        firstFile = dir(fullfile(root,subDir,'*.klb'));
        tempIm = MicroscopeData.KLB.readKLBheader(fullfile(root,subDir,firstFile(1).name));
        tempD.Dimensions = tempIm.xyzct(1:3);

        prgs = Utils.CmdlnProgress(imD.NumberOfFrames,true,'Renaming');
        for t=1:imD.NumberOfFrames
            timeMask = stacks==t-1;
            for c=1:imD.NumberOfChannels
                chanMask = chans==c-1;
                inName = [tifNames{timeMask & chanMask},suffix];
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
