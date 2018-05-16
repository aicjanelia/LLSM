function RenameKLBdirs(root)
    if (~exist('root','var') || isempty(root))
        root = uigetdir();
        if (isempty(root))
            return
        end
    end
    
    dList = dir(root);
    settingsFile = cellfun(@(x)(~isempty(x)),regexpi({dList.name},'Settings.txt'));
    klbDirs = cellfun(@(x)(~isempty(x)),regexpi({dList.name},'.*KLB.*'));
    if (any(settingsFile) && any(klbDirs))
        LLSM.RenameKLBfiles(root);
    else
        dList = dList([dList.isdir]==true);
        for i=1:length(dList)
            if (strcmp(dList(i).name,'.') || strcmp(dList(i).name,'..'))
                continue
            end
            LLSM.RenameKLBdirs(fullfile(root,dList(i).name));
        end
    end
end
