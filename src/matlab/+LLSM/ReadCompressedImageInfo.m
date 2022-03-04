function imInfo = ReadCompressedImageInfo(imagePath,verbose)
    [~,name] = fileparts(imagePath);
    tempDir = fullfile(getenv('TEMP'),name);

    if (exist(tempDir,'dir'))
        rmdir(tempDir,'s');
    end
    mkdir(tempDir);
    
    cmd = sprintf('7z e "%s" -o"%s"',imagePath,tempDir);
    [msg,cmdout] = system(cmd);
    if (exist('verbose','var') && ~isempty(verbose) && verbose)
        disp(cmdout);
    end
    
    tifName = dir(fullfile(tempDir,'*.tif'));
    imInfo = imfinfo(fullfile(tempDir,tifName.name));
    
    if (exist(tempDir,'dir'))
        rmdir(tempDir,'s');
    end
end