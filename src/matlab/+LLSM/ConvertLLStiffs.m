function ConvertLLStiffs(dirIn,dirOut,subfolders)
% ConvertLLStiffs(dirIn,dirOut,subfolders)
% Convert tif files from the LLSM into the H5 format for Eric's utilities

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
%         hasJson = dir(fullfile(dirIn,['*',subfolders{s},'.json']));
%         if (~isempty(hasJson))
%             continue
%         end
        
        %[im,imD] = LLSM.ReadOrgTiffs(dirIn,subfolders{s});
        LLSM.ReadOrgTiffs(dirIn,subfolders{s},dirOut);
       
        % write out KLB file
        %MicroscopeData.WriterKLB(im,'path',dirOut,'imageData',imD,'verbose',true);
    end
    
    fprintf('Total conversion time was %s\n',Utils.PrintTime(toc(llsmTic)))
end
