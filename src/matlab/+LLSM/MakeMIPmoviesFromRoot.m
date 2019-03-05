function MakeMIPmoviesFromRoot(root,subPath,overwrite)
    if (~exist('overwrite','var') || isempty(overwrite))
        overwrite = false;
    end

    if (~exist('root','var') || isempty(root))
        root = uigetdir();
        if (root==0)
            return
        end
    end
    if (~exist('subPath','var'))
        subPath = '';
    end

    dList = dir(fullfile(root,subPath));
    dList = dList([dList.isdir]);
    subDirs = {dList.name};
    subDirs = subDirs(~cellfun(@(x)(strcmp(x,'.') || strcmp(x,'..')),subDirs));
    if (isempty(subDirs))
        return
    end

    klbMask = cellfun(@(x)(~isempty(x)),regexp(subDirs,'KLB'));

    if (~any(klbMask))
        for i=1:length(subDirs)
            subSub = fullfile(subPath,subDirs{i});
            LLSM.MakeMIPmoviesFromRoot(root,subSub,overwrite);
        end
    else
        try
            LLSM.MakeMIPmovie(root,subPath,overwrite);
        catch err
            warning(err.message)
        end
    end
end
