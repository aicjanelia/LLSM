function MakeMIPmovie(root,subPath,overwrite)
    minFrames = 10;
    
    if (~exist('overwrite','var') || isempty(overwrite))
        overwrite = false;
    end

    frameDir = fullfile(root,subPath,'movieFrames');
    if (~overwrite && exist(frameDir,'dir'))
        dList = dir(fullfile(frameDir,'*.tif'));
        if (~isempty(dList))
            return
        end
    end

    dList = dir(fullfile(root,subPath));
    dList = dList(3:end);
    dList = dList([dList.isdir]);
    
    dirMask = cellfun(@(x)(~isempty(x)),strfind({dList.name}','CPPdeconKLB'));
    if (~any(dirMask))
        dirMask = cellfun(@(x)(~isempty(x)),strfind({dList.name}','DeskewedKLB'));
        if (~any(dirMask))
            warning('Cannot find KLB diretories in: %s',fullfile(root,subPath));
            return
        end
    end
    
    klbDir = dList(dirMask).name;
    imageList = dir(fullfile(root,subPath,klbDir,'*.klb'));
    if (length(imageList) < minFrames)
        return
    end
    
    [datasetNames,chans,cams,stacks,iter] = LLSM.ParseFileNames(imageList,'klb');

    outName = fullfile(subPath,klbDir);
    klbEnd = strfind(subPath,'KLB');
    if (~isempty(klbEnd))
        outName = outName(1:klbEnd-1);
    end
    mipEnd = strfind(subPath,'\MIPs');
    if (~isempty(mipEnd))
        outName = outName(1:mipEnd-1);
    end
    seps = regexpi(outName,'[/\\]');
    outName(seps) = '_';
    moviePathName = fullfile(root,[outName,'.mp4']);
    if (exist(moviePathName,'file'))
        return
    end
    
    if (isempty(chans) || (isempty(stacks) && isempty(iter)))
        return
    end
    
    settingsName = dir(fullfile(root,subPath,'*.txt'));
    if (isempty(settingsName))
        settingsName = dir(fullfile(root,subPath,'..','..','*.txt'));
        if (isempty(settingsName))
            warning('Cannot find settings from path: %s',fullfile(root,subPath));
            return
        end
    end
    
    metadata = LLSM.ParseSettingsFile(fullfile(settingsName(1).folder,settingsName(1).name));
    xyPhysicalSize = 0.104;
    zPhysicalSize = metadata.zOffset;
    
    if (isempty(iter))
        numFrames = max(stacks(:))+1;
    else
        if (isempty(stacks))
            numFrames = max(iter(:))+1;
        else
            numFrames = max(stacks(:))+1 * max(iter(:))+1;
        end
    end
    
    channels = struct('cam',[],'chan',[]);  
    if (~isempty(cams))
        useCams = true;
        unqCams = unique(cams);
        unqChns = unique(chans);
        c = 1;
        for cm = 1:length(unqCams)
            for ch = 1:length(unqChns)
                camChanMask = cellfun(@(x)(x==unqCams{cm}),cams) & chans==unqChns(ch);
                if (any(camChanMask))
                    channels(c).cam = unqCams{cm};
                    channels(c).chan = unqChns(ch) +1;
                    c = c +1;
                end
            end
        end
        numChans = c-1;
    else
        useCams = false;
        numChans = max(chans(:))+1;
        for i=1:numChans
            channels(i).cam = '';
            channels(i).chan = i;
        end
    end
    
    if (numFrames < minFrames)
        return
    end

    colors = single([0,1,0;1,0,1;1,1,0;0,1,1]);
    if (numChans==1)
        colors = [1,1,1];
    end
    
    if (exist(frameDir,'dir'))
        rmdir(frameDir,'s');
    end
    mkdir(frameDir);

    prgs = Utils.CmdlnProgress(numFrames,true,outName);
    prgs.PrintProgress(0);
    parfor t=1:numFrames   
        try
            fName = LLSM.GetFileName(fullfile(root,subPath,klbDir),channels(1).cam,t,channels(1).chan);
            im = MicroscopeData.KLB.readKLBstack(fName{1});
            
            imIntensity = zeros(size(im,1),size(im,2),size(im,3),numChans,'single');
            for c=1:numChans
                fName = LLSM.GetFileName(fullfile(root,subPath,klbDir),channels(c).cam,t,channels(c).chan);
                imIntensity(:,:,:,c) = MicroscopeData.KLB.readKLBstack(fName{end});
            end
            
            imFinal = ImUtils.MakeOrthoSliceProjections(imIntensity,colors(1:numChans,:),xyPhysicalSize,zPhysicalSize);
            
            sz = size(imFinal);
            if (sz(1)>sz(2))
                imFinal = imrotate(imFinal,90);
            end
            imFinal = imresize(imFinal,[1080,NaN]);
            imFinal = ImUtils.MakeImageXYDimEven(imFinal);

            imwrite(imFinal,fullfile(frameDir,sprintf('%s_%04d.tif',outName,t)));
        catch err
            warning(err.message);
            prgs.ClearProgress(false);
            numFrames = t-1;
        end
        prgs.PrintProgress(t);
    end

    fps = min(60,numFrames/10);
    fps = max(fps,7);
    fps = round(fps);
    
    
    MovieUtils.MakeMP4_ffmpeg(1,numFrames,frameDir,fps,[outName,'_']);
    
    if (~exist(fullfile(root,'MIPmovies'),'dir'))
        mkdir(fullfile(root,'MIPmovies'));
    end
    
    copyfile(fullfile(frameDir,[outName,'_','.mp4']),fullfile(root,'MIPmovies','.'));

    prgs.ClearProgress(true);
end
