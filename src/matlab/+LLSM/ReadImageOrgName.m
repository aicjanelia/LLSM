function im = ReadImageOrgName(metadata, names, frames, channels)
    if (~exist('frames','var') || isempty(frames))
        if (names.useStacks)
            frames = 1:max(names.stacks);
        else
            frames = 1:max(names.iter);
        end
    end
    if (~exist('channels','var') || isempty(channels))
        channels = 1:length(names.uniqueCams) * length(names.uniqueChans);
    end

    im = TimeLoop(metadata,names,0,'uint16',frames(1),channels);
    if (length(frames)==1)
        return
    end
    
    sz = size(im);
    cl = class(im);
    im = zeros([sz,length(frames)],'like',im);
    
    [~,~,ext] = fileparts(names.fileNames{1});
    
    if (strcmp(ext,'.tif'))
        parfor t=1:length(frames)
            curIm = TimeLoop(metadata,names,sz,cl,frames(t),channels);
            im(:,:,:,:,t) = curIm;
        end
    else
        for t=1:length(frames)
            curIm = TimeLoop(metadata,names,sz,cl,frames(t),channels);
            im(:,:,:,:,t) = curIm;
        end
    end
end

function im = TimeLoop(metadata,names,sz,cl,frame,channels)
    if (names.useStacks)
        timeMask = names.stacks==frame-1;
    else
        timeMask = names.iter==frame-1;
    end
    
    im = zeros(sz,cl);

    for c=1:length(channels)
        channel = channels(c);

        if (names.useCams)
            % cameras are itterated first and then the channel field
            cameraInd = mod(channel-1,length(names.uniqueCams)) +1;
            chnInd = floor((channel-1)/length(names.uniqueCams)) +1;

            chanMask = names.chans==names.uniqueChans(chnInd);
            camMask = strcmpi(names.uniqueCams{cameraInd},names.cams);
            camChanMask = camMask & chanMask;
            fMask = timeMask & camChanMask;
        else
            chanMask = names.chans==channel;
            fMask = timeMask & chanMask;
        end

        if (~any(fMask))
            warning('Cannot find file for frame: %d channel: %d name: %s',frame,channels(c),metadata.imageDir);
            return
        end

        fName = names.fileNames{fMask};
        if (~exist(fullfile(metadata.imageDir,fName),'file'))
            warning('Cannot find file for frame: %d channel: %d name: %s',frame,channels(c),metadata.imageDir);
            return
        end

        [~,~,ext] = fileparts(fName);

        if (strcmp(ext,'.klb'))
            tempIm = MicroscopeData.KLB.readKLBstack(fullfile(metadata.imageDir,fName));
        elseif (strcmp(ext,'.tif'))
            tempIm = MicroscopeData.LoadTif(fullfile(metadata.imageDir,fName));
        else
            error('Unknown file extention!');
        end
        
        szTemp = size(tempIm);

        im(1:szTemp(1),1:szTemp(2),1:szTemp(3),c) = tempIm;
    end
end
