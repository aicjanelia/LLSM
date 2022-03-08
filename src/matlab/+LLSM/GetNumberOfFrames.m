function [nFrames, useStacks] = GetNumberOfFrames(iter,stacks)
    if (isempty(iter) || max(stacks(:))>max(iter(:)))
        useStacks = true;
    else
        useStacks = false;
    end

    if (useStacks)
        nFrames = max(stacks(:));
    else
        nFrames = max(iter(:));
    end
end
