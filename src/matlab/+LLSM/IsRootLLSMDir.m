function [found,dataName] = IsRootLLSMDir(root) 
    found = false;
    dataName = '';

    dList = dir(fullfile(root,'*_Settings.txt'));
    
    if (isempty(dList))
        return
    end
    
    dataName = regexpi(dList.name,'(.*)_Settings.txt','tokens');
    dataName = dataName{1,1}{1,1};
    dList = dir(fullfile(root,[dataName,'*.tif']));
    
    if (isempty(dList))
        return
    else
        found = true;
    end
end
