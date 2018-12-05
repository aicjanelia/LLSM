function fullPath = GetFileName(rootDir,camera,frame,channel)
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
    curMask = cellfun(@(x)(~isempty(x)),regexpi(curFileNames,['\.',extension]));
    curFileNames = {curFiles(curMask).name}';
    curFileNames = regexpi(curFileNames,['(.*).',extension],'tokens');
    curFileNames = cellfun(@(x)(x{:}),curFileNames);
    [datasetName,chans,cams,stacks,iter] = LLSM.ParseFileNames(curFileNames);

    if (isempty(iter) || max(stacks(:))>max(iter(:)))
        useStacks = true;
    else
        useStacks = false;
    end

    if (useStacks)
        tempD.NumberOfFrames = max(stacks(:));
    else
        tempD.NumberOfFrames = max(iter(:));
    end

    if (~isempty(cams))
        unqCams = unique(cams);
        if (length(unqCams)>1)
            useCams = true;
        else
            useCams = false;
        end
    else
        useCams = false;
    end
    
    if (useStacks)
        timeMask = stacks==frame-1;
    else
        timeMask = iter==frame-1;
    end
    
    name = [];
    if (useCams)
        camChanMask = cellfun(@(x)(x==camera),cams) & chans==channel-1;
        if (any(camChanMask))
            name = [curFileNames{timeMask & camChanMask}];
        end
    else
        chanMask = chans==channel-1;
        if (any(chanMask))
            name = [curFileNames{timeMask & chanMask}];
        end
    end
    
    if (isempty(name))
        fullPath = '';
    else
        fullPath = fullfile(rootDir,[name,'.',extension]);
    end
end
