function [im,imD] = ReadTiffs(dirIn,datasetName)
    if (~exist('dirIn','var') || isempty(dirIn))
        root = uigetdir();
    else
        root = dirIn;
    end

    settingsListName = dir(fullfile(root,'*_Settings.txt'));
    if (length(settingsListName)>1)
        error('Multiple iterations is not implemented yet!');
    elseif (isempty(settingsListName))
        settingsListName = dir(fullfile(root,'..','*_Settings.txt'));
        if (isempty(settingsListName))
            [settingsListName,settingsListDir] = uigetfile(fullfile(root,'*.txt'),'Select settings file');
        else
            settingsListName = settingsListName.name;
            settingsListDir = fullfile(root,'..');
        end
    else
        settingsListName = settingsListName.name;
        settingsListDir = root;
    end

    imD = MicroscopeData.GetEmptyMetadata();
    
    metadata = LLSM.ParseSettingsFile(fullfile(settingsListDir,settingsListName));
    
    imD.StartCaptureDate = metadata.startCaptureDate;
    imD.ChannelNames = arrayfun(@(x)(num2str(x)),metadata.laserWaveLengths,'uniformoutput',false);

    if (length(imD.ChannelNames)==1)
        imD.ChannelColors = [1,1,1];
    else
        colrs = jet(7);
        imD.ChannelColors = colrs(round((metadata.laserWaveLengths-488)/300*7+1),:);
    end
    
    imList = dir(fullfile(root,'*.tif'));
        
    if (~exist('datasetName','var') || isempty(datasetName))
        [~,imD.DatasetName] = fileparts(root);
        iterPos = strfind(imList(1).name,'_Iter_');
        camPos = strfind(imList(1).name,'_Cam');
        chanPos = strfind(imList(1).name,'_ch');
        if (~isempty(iterPos))
            imD.DatasetName = [imD.DatasetName, '_', imList(1).name(1:iterPos-1)];
        elseif (~isempty(camPos))
            imD.DatasetName = [imD.DatasetName, '_', imList(1).name(1:camPos-1)];
        elseif (~isempty(chanPos))
            imD.DatasetName = [imD.DatasetName, '_', imList(1).name(1:chanPos-1)];
        end
    else
        [~,imD.DatasetName] = fileparts(root);
    end

    timeStampStr = regexp(imList(1).name,'(\d+)msecAbs','tokens');
    startTimeStamp = str2double(timeStampStr{1});
    imD.TimeStampDelta = 0;

    % find the number of channels for memory pre-allocation
    chansStr = regexp({imList.name},'_ch(\d)_','tokens');
    chans = cellfun(@(x)(str2double(x{1})),chansStr);
    numChans = max(chans) +1;

    % find the number of frames for memory pre-allocation
    framesStr = regexp({imList.name},'_stack(\d+)_','tokens');
    frames = cellfun(@(x)(str2double(x{1})),framesStr);
    numFrames = max(frames) +1;

    % pre-allocate memory
    im1 = LLSM.loadtiff(fullfile(root,imList(1).name));
    im = zeros([size(im1),numChans,numFrames],'like',im1);
    imD.TimeStampDelta = zeros(1,numChans,numFrames);
    imD.Position = zeros(1,numChans,numFrames,3);

    clear im1

    prgs = Utils.CmdlnProgress(length(imList),true,sprintf('Reading in %s',imD.DatasetName));
    for i=1:length(imList)
        curName = imList(i).name;

        % get the frame number
        tStr = regexp(curName,'_stack(\d+)_','tokens');
        t = str2double(tStr{1}) +1;

        % get the channel number
        chanStr = regexp(curName,'_ch(\d)_','tokens');
        c = str2double(chanStr{1}) +1;

        curIm = LLSM.loadtiff(fullfile(root,imList(i).name));

        curIm = squeeze(curIm);
        im(1:size(curIm,1),1:size(curIm,2),1:size(curIm,3),c,t) = curIm;

        % get the time stamp
        timeStampStr = regexp(curName,'(\d+)msecAbs','tokens');
        imD.TimeStampDelta(1,c,t) = str2double(timeStampStr{1}) - startTimeStamp;

%         % get stage position from orginal file
%         info = imfinfo(fullfile(root,imList(i).name));
%         posTemp = info(1).UnknownTags;
%         imD.Position(1,c,t,:) = [posTemp(1).Value, posTemp(2).Value, posTemp(3).Value];

        prgs.PrintProgress(i);
    end
    prgs.ClearProgress(true);

    % make metadata
    sz = size(im);
    imD.Dimensions = sz([2,1,3]);
    imD.NumberOfChannels = size(im,4);
    imD.NumberOfFrames = size(im,5);
    imD.PixelFormat = class(im);
    imD.PixelPhysicalSize = [0.104,0.104,metadata.zOffset*sin(31.8)];
end
