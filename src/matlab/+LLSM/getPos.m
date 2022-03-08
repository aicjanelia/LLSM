function [pos] = getPos(fpath,ch)
% [pos] = getPos(fileDir)
% This function takes a list of LLSM file names and paths (from PATH) and
% will find the X-Y-Z stage positions and store them in a POS array. 

cd (fpath);
fileListing = dir(['*' ch '*.tif']);

pos = zeros(length(fileListing),3);

parfor k=1:length(fileListing)
    tic
    fname = fileListing(k).name;
    info = imfinfo(fname);
    posTemp = info(1).UnknownTags;
    pos(k,:) = [posTemp(1).Value, posTemp(2).Value, posTemp(3).Value];
    toc
end
