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
    
    mipSubsMask = cellfun(@(x)(strcmpi(x,'MIPs')),subDirs);
    mipSubs = subDirs(mipSubsMask);
    
    subDirs = subDirs(~mipSubsMask);
    
    for i=1:length(mipSubs)
        subSub = fullfile(subPath,mipSubs{i});
        LLSM.MakeMIPmovie(root,subSub);
    end
    
    for i=1:length(subDirs)
        subSub = fullfile(subPath,subDirs{i});
        LLSM.MakeMIPmoviesFromRoot(root,subSub);
    end
end
