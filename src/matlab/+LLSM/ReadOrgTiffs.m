function [im,imD] = ReadOrgTiffs(dirIn,subfolder,dirOut)
% ConvertLLStiffs(dirIn,datasetName,dirOut)
% Convert tif files from the LLSM into the H5 format for Eric's utilities
    if (~exist('dirIn','var') || isempty(dirIn))
        root = uigetdir();
    else
        root = dirIn;
    end
    
    if (~exist('dirOut','var'))
        dirOut = [];
    else
        dirOut = fullfile(dirOut,[subfolder,'KLB']);
    end
    
    if (~isempty(dirOut) && ~exist(dirOut,'dir'))
        mkdir(dirOut);
    end        
    
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % TODO: this is a hack until we have more than one camera
    camNums = [1];
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    [datasetName,iterNumbers] = LLSM.ParseSettingsFileNames(root);
    settingsList = dir(fullfile(root,'*_Settings.txt'));
    firstMetaSettings = LLSM.ParseSettingsFile(fullfile(root,settingsList(1).name));
    tifList = dir(fullfile(root,subfolder,'*.tif'));
    firstIm = LLSM.loadtiff(fullfile(root,subfolder,tifList(1).name));

    fullImageSize = [size(firstIm),firstMetaSettings.numChan,max(iterNumbers+1)*max(firstMetaSettings.numStacks)];
    if (nargout>0)
        im = zeros(fullImageSize,'uint16');
    end
    clear firstIm

    imD = MicroscopeData.GetEmptyMetadata();
    imD.DatasetName = [datasetName,'_',subfolder];
    if (length(firstMetaSettings.laserWaveLengths)==1)
        imD.ChannelColors = [1,1,1];
    else
        colrs = jet(7);
        imD.ChannelColors = colrs(round((firstMetaSettings.laserWaveLengths-488)/300*7+1),:);
    end
    imD.ChannelNames = arrayfun(@(x)(num2str(x)),firstMetaSettings.laserWaveLengths,'uniformoutput',false);
    imD.PixelPhysicalSize = [0.104, 0.104, firstMetaSettings.zOffset];
    
    prgs = Utils.CmdlnProgress(prod(fullImageSize(4:5)),true,sprintf('Reading %s',imD.DatasetName));
    p = 0;
    %%%%%%%%%%%%%
    % the metadata is the maximum possible frames not actual
    %%%%%%%%%%%%%
    maxT = 0;
    maxC = 0;
    for itr = 1:length(iterNumbers)
        % Get the settings file info for this image
        if (length(iterNumbers)==1)
            fileName = [datasetName,'_Settings.txt'];
            searchStr = '*';
        else
            fileName = sprintf('%s_Iter_%04d_Settings.txt',datasetName,iterNumbers(itr));
            searchStr = sprintf('*_Iter_%04d_*',iterNumbers(itr));
        end
        metaSettings = LLSM.ParseSettingsFile(fullfile(root,fileName));
        
        % fill in metadata
        if itr==1
            imD.StartCaptureDate = metaSettings.startCaptureDate;
        end
        
        imList = dir(fullfile(root,subfolder,[searchStr,'.tif']));
        if (isempty(imList))
            imList = dir(fullfile(root,subfolder,[searchStr,'.bz2']));
        end
        
        for i = 1:length(imList)
            fName = imList(i).name;
            [~,~,ext] = fileparts(fName);
            metaFile = LLSM.GetMetadataFromFileName(fName);
            
            chan = metaFile.Channel+1;
            if (maxC<chan)
                maxC = chan;
            end
            
            frame = (metaFile.Stack+1)+(itr-1)*metaSettings.numStacks(chan);
            if (maxT<frame)
                maxT = frame;
            end
            
            orgFileTokens = regexpi(fName,'(.*)_(\d+)t.*\.(.*)','tokens');
            orgFileName = [orgFileTokens{1,1}{1,1},'_',orgFileTokens{1,1}{1,2},'t.',orgFileTokens{1,1}{1,3}];
            if (strcmp(ext,'.tif'))
                tempIm = LLSM.loadtiff(fullfile(root,subfolder,fName));
                if (any(size(tempIm)~=fullImageSize(1:3)))
                    warning(sprintf('Wrong image size chan:%d frame:%d',chan,frame));
                    continue
                end
                if (nargout>0)
                    im(:,:,:,chan,frame) = tempIm;
                end
                info = imfinfo(fullfile(root,orgFileName));
            else
                tempIm = LLSM.ReadCompressedIm(fullfile(root,subfolder,fName));
                if (nargout>0)
                    im(:,:,:,chan,frame) = tempIm;
                end
                info = LLSM.ReadCompressedImageInfo(fullfile(root,orgFileName));                
            end
            
            if (~isempty(dirOut))
                %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                % Hack!
                % This needs to be changed so that each frame can be
                % written out to a single klb file (not each channel too!)
                %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                MicroscopeData.KLB_(tempIm,imD.DatasetName,dirOut,imD.PixelPhysicalSize,chan,[frame,frame],true,true,false);
            end
            
            try
                posTemp = info(1).UnknownTags;
                imD.Position(chan,frame,:) = [posTemp(1).Value, posTemp(2).Value, posTemp(3).Value];
            catch err
                imD.Position(chan,frame,:) = [0,0,0];
            end
            
            imD.TimeStampDelta(chan,frame) = metaFile.DeltaMSec;
            
            p = p +1;
            prgs.PrintProgress(p);
        end
    end
    prgs.ClearProgress(true);

    imD.Dimensions = fullImageSize([2,1,3]);
    %%%%%%%%%%%%%
    % the metadata is the maximum possible frames not actual
    %%%%%%%%%%%%%
    imD.NumberOfChannels = maxC;
    imD.NumberOfFrames = maxT;
    if (nargout>0)
        im = im(:,:,:,1:maxC,1:maxT);
    end
    
    imD.PixelFormat = class(tempIm);
    
    if (~isempty(dirOut))
        MicroscopeData.CreateMetadata(dirOut,imD);
        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        % Hack!
        % This needs to be changed so that each frame can be
        % written out to a single klb file (not each channel too!)
        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        chans = [1:imD.NumberOfChannels];
        prgs = Utils.CmdlnProgress(imD.NumberOfFrames,true,'Combining channels');
        for t=1:imD.NumberOfFrames
            tempIm = MicroscopeData.Reader(dirOut,'timeRange',[t,t]);
            MicroscopeData.KLB_(tempIm,imD.DatasetName,dirOut,imD.PixelPhysicalSize,chans,[t,t],true,false,false);
            
            prgs.PrintProgress(t);
        end
        prgs.ClearProgress(true);
        
        dList = dir(fullfile(dirOut,[imD.DatasetName,'_c*_t*.klb']));
        for i=1:length(dList)
            delete(fullfile(dirOut,dList(i).name));
        end
    end
end
