function [datasetName,chans,cams,stacks,wavelengths,secs,fileSuffixs] = ParseFileNames(fNames,ext)

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
    wavelengthSuffix = 'nm';
    secsSuffix = 'msec';

    datasetName = regexpi(fNames,['^(\w+)',datasetNameSuffix],'tokens');
    datasetName = vertcat(datasetName{:});

    chans = regexpi(fNames,[chanPrefix,'(\d)'],'tokens');
    chans = cellfun(@(x)(str2double(x{:})),chans)';

    cams = regexpi(fNames,[camsPrefix,'(\d)'],'tokens');
    cams = cellfun(@(x)(str2double(x{:})),cams)';

    stacks = regexpi(fNames,[stacksPrefix,'(\d+)'],'tokens');
    stacks = cellfun(@(x)(str2double(x{:})),stacks)';

    wavelengths = regexpi(fNames,['(\d+)',wavelengthSuffix],'tokens');
    wavelengths = cellfun(@(x)(str2double(x{:})),wavelengths)';

    secs = regexpi(fNames,['(\d+)',secsSuffix,'_'],'tokens');
    secs = cellfun(@(x)(str2double(x{:})),secs)';

    fileSuffixs = regexpi(fNames,'msec_(.*)','tokens');
    fileSuffixs = cellfun(@(x)(x{:}),fileSuffixs);
end