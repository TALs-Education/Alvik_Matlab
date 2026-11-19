function root = alvikLibraryRoot()
% ALVIKLIBRARYROOT Folder that holds the Arduino_Alvik and ucPack libraries.
%   Search order:
%     1. getpref('AlvikBlocks', 'LibraryRoot'), if set:
%          setpref('AlvikBlocks', 'LibraryRoot', 'D:\path\to\libraries')
%     2. third_party next to this file (a copy shipped with the blocks)
%     3. the Arduino IDE sketchbook: <Documents>\Arduino\libraries
%   Install the libraries with the Arduino IDE Library Manager
%   ("Arduino_Alvik", which also installs "ucPack").

candidates = {};
if ispref('AlvikBlocks', 'LibraryRoot')
    candidates{end+1} = getpref('AlvikBlocks', 'LibraryRoot');
end
candidates{end+1} = fullfile(fileparts(mfilename('fullpath')), 'third_party');
if ispc
    candidates{end+1} = fullfile(getenv('USERPROFILE'), 'Documents', 'Arduino', 'libraries');
else
    candidates{end+1} = fullfile(getenv('HOME'), 'Arduino', 'libraries');
    candidates{end+1} = fullfile(getenv('HOME'), 'Documents', 'Arduino', 'libraries');
end

for k = 1:numel(candidates)
    c = candidates{k};
    if isfile(fullfile(c, 'Arduino_Alvik', 'src', 'Arduino_Alvik.h')) && ...
            isfile(fullfile(c, 'ucPack', 'src', 'ucPack.h'))
        root = c;
        return
    end
end

error('AlvikBlocks:libraryNotFound', ...
    ['Arduino_Alvik / ucPack libraries not found. Install "Arduino_Alvik" with the ' ...
    'Arduino IDE Library Manager, or set the folder with ' ...
    'setpref(''AlvikBlocks'', ''LibraryRoot'', <folder>). Searched:\n  %s'], ...
    strjoin(candidates, '\n  '));
end
