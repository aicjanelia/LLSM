function MakeMIPmoviesFromRoot(root,subPath)
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

    mipSubsMask = cellfun(@(x)(strcmpi(x,'MIPs')),subDirs);
    otherDirs = subDirs(~mipSubsMask);
    mipSubs = subDirs(mipSubsMask);
    if (isempty(mipSubs))
        mipSubsMask = cellfun(@(x)(~isempty(x)),regexp(subDirs,'KLB'));
        deskewMask = cellfun(@(x)(~isempty(x)),regexp(subDirs,'Deskewed'));
        mipSubsMask = mipSubsMask & ~deskewMask;
        mipSubs = subDirs(mipSubsMask);
    end

    if (~isempty(subPath))
        parfor i=1:length(otherDirs)
            subSub = fullfile(subPath,otherDirs{i});
            try
                LLSM.MakeMIPmoviesFromRoot(root,subSub);
            catch err
                warning('Could not process %s\n%s',fullfile(root,subSub),err.message);
            end
        end
    else
        for i=1:length(otherDirs)
            subSub = fullfile(subPath,otherDirs{i});
            try
                LLSM.MakeMIPmoviesFromRoot(root,subSub);
            catch err
                warning('Could not process %s\n%s',fullfile(root,subSub),err.message);
            end
        end
    end

    for i=1:length(mipSubs)
        subSub = fullfile(subPath,mipSubs{i});
        LLSM.MakeMIPmovie(root,subSub);
    end
end
