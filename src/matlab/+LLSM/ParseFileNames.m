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
    datasetNameSuffix = '_ch';
    chanPrefix = 'ch';
    camsPrefix = 'CAM';
    stacksPrefix = 'stack';
    iterPrefix = 'Iter_';
    wavelengthSuffix = 'nm';
    secsSuffix = 'msec';

    datasetName = regexpi(fNames,['^(\w+)',datasetNameSuffix],'tokens');
    datasetName = vertcat(datasetName{:});

    chans = regexpi(fNames,[chanPrefix,'(\d)'],'tokens');
    chans = cellfun(@(x)(str2double(x{:})),chans)';

    cams = regexpi(fNames,[camsPrefix,'(\d)'],'tokens');
    if (any(cellfun(@(x)(~isempty(x)),cams)))
        cams = cellfun(@(x)(str2double(x{:})),cams)';
    else
        cams = [];
    end

    stacks = regexpi(fNames,[stacksPrefix,'(\d+)'],'tokens');
    stacks = cellfun(@(x)(str2double(x{:})),stacks)';
    
    iter = regexpi(fNames,[iterPrefix,'(\d+)'],'tokens');
    if (~isempty(iter{1}))
        iter = cellfun(@(x)(str2double(x{:})),iter)';
    else
        iter = [];
    end

    wavelengths = regexp(fNames,['(\d\d\d)',wavelengthSuffix],'tokens');
    wavelengths = cellfun(@(x)(str2double(x{:})),wavelengths)';

    secs = regexpi(fNames,['(\d+)',secsSuffix,'_'],'tokens');
    secs = cellfun(@(x)(str2double(x{:})),secs)';

    fileSuffixs = regexpi(fNames,'msec_(.*)','tokens');
    fileSuffixs = cellfun(@(x)(x{:}),fileSuffixs);
end