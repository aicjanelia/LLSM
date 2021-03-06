function RenameFiles(root)
    [imD,dirNames] = LLSM.GetRAWmetadata(root,false);

    for i = 1:length(dirNames)
        subDir = dirNames{i};
        fprintf('%s...',fullfile(root,subDir));
        imDTemp = MicroscopeData.ReadMetadata(fullfile(root,subDir),false);
        if (~isempty(imDTemp))
            fprintf('\n');
            continue
        end
        
        suffix = '';
        if (~isempty(regexpi(subDir,'decon')))
            suffix = '_decon';
        elseif (~isempty(regexpi(subDir,'deskewed')))
            suffix = '_deskewed';
        else
            fprintf('\n');
            continue
        end
        
        tempD = MicroscopeData.GetEmptyMetadata;
        tempD.DatasetName = [imD.DatasetName,suffix];
        tempD.PixelPhysicalSize = imD.PixelPhysicalSize;
        
        curFiles = dir(fullfile(root,subDir,'*.klb'));
        if (~isempty(curFiles))
            ext = 'klb';
        else
            curFiles = dir(fullfile(root,subDir,'*.tif'));
            if (isempty(curFiles))
                error('No image files found');
            end
            ext = 'tif';
        end
        
        switch ext
            case 'klb'
                tempIm = MicroscopeData.KLB.readKLBstack(fullfile(root,subDir,curFiles(1).name));
            case 'tif'
                tempIm = MicroscopeData.LoadTif(fullfile(root,subDir,curFiles(1).name));
        end
        
        sz = size(tempIm);
        tempD.Dimensions = sz([2,1,3]);
        w = whos('tempIm');
        tempD.PixelFormat = w.class;
        
        curFileNames = {curFiles.name};
        curMask = cellfun(@(x)(~isempty(x)),regexpi(curFileNames,['\.',ext]));
        curFileNames = {curFiles(curMask).name}';
        curFileNames = regexpi(curFileNames,['(.*).',ext],'tokens');
        curFileNames = cellfun(@(x)(x{:}),curFileNames);
        [datasetName,chans,cams,stacks,iter,wavelengths,secs,fileSuffixs] = LLSM.ParseFileNames(curFileNames);
        
        if (~strcmp(datasetName,imD.DatasetName))
%             warning('Dataset name missmatch: %s -> %s',imD.DatasetName,datasetName{1});
        end
        
        [tempD.NumberOfFrames, useStacks] = LLSM.GetNumberOfFrames(iter,stacks);
        
        [tempD.NumberOfChannels, tempD.ChannelNames, wavelengths, useCams] = LLSM.GetChannelData(cams,chans,wavelengths);
        
        tempD.ChannelColors = Utils.GetColorByWavelength(wavelengths);
        
        tempD.TimeStampDeta = MicroscopeData.GetOrderedTimeStamps(secs,tempD.NumberOfFrames,tempD.NumberOfChannels);

        prgs = Utils.CmdlnProgress(tempD.NumberOfFrames,true,'Renaming');
        for t=1:tempD.NumberOfFrames
            if (useStacks)
                timeMask = stacks==t-1;
            else
                timeMask = iter==t-1;
            end
            if (useCams)
                curChan = 1;
                unqChns = unique(chans);
                unqCams = unique(cams);
                for ch = 1:length(unqChns)
                    chanMask = chans==unqChns(ch);
                    for cm = 1:length(unqCams)
                        camMask = strcmpi(unqCams(cm),cams);
                        camChanMask = camMask & chanMask;
                        if (any(camChanMask))
                            inName = [curFileNames{timeMask & camChanMask}];
                            outName = sprintf('%s_c%d_t%04d',tempD.DatasetName,curChan,t);
                            if (exist(fullfile(root,subDir,[inName,'.',ext]),'file'))
                                movefile(fullfile(root,subDir,[inName,'.',ext]),fullfile(root,subDir,[outName,'.',ext]));
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
                    if (exist(fullfile(root,subDir,[inName,'.',ext]),'file'))
                        movefile(fullfile(root,subDir,[inName,'.',ext]),fullfile(root,subDir,[outName,'.',ext]));
                    end
                end
            end
            prgs.PrintProgress(t);
        end
 
        MicroscopeData.CreateMetadata(fullfile(root,subDir),tempD);
        prgs.ClearProgress(true);
    end
end
