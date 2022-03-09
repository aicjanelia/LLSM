function [numChannels, channelNames, wavelengths, useCamField, uniqueCams, uniqueChans] = GetChannelData(cams,chans,wavelengths)
    wavelengthList = [];
    channelNames = {};
    
    uniqueCams = 0;
    uniqueChans = 0;
    if (~isempty(cams))
        useCamField = true;
        uniqueCams = unique(cams);
        uniqueChans = unique(chans);
        c = 0;
        for ch = 1:length(uniqueChans)
            chanMask = chans==uniqueChans(ch);
            for cm = 1:length(uniqueCams)
                camMask = strcmpi(uniqueCams(cm),cams);
                camChanMask = camMask & chanMask;
                if (any(camChanMask))
                    wvlgth = unique(wavelengths(camChanMask));
                    c = c +1;
                    wavelengthList(c) = wvlgth;
                    channelNames{c} = sprintf('%d Cam%s',wvlgth,uniqueCams{cm});
                end
            end
        end
        numChannels = c;
    else
        useCamField = false;
        
        uniqueChans = unique(chans);
        c = 0;
        for ch = 1:length(uniqueChans)
            chanMask = chans==uniqueChans(ch);
            if (any(chanMask))
                wvlgth = unique(wavelengths(chanMask));
                c = c +1;
                wavelengthList(c) = wvlgth;
                channelNames{c} = sprintf('%d',wvlgth);
            end
        end
        numChannels = c;
    end

    wavelengths = zeros(numChannels,1);
    for c=1:numChannels
        wavelengths(c) = wavelengthList(c);
    end
end
