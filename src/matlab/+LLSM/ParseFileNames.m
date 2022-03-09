function [datasetName,chans,cams,stacks,iter,wavelengths,secs,fileSuffixs] = ParseFileNames(fNames,ext)

    if (isstruct(fNames))
        if (~exist('ext','var')||isempty(ext))
            ext = 'klb';
        end
        curFileNames = {fNames.name};
        curMask = cellfun(@(x)(~isempty(x)),regexpi(curFileNames,['\.',ext]));
        curFileNames = {fNames(curMask).name}';
        curFileNames = regexpi(curFileNames,['(.*).',ext],'tokens');
        curFileNames = cellfun(@(x)(x{:}),curFileNames);
        fNames = curFileNames;
    end
    
    chanPrefix = '_ch';
    camsPrefix = '_Cam';
    stacksPrefix = '_stack';
    iterPrefix = '_Iter_';
    wavelengthSuffix = 'nm_';
    secsSuffix = 'msec_';
    timeSuffix = 'msecAbs_';
    
    filePrefix = Utils.GetDirListPrefixSuffix(fNames);

    cams = regexp(fNames,[camsPrefix,'(\w)'],'tokens');
    cams = cams(cellfun(@(x)(~isempty(x)),cams));
    if (~isempty(cams))
        cams = cellfun(@(x)(x{:}),cams)';
    end

    chans = regexpi(fNames,[chanPrefix,'(\d)'],'tokens');
    chans = chans(cellfun(@(x)(~isempty(x)),chans));
    if (~isempty(chans))
        chans = cellfun(@(x)(str2double(x{:})),chans)';
    end

    stacks = regexpi(fNames,[stacksPrefix,'(\d+)'],'tokens');
    stacks = stacks(cellfun(@(x)(~isempty(x)),stacks));
    if (~isempty(stacks))
        stacks = cellfun(@(x)(str2double(x{:})),stacks)';
    else
        stacks = [];
    end
    
    iter = regexpi(fNames,[iterPrefix,'(\d+)'],'tokens');
    iter = iter(cellfun(@(x)(~isempty(x)),iter));
    if (~isempty(iter))
        iter = cellfun(@(x)(str2double(x{:})),iter)';
    end
    
    if (~contains(filePrefix,'_'))
        datasetName = filePrefix;
    elseif (isempty(iter))
        datasetName = regexpi(filePrefix,['^(\w+)',chanPrefix],'tokens');
    else
        datasetName = regexpi(filePrefix,['^(\w+)',iterPrefix],'tokens');
    end
    if (iscell(datasetName))
        datasetName = vertcat(datasetName{:});
    end

    wavelengths = regexp(fNames,['_(\d\d\d)',wavelengthSuffix],'tokens');
    wavelengths = wavelengths(cellfun(@(x)(~isempty(x)),wavelengths));
    if (~isempty(wavelengths))
        wavelengths = cellfun(@(x)(str2double(x{:})),wavelengths)';
    end

    if (max(stacks)>0)
        secs = regexpi(fNames,['(\d+)',secsSuffix],'tokens');
        secs = secs(cellfun(@(x)(~isempty(x)),secs));
        if (~isempty(secs))
            secs = cellfun(@(x)(str2double(x{:})),secs)';
        end
    else
        timeStamps = regexpi(fNames,['(\d+)',timeSuffix],'tokens');
        timeStamps = timeStamps(cellfun(@(x)(~isempty(x)),timeStamps));
        if (~isempty(timeStamps))
            timeStamps = cellfun(@(x)(str2double(x{:})),timeStamps)';
        end
        secs = timeStamps-timeStamps(1);
    end

    fileSuffixs = regexpi(fNames,'msec_(.*)','tokens');
    fileSuffixs = fileSuffixs(cellfun(@(x)(~isempty(x)),fileSuffixs));
    if (~isempty(fileSuffixs))
        fileSuffixs = cellfun(@(x)(x{:}),fileSuffixs);
    end
end