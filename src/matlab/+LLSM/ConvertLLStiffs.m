function ConvertLLStiffs(dirIn,dirOut,subfolders,overwrite,deleteOrg)
% ConvertLLStiffs(dirIn,dirOut,subfolders)
% Convert tif files from the LLSM into the H5 format for Eric's utilities

    if (~exist('overwrite','var') || isempty(overwrite))
        overwrite = false;
    end

    llsmTic = tic;

    if (~exist('dirIn','var') || isempty(dirIn))
        root = uigetdir();
    else
        root = dirIn;
    end

    if (~exist('dirOut','var') || isempty(dirOut))
        dirOut = root;
    end
    
    if (~exist('subfolders','var') || isempty(subfolders))
        subfolders = {'CPPdecon';'Deskewed';''};
    end
    
    if (ischar(subfolders))
        subfolders = {subfolders};
    end
    
    if (~exist('deleteOrg','var') || isempty(deleteOrg))
        deleteOrg = false;
    end

    for s = 1:length(subfolders)
        if (~exist(fullfile(root,subfolders{s}),'dir'))
            warning('Cannot find %s.\nSkipping\n',fullfile(root,subfolders{s}));
            continue
        end
        
        datasetName = LLSM.ParseSettingsFileNames(dirIn);
        
        tempSubfolderName = subfolders{s};
        if (~isempty(tempSubfolderName))
            tempSubfolderName = ['_',tempSubfolderName];
        end
        
        if (~isempty(datasetName) && ...
                (~exist(fullfile(dirOut,[subfolders{s},'KLB'],[datasetName,tempSubfolderName,'.json']),'file') || overwrite))
            [errorCodes,errorMsg] = LLSM.ReadOrgTiffs(dirIn,subfolders{s},dirOut);
            if ((isempty(errorCodes) || ~errorCodes(3)) && deleteOrg)
                dList = dir(fullfile(dirIn,subfolders{s},'*.tif'));
                for i=1:length(dList)
                    delete(fullfile(dirIn,subfolders{s},dList(i).name));
                end
                dList = dir(fullfile(dirIn,subfolders{s},'*.*'));
                if (length(dList)==2)
                    delete(fullfile(dirIn,subfolders{s}));
                end
            end
            dList = dir(fullfile(dirIn,subfolders{s},'*.csv'));
            for i=1:length(dList)
                if (~strcmpi(fullfile(dirIn,subfolders{s},dList(i).name),fullfile(dirOut,dList(i).name)))
                    copyfile(fullfile(dirIn,subfolders{s},dList(i).name),fullfile(dirOut,dList(i).name));
                end
            end
            dList = dir(fullfile(dirIn,subfolders{s},'*.txt'));
            for i=1:length(dList)
                if (~strcmpi(fullfile(dirIn,subfolders{s},dList(i).name),fullfile(dirOut,dList(i).name)))
                    copyfile(fullfile(dirIn,subfolders{s},dList(i).name),fullfile(dirOut,dList(i).name));
                end
            end
        else
            disp(['Skipping ',datasetName,'_',subfolders{s}]);
        end
    end
    
    fprintf('Total conversion time was %s\n',Utils.PrintTime(toc(llsmTic)))
end
