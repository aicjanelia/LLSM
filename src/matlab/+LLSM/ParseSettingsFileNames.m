function [datasetName,iterNumbers] = ParseSettingsFileNames(root)  
    settingsList = dir(fullfile(root,'*_Settings.txt'));
    if (length(settingsList)==1)
        [~,datasetName] = fileparts(settingsList.name);
        iterNumbers = 0;
    else
        vals = regexpi({settingsList.name},'(.*)_Iter_(\d+)_(.*).txt','tokens');
        if (isempty(vals{3}))
            warning(sprintf('%s does not have LLSM name scheme',root));
            datasetName = '';
            iterNumbers = [];
            return
        end
        datasetNames = cellfun(@(x)(x{1,1}{1,1}),vals,'uniformoutput',false);
        iterNumbers = cellfun(@(x)(str2double(x{1,1}{1,2})),vals);
        
        datasetName = unique(datasetNames);
        if (length(datasetName)>1)
            error('Found more than one dataset in folder');
        end
        datasetName = datasetName{1};
    end
    
    datasetName = strrep(datasetName,'_Settings','');
end
