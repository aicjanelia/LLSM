function ConvertLLStiffs(dirIn,dirOut,subfolders,overwrite)
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
        subfolders = {'';'Deskewed';'CPPdecon'};
    end

    for s = 1:length(subfolders)
        if (~exist(fullfile(root,subfolders{s}),'dir'))
            warning('Cannot find %s.\nSkipping\n',fullfile(root,subfolders{s}));
            continue
        end
        
        datasetName = LLSM.ParseSettingsFileNames(dirIn);
        
        if (~isempty(datasetName) && ...
                (~exist(fullfile(dirOut,[subfolders{s},'KLB'],[datasetName,'_',subfolders{s},'.json']),'file') || overwrite))
            LLSM.ReadOrgTiffs(dirIn,subfolders{s},dirOut);
        else
            disp(['Skipping ',datasetName,'_',subfolders{s}]);
        end
    end
    
    fprintf('Total conversion time was %s\n',Utils.PrintTime(toc(llsmTic)))
end
