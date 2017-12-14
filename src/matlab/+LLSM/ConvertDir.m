function ConvertDir(rootDir,outDir,overwrite,deleteOrg)
    if (~exist('rootDir','var') || isempty(rootDir))
        rootDir = uigetdir('.','Choose source folder');
        if (rootDir==0), return, end
    end
    [~,name,~]=fileparts(rootDir);
    if (~exist('outDir','var') || isempty(outDir))
        outDir = uigetdir(rootDir,['Choose destination folder for source: ' name]);
        if (outDir==0), return, end
    end

    if (~exist('overwrite','var'))% || isempty(overwrite))
        overwrite = 0;
    end

    recursiveConvertDir(rootDir,outDir,'',overwrite,deleteOrg);
end

function recursiveConvertDir(rootDir,outDir,subDir,overwrite,deleteOrg)
    isRoot = LLSM.IsRootLLSMDir(fullfile(rootDir,subDir));
    if (isRoot)
        LLSM.ConvertLLStiffs(fullfile(rootDir,subDir),fullfile(outDir,subDir),[],overwrite,deleteOrg);
        return
    end
    
    dList = dir(fullfile(rootDir,subDir));
    dList = dList([dList.isdir]);
    rm = strcmp({dList.name},'.') | strcmp({dList.name},'..');
    dList = dList(~rm);
    
    for i=1:length(dList)
        recursiveConvertDir(fullfile(rootDir,subDir),fullfile(outDir,subDir),dList(i).name,overwrite,deleteOrg);
    end    
end