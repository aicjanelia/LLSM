function ConvertLLStiffs(dirIn,datasetName,dirOut)
% ConvertLLStiffs(dirIn,datasetName,dirOut)
% Convert tif files from the LLSM into the H5 format for Eric's utilities

    llsmTic = tic;
    subfolders = {'Deskewed';'CPPdecon'};

    if (~exist('dirIn','var') || isempty(dirIn))
        root = uigetdir();
    else
        root = dirIn;
    end

    if (~exist('dirOut','var') || isempty(dirOut))
        dirOut = root;
    end

    settingsList = dir(fullfile(root,'*_Settings.txt'));
    if (length(settingsList)>1)
        error('Multiple iterations is not implemented yet!');
    end

    imD = MicroscopeData.GetEmptyMetadata();

    metadataStr = fileread(fullfile(root,settingsList.name));
    dateStr = regexp(metadataStr,'(\d+)/(\d+)/(\d+) (\d+):(\d+):(\d+) ([AP]M)','tokens');
    month = str2double(dateStr{1,1}{1,1});
    day = str2double(dateStr{1,1}{1,2});
    year = str2double(dateStr{1,1}{1,3});
    hour = str2double(dateStr{1,1}{1,4});
    if (strcmpi(dateStr{1,1}{1,7},'PM'))
        hour = hour +12;
    end
    minute = str2double(dateStr{1,1}{1,5});
    second = str2double(dateStr{1,1}{1,6});

    imD.StartCaptureDate = sprintf('%d-%d-%d %02d:%02d:%02d',year,month,day,hour,minute,second);

    zOffsetStr = regexp(metadataStr,'S PZT.*Excitation \(0\) :\t\d+\t(\d+).(\d+)','tokens');
    zOffset = str2double([zOffsetStr{1,1}{1,1}, '.', zOffsetStr{1,1}{1,2}]);

    laserWavelengthStr = regexp(metadataStr,'Excitation Filter, Laser, Power \(%\), Exp\(ms\) \((\d+)\) :\tN/A\t(\d+)','tokens');
    laserWaveLengths = zeros(length(laserWavelengthStr),1);
    for i=1:length(laserWavelengthStr)
        laserWaveLengths(str2double(laserWavelengthStr{1,i}{1,1})+1) = str2double(laserWavelengthStr{1,i}{1,2});
    end
    imD.ChannelNames = arrayfun(@(x)(num2str(x)),laserWaveLengths,'uniformoutput',false);

    if (length(laserWavelengthStr)==1)
        imD.ChannelColors = [1,1,1];
    else
        colrs = jet(7);
        imD.ChannelColors = colrs(round((laserWaveLengths-488)/300*7+1),:);
    end
    
    for s = 1:length(subfolders)
        if (~exist(fullfile(root,subfolders{s}),'dir'))
            warning('Cannot find %s.\nSkipping\n',fullfile(root,subfolders{s}));
            continue
        end
        imList = dir(fullfile(root,subfolders{s},'*.tif'));
        if (isempty(imList))
            imList = dir(fullfile(root,subfolders{s},'*.bz2'));
        end
        
        if (~exist('datasetName','var') || isempty(datasetName))
            iterPos = strfind(imList(1).name,'_Iter_');
            camPos = strfind(imList(1).name,'_Cam');
            chanPos = strfind(imList(1).name,'_ch');
            if (~isempty(iterPos))
                imD.DatasetName = imList(1).name(1:iterPos-1);
            elseif (~isempty(camPos))
                imD.DatasetName = imList(1).name(1:camPos-1);
            elseif (~isempty(chanPos))
                imD.DatasetName = imList(1).name(1:chanPos-1);
            end
        else
            [~,imD.DatasetName] = fileparts(root);
        end
        
        imD.DatasetName = [imD.DatasetName, '_', subfolders{s}];

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
        [~,~,ext] = fileparts(imList(1).name);
        if (strcmp(ext,'.tif'))
            im1 = LLSM.loadtiff(fullfile(root,subfolders{s},imList(1).name));
        else
            im1 = LLSM.ReadCompressedIm(fullfile(root,subfolders{s},imList(1).name));
        end
        im = zeros([size(im1),numChans,numFrames],'like',im1);
        imD.TimeStampDelta = zeros(1,numChans,numFrames);
        imD.Position = zeros(1,numChans,numFrames,3);

        clear im1

        prgs = Utils.CmdlnProgress(length(imList),true,sprintf('Reading in %s',subfolders{s}));
        for i=1:length(imList)
            curName = imList(i).name;

            % get the frame number
            tStr = regexp(curName,'_stack(\d+)_','tokens');
            t = str2double(tStr{1}) +1;

            % get the channel number
            chanStr = regexp(curName,'_ch(\d)_','tokens');
            c = str2double(chanStr{1}) +1;
            
            if (strcmp(ext,'.tif'))
                curIm = LLSM.loadtiff(fullfile(root,subfolders{s},imList(i).name));
            else
                curIm = LLSM.ReadCompressedIm(fullfile(root,subfolders{s},imList(i).name));
            end

            curIm = squeeze(curIm);
            im(1:size(curIm,1),1:size(curIm,2),1:size(curIm,3),c,t) = curIm;

            % get the time stamp
            timeStampStr = regexp(curName,'(\d+)msecAbs','tokens');
            imD.TimeStampDelta(1,c,t) = str2double(timeStampStr{1}) - startTimeStamp;
            
            % get stage position from orginal file
            underscorePos = strfind(imList(i).name,'_');
            orgFileName = imList(i).name;
            orgFileName = [orgFileName(1:underscorePos(end)-1),'.tif'];
            
            if (strcmp(ext,'.tif'))
                info = imfinfo(fullfile(root,orgFileName));
            else
                orgFileName = [orgFileName,'.bz2'];
                info = LLSM.ReadCompressedImageInfo(fullfile(root,orgFileName));
            end
                
            posTemp = info(1).UnknownTags;
            imD.Position(1,c,t,:) = [posTemp(1).Value, posTemp(2).Value, posTemp(3).Value];

            prgs.PrintProgress(i);
        end
        prgs.ClearProgress(true);

        % make metadata for HDF5 file
        sz = size(im);
        imD.Dimensions = sz([2,1,3]);
        imD.NumberOfChannels = size(im,4);
        imD.NumberOfFrames = size(im,5);
        imD.PixelFormat = class(im);
        imD.PixelPhysicalSize = [0.104,0.104,zOffset*sin(31.8)];

        % write out KLB file
        MicroscopeData.WriterKLB(im,'path',dirOut,'imageData',imD,'verbose',true);
    end
    
    fprintf('Total conversion time was %s\n',Utils.PrintTime(toc(llsmTic)))
end
