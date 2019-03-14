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
    curMask = cellfun(@(x)(~isempty(x)),regexpi(curFileNames,[filesep,'*.',extension]));
    curFileNames = {curFiles(curMask).name}';
    curFileNames = regexpi(curFileNames,['(.*).',extension],'tokens');
    curFileNames = cellfun(@(x)(x{:}),curFileNames);
    [datsetNames,chans,cams,stacks,iter] = LLSM.ParseFileNames(curFileNames);

    if (~isempty(stacks))
        useStacks = true;
        numStacks = max(stacks)+1;
    else
        useStacks = false;
        numStacks = [];
    end
    if (~isempty(iter))
        numIter = max(iter)+1;
        useIter = true;
    else
        useIter = false;
        numIter = [];
    end

    if (~isempty(cams))
        unqCams = unique(cams);
        if (length(unqCams)>1)
            useCams = true;
        else
            if (~isempty(camera) && ~strcmpi(unqCams,camera))
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
    
    if (useIter)
        if (useStacks)
            iterIdx = floor((frame-1)/numStacks);
            stackIdx = frame -1 - iterIdx*numStacks;
            timeMask = stacks==stackIdx & iter==iterIdx;
        else
            timeMask = iter==(frame-1);
        end
    else
        timeMask = stacks==(frame-1);
    end
    
    name = [];
    if (~isempty(camera) && useCams)
        camChanMask = cellfun(@(x)(x==camera),cams) & chans==channel-1;
        if (any(camChanMask))
            name = {curFileNames{timeMask' & camChanMask'}}';
        end
    elseif (~isempty(channel))
        chanMask = chans==channel-1;
        if (any(chanMask))
            name = {curFileNames{timeMask & chanMask}}';
        end
    else
        name ={curFileNames{timeMask}}';
    end
    
    if (isempty(name))
        fullPath = '';
    else
        fullPath = cellfun(@(x)(fullfile(rootDir,[x,'.',extension])),name,'uniformOutput',false);
    end
end
