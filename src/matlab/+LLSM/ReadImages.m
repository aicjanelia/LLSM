function [im,metadata] = ReadImages(imageDir, frames, channels, optionalTxtPath)
    if (~exist('frames','var'))
        frames = [];
    end
    
    if (~exist('channels','var'))
        channels = [];
    end

    if (~exist('optionalTxtPath','var'))
        optionalTxtPath = [];
    end

    tifList = dir(fullfile(imageDir,'*.tif'));
    klbList = dir(fullfile(imageDir,'*.klb'));
    
    ext = 'tif';
    if (isempty(tifList))
        if (isempty(klbList))
            error('Cannot find image files!');
        end
        ext = 'klb';
    elseif (~isempty(klbList))
        if (length(tifList)<length(klbList))
            ext = 'klb';
        end
    end  

    [metadata, names] = LLSM.GetMetadataFromFileStruct(imageDir,ext,optionalTxtPath);

    im = LLSM.ReadImageOrgName(metadata,names,frames,channels);
    
    metadata.NumberOfFrames = size(im,5);
    metadata.NumberOfChannels = size(im,4);
end
