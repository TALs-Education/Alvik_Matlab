function createAlvikBlocksLibrary()
% CREATEALVIKBLOCKSLIBRARY Builds AlvikBlocks.slx with the Arduino Alvik block.
%   Safe to re-run; it rebuilds the library.

libName = 'AlvikBlocks';
libDir  = fileparts(mfilename('fullpath'));
libFile = fullfile(libDir, [libName '.slx']);

if bdIsLoaded(libName)
    close_system(libName, 0);
end
if isfile(libFile)
    delete(libFile);
end

new_system(libName, 'Library');
add_block('simulink/User-Defined Functions/MATLAB System', ...
    [libName '/Arduino Alvik'], 'System', 'AlvikRobot', 'Position', [100 80 300 200]);
set_param(libName, 'EnableLBRepository', 'on');
save_system(libName, libFile);
close_system(libName, 0);

fprintf('Created %s\n', libFile);
end
