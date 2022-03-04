function [found,dataName] = IsRootLLSMDir(root) 
    found = false;
    dataName = '';

    dList = dir(fullfile(root,'*_Settings.txt'));
    
    if (isempty(dList))
        return
    end
    
%     if (length(dList)>1)
%         warning(['Too many settings files in ',root]);
%         return
%     end
    
    dataName = regexpi(dList(1).name,'(.*)_Settings.txt','tokens');
    if (isempty(dataName{1,1}))
        return
    end
    
    dataName = dataName{1,1}{1,1};
    dList = dir(fullfile(root,[dataName,'*.tif']));
    
%     if (isempty(dList))
%         return
%     else
        found = true;
%     end
end
