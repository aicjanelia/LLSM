function RenameKLBdirs(root)
    dList = dir(root);
    settingsFile = cellfun(@(x)(~isempty(x)),regexpi({dList.name},'Settings.txt'));
    if (any(settingsFile))
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
