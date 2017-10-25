function metadata = ParseSettingsFile(fullPathToFile)
% Metadata fields are:
%   startCaptureData
%   zOffset
%   laserWaveLengths
%   numChan
%   numStacks

    metadataStr = fileread(fullPathToFile);
        
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
    
    metadata.startCaptureDate = sprintf('%d-%d-%d %02d:%02d:%02d',year,month,day,hour,minute,second);
    
    zOffsetStr = regexp(metadataStr,'S PZT.*Excitation \(0\) :\t\d+\t(\d+)\.(\d+)','tokens');
    if (isempty(zOffsetStr))
        zOffsetStr = regexp(metadataStr,'S Piezo.*Excitation \(0\) :\t\d+\t(\d+)\.(\d+)','tokens');
    end
    if (~isempty(zOffsetStr))
        metadata.zOffset = str2double([zOffsetStr{1,1}{1,1}, '.', zOffsetStr{1,1}{1,2}]) * sin(31.8);
    else
        zOffsetStr = regexp(metadataStr,'Z PZT.*Excitation \(0\) :\t\d+\t(\d+)\.(\d+)','tokens');
        metadata.zOffset = str2double([zOffsetStr{1,1}{1,1}, '.', zOffsetStr{1,1}{1,2}]);
    end
    
    laserWavelengthStr = regexp(metadataStr,'Excitation Filter, Laser, Power \(%\), Exp\(ms\) \((\d+)\) :\tN/A\t(\d+)','tokens');
    laserWaveLengths = zeros(length(laserWavelengthStr),1);
    for i=1:length(laserWavelengthStr)
        laserWaveLengths(str2double(laserWavelengthStr{1,i}{1,1})+1) = str2double(laserWavelengthStr{1,i}{1,2});
    end
    
    metadata.laserWaveLengths = laserWaveLengths;
    
    numStacks = regexpi(metadataStr,'# of stacks \((\d)\) :	(\d+)','tokens');
    metadata.numChan = length(numStacks);
    metadata.numStacks = [];
    for c=1:metadata.numChan
        metadata.numStacks(c) = str2double(numStacks{1,c}{1,2});
    end
end
