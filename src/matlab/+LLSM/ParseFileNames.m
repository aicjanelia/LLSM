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
    
    chanPrefix = 'ch';
    camsPrefix = 'CAM';
    stacksPrefix = 'stack';
    iterPrefix = 'Iter_';
    wavelengthSuffix = 'nm';
    secsSuffix = 'msec';
    
    filePrefix = Utils.GetDirListPrefixSuffix(fNames);

    cams = regexpi(fNames,[camsPrefix,'(.)'],'tokens');
    if (any(cellfun(@(x)(~isempty(x)),cams)))
        cams = cellfun(@(x)(x{:}),cams)';
    else
        cams = [];
    end
    
    if (isempty(cams))
        datasetName = regexpi(filePrefix,['^(\w+)','_',chanPrefix],'tokens');
    else
        datasetName = regexpi(filePrefix,['^(\w+)','_',camsPrefix],'tokens');
    end
    datasetName = vertcat(datasetName{:});

    chans = regexpi(fNames,[chanPrefix,'(\d)'],'tokens');
    if (~isempty(chans{1}))
        chans = cellfun(@(x)(str2double(x{:})),chans)';
    else
        chans = [];
    end

    stacks = regexpi(fNames,[stacksPrefix,'(\d+)'],'tokens');
    if (~isempty(stacks{1}))
        stacks = cellfun(@(x)(str2double(x{:})),stacks)';
    else
        stacks = [];
    end
    
    iter = regexpi(fNames,[iterPrefix,'(\d+)'],'tokens');
    if (~isempty(iter{1}))
        iter = cellfun(@(x)(str2double(x{:})),iter)';
    else
        iter = [];
    end

    wavelengths = regexp(fNames,['(\d\d\d)',wavelengthSuffix],'tokens');
    if (~isempty(wavelengths{1}))
        wavelengths = cellfun(@(x)(str2double(x{:})),wavelengths)';
    else
        wavelengths = [];
    end

    secs = regexpi(fNames,['(\d+)',secsSuffix,'_'],'tokens');
    if (~isempty(secs{1}))
        secs = cellfun(@(x)(str2double(x{:})),secs)';
    else
        secs = [];
    end

    fileSuffixs = regexpi(fNames,'msec_(.*)','tokens');
    if (~isempty(fileSuffixs{1}))
        fileSuffixs = cellfun(@(x)(x{:}),fileSuffixs);
    else
        fileSuffixs = [];
    end
end