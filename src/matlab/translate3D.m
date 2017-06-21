function [ output ] = translate3D( inArray, dx,dy,dz)

T = [1 0 0 0
    0 1 0 0
    0 0 1 0
    dx dy dz 1];

% TSIZE_B = round(size(inArray) + [ceil(dx) ceil(dy) ceil(dz)]);
TSIZE_B = size(inArray);

tform = maketform('affine', T);
R = makeresampler('linear', 'fill');
TDIMS_A = [1 2 3];
TDIMS_B = [1 2 3];
output = tformarray(inArray, tform, R, TDIMS_A, TDIMS_B, TSIZE_B, [], 0);

end

