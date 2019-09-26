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

    for t=1:length(frames)
        frame = frames(t);

        if (names.useStacks)
            timeMask = names.stacks==frame-1;
        else
            timeMask = names.iter==frame-1;
        end

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
                im = [];
                return
            end

            fName = names.fileNames{fMask};
            if (~exist(fullfile(metadata.imageDir,fName),'file'))
                im = [];
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
            
            if (~exist('im','var'))
                im = zeros([size(tempIm),length(channels),length(frames)],'like',tempIm);
            end
            
            im(:,:,:,c,t) = tempIm;
        end
    end
end
