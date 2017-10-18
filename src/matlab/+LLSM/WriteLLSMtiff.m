function WriteLLSMtiff(im,imD,root,deconRoot,orgSettingsPath,decon)
% e.g.
%cell_ch0_CAM1_stack0000_642nm_0000000msec_0025478410msecAbs_000x_000y_000z_0000t
    if (~exist('decon','var') || isempty(decon))
        decon = false;
    end

    if (~exist('root','var') || isempty(root))
        root = uigetdir();
    end
    
    if (~exist(root,'dir'))
        mkdir(root);
    end

    disp('Starting Deskew/Decon...');
    prgs = Utils.CmdlnProgress(imD.NumberOfChannels,true,'Writing Tiffs');
%     prgs = Utils.CmdlnProgress(imD.NumberOfChannels*imD.NumberOfFrames,true,'Writing Tiffs');
    for c=1:imD.NumberOfChannels
        parfor t=1:imD.NumberOfFrames

            fname = sprintf('cell_ch%d_CAM1_stack%04d_%snm_%07dmsec_0000000000msecAbs_000x_000y_000z_0000t.tif',...
                c-1, t-1,imD.ChannelNames{c},imD.TimeStampDelta(1,c,t));

            LLSM.write3Dtiff(im(:,:,:,c,t),fullfile(root,fname));
            
            if (decon)
                cmdArgs = sprintf('decon.exe "%s" "%s" -i 5 -b 25 -u -z 0.3 -Z 0.1',...
                    fullfile(root,fname),fullfile(deconRoot,[imD.ChannelNames{c},'_PSF.tif']));
                
                [status,cmdout] = system(cmdArgs);
            end 
        end
        prgs.PrintProgress(c);
    end
    
    if (exist('orgSettingsPath','var') && ~isempty(orgSettingsPath) && exist(orgSettingsPath,'file'))
        copyfile(orgSettingsPath,root);
    end
    
    prgs.ClearProgress(true);
end
