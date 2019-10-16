function [im,boundBW] = RemoveSkewArtifacts(im,shrinkDist,minVal,verbose)
% RemoveBoarderArtifacts attempts to take the edge of the real data and
% shrinks in by shrinkDist

    if (~exist('verbose','var') || isempty(verbose))
        verbose = false;
    end

    if (~exist('shrinkDist','var') || isempty(shrinkDist))
        shrinkDist = 5;
    end
    if (~exist('minVal','var') || ispempty(minVal))
        minVal = 0;
    end
    
    artT = tic;
    
    if (verbose)
        fprintf('Removing Artifacts...');
    end

    boundBW = im>minVal;
    boundBW = HIP.Closure(boundBW,ones(5,5,5),[],[]);
    boundBW(1,:,:,:,:) = false;
    boundBW(:,1,:,:,:) = false;
    boundBW(:,:,1,:,:) = false;
    boundBW(end,:,:,:,:) = false;
    boundBW(:,end,:,:,:) = false;
    boundBW(:,:,end,:,:) = false;
    se = HIP.MakeEllipsoidMask([shrinkDist,1,3]);
    boundBW = HIP.MinFilter(boundBW,se,[],[]);
    
    im(~boundBW) = 0;
    
    if (verbose)
        fprintf('done. %s\n',Utils.PrintTime(toc(artT)));
    end
end
