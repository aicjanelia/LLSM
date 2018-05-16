function im = RemoveSkewArtifacts(im,shrinkDist,minVal)
% RemoveBoarderArtifacts attempts to take the edge of the real data and
% shrinks in by shrinkDist

    if (~exist('shrinkDist','var') || isempty(shrinkDist))
        shrinkDist = 5;
    end
    if (~exist('minVal','var') || ispempty(minVal))
        minVal = 0;
    end
    
    artT = tic;
    fprintf('Removing Artifacts...');

    boundBW = im>minVal;
    boundBW = ImProc.Closure(boundBW,ones(5,5,5),[],[]);
    boundBW(1,:,:,:,:) = false;
    boundBW(:,1,:,:,:) = false;
    boundBW(:,:,1,:,:) = false;
    boundBW(end,:,:,:,:) = false;
    boundBW(:,end,:,:,:) = false;
    boundBW(:,:,end,:,:) = false;
    se = ImProc.MakeEllipsoidMask([shrinkDist,1,3]);
    boundBW = ImProc.MinFilter(boundBW,se,[],[]);
    im(~boundBW) = 0;
    
    fprintf('done. %s\n',Utils.PrintTime(toc(artT)));
end
