function RenameKLBfiles(root)
    [imD,dirNames] = LLSM.GetRAWmetadata(root);

    for i = 1:length(dirNames)
        subDir = dirNames{i};
        fprintf('%s...',fullfile(root,subDir));
        imDTemp = MicroscopeData.ReadMetadata(fullfile(root,subDir),false);
        if (~isempty(imDTemp))
            continue
        end
        
        suffix = '';
        if (~isempty(regexpi(subDir,'decon')))
            suffix = '_decon';
        elseif (~isempty(regexpi(subDir,'deskewed')))
            suffix = '_deskewed';
        end
        tempD = imD;
        tempD.DatasetName = [tempD.DatasetName,suffix];
        curFiles = dir(fullfile(root,subDir,'*.klb'));
        tempIm = MicroscopeData.KLB.readKLBstack(fullfile(root,subDir,curFiles(1).name));
        sz = size(tempIm);
        tempD.Dimensions = sz([2,1,3]);
        w = whos('tempIm');
        tempD.PixelFormat = w.class;
        
        curFileNames = {curFiles.name};
        curMask = cellfun(@(x)(~isempty(x)),regexpi(curFileNames,'\.klb'));
        curFileNames = {curFiles(curMask).name}';
        curFileNames = regexpi(curFileNames,'(.*).klb','tokens');
        curFileNames = cellfun(@(x)(x{:}),curFileNames);
        [datasetName,chans,cams,stacks,iter,wavelengths,secs,fileSuffixs] = LLSM.ParseFileNames(curFileNames);
        
        if (~strcmp(datasetName,imD.DatasetName))
%             warning('Dataset name missmatch: %s -> %s',imD.DatasetName,datasetName{1});
        end
        
        if (isempty(iter) || max(stacks(:))>max(iter(:)))
            useStacks = true;
        else
            useStacks = false;
        end
        
        if (useStacks)
            tempD.NumberOfFrames = max(stacks(:));
        else
            tempD.NumberOfFrames = max(iter(:));
        end
        
        if (~isempty(cams))
            useCams = true;
            unqCams = unique(cams);
            unqChns = unique(chans);
            c = 0;
            for cm = 1:length(unqCams)
                camMask = strcmpi(unqCams(cm),cams);
                for ch = 1:length(unqChns)
                    camChanMask = camMask & chans==unqChns(ch);
                    if (any(camChanMask))
                        wvlgth = unique(wavelengths(camChanMask));
                        c = c +1;
                        tempD.ChannelNames{c} = [num2str(wvlgth),' ', unqCams(cm)];
                        if (size(tempD.ChannelColors,1)<c)
                            tempD.ChannelColors(c,:) = colrs(c,:);
                        end
                    end
                end
            end
            tempD.NumberOfChannels = c;
        else
            useCams = false;
            tempD.NumberOfChannels = max(chans(:))+1;
        end
        
        secOrdered = sort(secs);
        tempD.TimeStampDelta = [];
        for t=0:tempD.NumberOfFrames-1
            startIdx = t*tempD.NumberOfChannels+1;
            deltaT = 0;
            for c=0:tempD.NumberOfChannels-1
                deltaT = deltaT + secOrdered(startIdx+c);
            end
            tempD.TimeStampDelta(end+1) = (deltaT./tempD.NumberOfChannels)*1e-3;
        end

        prgs = Utils.CmdlnProgress(tempD.NumberOfFrames,true,'Renaming');
        for t=1:tempD.NumberOfFrames
            if (useStacks)
                timeMask = stacks==t-1;
            else
                timeMask = iter==t-1;
            end
            if (useCams)
                curChan = 1;
                for ch = 1:length(unqChns)
                    camMask = strcmpi(unqCams(cm),cams);
                    for cm = 1:length(unqCams)
                        camChanMask = camMask & chans==unqChns(ch);
                        if (any(camChanMask))
                            inName = [curFileNames{timeMask & camChanMask}];
                            outName = sprintf('%s_c%d_t%04d',tempD.DatasetName,curChan,t);
                            if (exist(fullfile(root,subDir,[inName,'.klb']),'file'))
                                movefile(fullfile(root,subDir,[inName,'.klb']),fullfile(root,subDir,[outName,'.klb']));
                            end
                            curChan = curChan +1;
                        end
                    end
                end
            else
                for c=1:tempD.NumberOfChannels
                    chanMask = chans==c-1;
                    inName = [curFileNames{timeMask & chanMask}];
                    outName = sprintf('%s_c%d_t%04d',tempD.DatasetName,c,t);
                    if (exist(fullfile(root,subDir,[inName,'.klb']),'file'))
                        movefile(fullfile(root,subDir,[inName,'.klb']),fullfile(root,subDir,[outName,'.klb']));
                    end
                end
            end
            prgs.PrintProgress(t);
        end
 
        MicroscopeData.CreateMetadata(fullfile(root,subDir),tempD);
        prgs.ClearProgress(true);
    end
end
