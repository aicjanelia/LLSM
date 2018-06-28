function MakeMIPmovie(root,subPath)
    % root = 'D:\Images\LLSM\Pfisterer';
    % subPath = '20171108_Pfisterer\TimeLapse3\CPPdecon\MIPs';
    seps = regexpi(subPath,'[/\\]');
    outName = subPath;
    outName(seps) = '_';
    moviePathName = fullfile(root,[outName,'.mp4']);
    if (exist(moviePathName,'file'))
        %return
    end

    colors = single([0,1,0;1,0,1;1,1,0;0,1,1]);

    tiffList = dir(fullfile(root,subPath,'*.tif'));
    if (isempty(tiffList))
        warning('No tif files found in %s',fullfile(root,subPath));
        return
    end
    try
    [~,chans,~,stacks] = LLSM.ParseFileNames(tiffList,'tif');
    catch
        warning('Could not parse names from %s',fullfile(root,subPath));
        return
    end
    numFrames = max(stacks(:))+1;
    numChans = max(chans(:))+1;
    
    if (numFrames < 30)
        return
    end

    colorMultiplier = zeros(1,1,3,length(numChans),'single');
    for c=1:numChans
        colorMultiplier(1,1,:,c) = colors(c,:);
    end

    frameDir = fullfile(root,subPath,outName);
    if (exist(frameDir,'dir'))
        rmdir(frameDir,'s');
    end
    mkdir(frameDir);

    prgs = Utils.CmdlnProgress(numFrames,true,outName);
    for t=1:numFrames
        timeMask = stacks==t-1;
        chanMask = chans==0;
        i = timeMask & chanMask;
        try
            im = imread(fullfile(root,subPath,tiffList(i).name));

            imColors = zeros(size(im,1),size(im,2),3,numChans,'single');
            imIntensity = zeros(size(im,1),size(im,2),numChans,'single');
            for c=1:numChans
                chanMask = chans==c-1;
                i = timeMask & chanMask;
                im = imread(fullfile(root,subPath,tiffList(i).name));
                im = mat2gray(im);
                im = ImUtils.BrightenImages(im);
                im = mat2gray(im);
                imIntensity(:,:,c) = im;
                color = repmat(colorMultiplier(1,1,:,c),size(im,1),size(im,2),1);
                imColors(:,:,:,c) = repmat(im,1,1,3).*color;
            end

            imMax = max(imIntensity,[],3);
            imIntSum = sum(imIntensity,3);
            imIntSum(imIntSum==0) = 1;
            imColrSum = sum(imColors,4);
            imFinal = imColrSum.*repmat(imMax./imIntSum,1,1,3);
            imFinal = im2uint8(imFinal);
            
            sizeEven = size(imFinal)/2;
            sizeEven = sizeEven([1,2]);
            sizeEven = sizeEven~=round(sizeEven);
            imFinal(end:end+sizeEven(1),end:end+sizeEven(2),:) = 0;
            
            sz = size(imFinal);
            if (sz(1)>sz(2))
                imFinal = imrotate(imFinal,90);
            end

            imwrite(imFinal,fullfile(frameDir,sprintf('%04d.tif',t)));
        catch
        end
        prgs.PrintProgress(t);
    end

    fps = min(60,numFrames/10);
    fps = max(fps,7);
    fps = round(fps);
    
    MovieUtils.MakeMP4_ffmpeg(1,numFrames,frameDir,fps);
    copyfile(fullfile(frameDir,[outName,'.mp4']),fullfile(root,'.'));
    rmdir(frameDir,'s');

    prgs.ClearProgress(true);
end

