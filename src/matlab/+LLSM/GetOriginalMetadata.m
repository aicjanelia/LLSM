function [imageData,zOffset] = GetOriginalMetadata(rootDir)
    if (~exist('rootDir','var') || isempty(rootDir))
        root = uigetdir();
    else
        root = rootDir;
    end
    
    settingsList = dir(fullfile(root,'*_Settings.txt'));
    if (length(settingsList)>1)
        error('Multiple iterations is not implemented yet!');
    end
    
    imageData = MicroscopeData.GetEmptyMetadata();

    metadataStr = fileread(fullfile(root,settingsList.name));
    dateStr = regexp(metadataStr,'(\d+)/(\d+)/(\d+) (\d+):(\d+):(\d+) ([AP]M)','tokens');
    month = str2double(dateStr{1,1}{1,1});
    day = str2double(dateStr{1,1}{1,2});
    year = str2double(dateStr{1,1}{1,3});
    hour = str2double(dateStr{1,1}{1,4});
    if (strcmpi(dateStr{1,1}{1,7},'PM'))
        hour = hour +12;
    end
    minute = str2double(dateStr{1,1}{1,5});
    second = str2double(dateStr{1,1}{1,6});

    imageData.StartCaptureDate = sprintf('%d-%d-%d %02d:%02d:%02d',year,month,day,hour,minute,second);

    zOffsetStr = regexp(metadataStr,'S PZT.*Excitation \(0\) :\t\d+\t(\d+).(\d+)','tokens');
    zOffset = str2double([zOffsetStr{1,1}{1,1}, '.', zOffsetStr{1,1}{1,2}]);

    laserWavelengthStr = regexp(metadataStr,'Excitation Filter, Laser, Power \(%\), Exp\(ms\) \((\d+)\) :\tN/A\t(\d+)','tokens');
    laserWaveLengths = zeros(length(laserWavelengthStr),1);
    for i=1:length(laserWavelengthStr)
        laserWaveLengths(str2double(laserWavelengthStr{1,i}{1,1})+1) = str2double(laserWavelengthStr{1,i}{1,2});
    end
    imageData.ChannelNames = arrayfun(@(x)(num2str(x)),laserWaveLengths,'uniformoutput',false);

    if (length(laserWavelengthStr)==1)
        imageData.ChannelColors = [1,1,1];
    else
        colrs = jet(7);
        imageData.ChannelColors = colrs(round((laserWaveLengths-488)/300*7+1),:);
    end
end
