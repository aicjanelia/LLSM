%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% User Set
% root = 'F:\LLS\Ehret\20170613_Ehret\TimeLapse1_Pos1_581\CPPdecon';
% datasetName = 'TimeLapse1_Pos1_581_decon';
% 
% outDir = root;
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function ConvertLLStiffs(dirIn,datasetName,dirOut)

    if (~exist('dirIn','var') || isempty(dirIn))
        root = uigetdir();
    else
        root = dirIn;
    end
    
    if (~exist('datasetName','var') || isempty(datasetName))
        [~,datasetName] = fileparts(root);
    end

    if (~exist('dirOut','var') || isempty(dirOut))
        dirOut = root;
    end

    imList = dir(fullfile(root,'*.tif'));

    % find the number of channels for memory pre-allocation
    chansStr = regexp({imList.name},'ch(\d)_','tokens');
    chans = cellfun(@(x)(str2double(x{1})),chansStr);
    numChans = max(chans) +1;

    % find the number of frames for memory pre-allocation
    framesStr = regexp({imList.name},'z_(\d+)t','tokens');
    frames = cellfun(@(x)(str2double(x{1})),framesStr);
    numFrames = max(frames) +1;

    % pre-allocate memory
    im1 = squeeze(MicroscopeData.Original.ReadData(root,imList(1).name));
    im = zeros([size(im1),numChans,numFrames],'like',im1);
    clear im1

    prgs = Utils.CmdlnProgress(length(imList),false,'Reading in orginals');
    for i=1:length(imList)
        curName = imList(i).name;

        % get the frame number
        tStr = regexp(curName,'_\d+t','match');
        t = str2double(tStr{1}(2:5)) +1;

        % get the time stamp
        timeStampStr = regexp(curName,'\d+msecAbs','match');
        timeStamp = str2double(timeStampStr{1}(1:10));

        % get the channel number
        chanWaveStr = regexp(curName,'\d+nm','match');
        chanWave = str2double(chanWaveStr{1}(1:end-2));
        chanWaves = regexp(curName,'(\d+)_(\d+)mW','tokens');
        c = find(strcmpi(vertcat(chanWaves{:}),num2str(chanWave)));

        curIm = MicroscopeData.Original.ReadData(root,imList(i).name);

        curIm = squeeze(curIm);
        im(1:size(curIm,1),1:size(curIm,2),1:size(curIm,3),c,t) = curIm;

        prgs.PrintProgress(i);
    end
    prgs.ClearProgress(true);

    % make metadata for HDF5 file
    deltaZ = 0.25;
    imD = MicroscopeData.GetEmptyMetadata();
    sz = size(im);
    imD.Dimensions = sz([2,1,3]);
    imD.DatasetName = datasetName;
    imD.NumberOfChannels = size(im,4);
    imD.NumberOfFrames = size(im,5);
    imD.ChannelNames = regexp(imList(1).name,'\d+_\d+mW','match')';
    imD.PixelFormat = class(im);
    imD.PixelPhysicalSize = [0.104,0.104,deltaZ*sin(31.8)];
    imD = rmfield(imD,'ChannelColors');

    % write out HDF5 file
    MicroscopeData.WriterH5(im,'path',dirOut,'imageData',imD,'verbose',true);
end
