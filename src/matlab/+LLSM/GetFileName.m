function fullPath = GetFileName(rootDir,camera,frame,channel)
    if (~exist('camera','var'))
        camera = [];
    end
    
    if (~exist('frame','var'))
        frame = [];
    end
    
    if (~exist('channel','var'))
        channel = [];
    end

    extension = 'klb';        

    curFiles = dir(fullfile(rootDir,['*.',extension]));
    if (isempty(curFiles))
        curFiles = dir(fullfile(rootDir,'*.tif'));
        if (isempty(curFiles))
            error('Unknown file extension');
        end
        extension = 'tif';
    end

    curFileNames = {curFiles.name};
    curMask = cellfun(@(x)(~isempty(x)),regexpi(curFileNames,[filesep,'.',extension]));
    curFileNames = {curFiles(curMask).name}';
    curFileNames = regexpi(curFileNames,['(.*).',extension],'tokens');
    curFileNames = cellfun(@(x)(x{:}),curFileNames);
    [~,chans,cams,stacks,iter] = LLSM.ParseFileNames(curFileNames);

    if (isempty(iter) || max(stacks(:))>max(iter(:)))
        useStacks = true;
    else
        useStacks = false;
    end

    if (~isempty(cams))
        unqCams = unique(cams);
        if (length(unqCams)>1)
            useCams = true;
        else
            if (~isempty(camera) && ~strcmp(unqCams,camera))
                fullPath = '';
                return
            end
            useCams = false;
        end
    else
        if (~isempty(camera))
            fullPath = '';
            return
        end
        useCams = false;
    end
    
    if (useStacks)
        if (isempty(frame))
            timeMask = true(size(stacks));
        else
            timeMask = stacks==frame-1;
        end
    else
        if (isempty(frame))
            timeMask = true(size(stacks));
        else
            timeMask = iter==frame-1;
        end
    end
    
    name = [];
    if (~isempty(camera) && useCams)
        camChanMask = cellfun(@(x)(x==camera),cams) & chans==channel-1;
        if (any(camChanMask))
            name = vertcat(curFileNames{timeMask & camChanMask});
        end
    elseif (~isempty(channel))
        chanMask = chans==channel-1;
        if (any(chanMask))
            name = [curFileNames{timeMask & chanMask}];
        end
    else
        name = vertcat(curFileNames{timeMask});
    end
    
    if (isempty(name))
        fullPath = '';
    else
        fullPath = [repmat([rootDir,filesep],size(name,1),1),[name,repmat(['.',extension],size(name,1),1)]];
    end
end
