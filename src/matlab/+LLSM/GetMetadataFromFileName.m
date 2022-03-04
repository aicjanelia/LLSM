function metadata = GetMetadataFromFileName(fName)
% Metadata has the following fields: 
%	DatasetName
%	Channel
%	Camera
%	Stack
%	Wavelength
%	DeltaMSec
%	CpuMSec

    iter = regexpi(fName,'.*_Iter_(\d+)_.*','tokens');
    
    if (isempty(iter))
        iter = regexpi(fName,'.*_Iter_ch(\d+)_.*','tokens');
        if (isempty(iter))
            searchStr = '^(.*)_ch(\d)_CAM(\d)_stack(\d+)_(\d+)nm_(\d+)msec_(\d+).*';
        else
            searchStr = '^(.*)_Iter_ch(\d)_CAM(\d)_stack(\d+)_(\d+)nm_(\d+)msec_(\d+).*';
        end
    else
        searchStr = '^(.*)_Iter_\d+_ch(\d)_CAM(\d)_stack(\d+)_(\d+)nm_(\d+)msec_(\d+).*';
    end
    
    tok = regexpi(fName,searchStr,'tokens');
    
    metadata.DatasetName = tok{1,1}{1,1};
    metadata.Channel = str2double(tok{1,1}{1,2});
    metadata.Camera = str2double(tok{1,1}{1,3});
    metadata.Stack = str2double(tok{1,1}{1,4});
    metadata.Wavelength = str2double(tok{1,1}{1,5});
    metadata.DeltaMSec = str2double(tok{1,1}{1,6});
    metadata.CpuMSec = str2double(tok{1,1}{1,7}); 
end
