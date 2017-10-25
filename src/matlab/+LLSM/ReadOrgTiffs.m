function [im,imD] = ReadOrgTiffs(dirIn,subfolder)
% ConvertLLStiffs(dirIn,datasetName,dirOut)
% Convert tif files from the LLSM into the H5 format for Eric's utilities
    if (~exist('dirIn','var') || isempty(dirIn))
        root = uigetdir();
    else
        root = dirIn;
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

    im = zeros([size(firstIm),firstMetaSettings.numChan,max(iterNumbers+1)*max(firstMetaSettings.numStacks)],'uint16');
    eachFileSize = size(im);
    eachFileSize = eachFileSize(1:3);
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
    
    prgs = Utils.CmdlnProgress(size(im,4)*size(im,5),true,sprintf('Reading %s',imD.DatasetName));
    p = 0;
    for itr = 1:length(iterNumbers)
        % Get the settings file info for this image
        if (length(iterNumbers)==1)
            fileName = [datasetName,'.txt'];
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
            frame = (metaFile.Stack+1)+(itr-1)*metaSettings.numStacks(chan);
            
            orgFileTokens = regexpi(fName,'(.*)_(\d+)t.*\.(.*)','tokens');
            orgFileName = [orgFileTokens{1,1}{1,1},'_',orgFileTokens{1,1}{1,2},'t.',orgFileTokens{1,1}{1,3}];
            if (strcmp(ext,'.tif'))
                tempIm = LLSM.loadtiff(fullfile(root,subfolder,fName));
                if (any(size(tempIm)~=eachFileSize))
                    warning(sprintf('Wrong image size chan:%d frame:%d',chan,frame));
                    continue
                end
                im(:,:,:,chan,frame) = tempIm;
                info = imfinfo(fullfile(root,orgFileName));
            else
                im(:,:,:,chan,frame) = LLSM.ReadCompressedIm(fullfile(root,subfolder,fName));
                info = LLSM.ReadCompressedImageInfo(fullfile(root,orgFileName));                
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

    sz = size(im);
    imD.Dimensions = sz([2,1,3]);
    imD.NumberOfChannels = size(im,4);
    imD.NumberOfFrames = size(im,5);
    imD.PixelFormat = class(im);
end
