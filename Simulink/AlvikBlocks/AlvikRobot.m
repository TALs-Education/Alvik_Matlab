classdef AlvikRobot < matlab.System & coder.ExternalDependency
    % AlvikRobot Arduino Alvik robot block (Arduino Nano ESP32 on the Alvik).
    %
    %   Talks to the Alvik's STM32 carrier through the Arduino_Alvik library.
    %
    %   Outputs
    %     battery   single   battery state of charge, percent. Updated every
    %                        Battery sample time and held in between.
    %     charging  boolean  (optional) true while the battery is charging.
    %     status    int8     (optional) result of the connection at startup:
    %                        0 = OK, -2 = STM32 firmware differs from the
    %                        version the library requires (still runs).
    %
    %   Inputs (Motor command mode)
    %     Disabled                 no inputs, the wheels aren't commanded.
    %     Wheel speeds             left, right wheel speed.
    %     Drive (linear, angular)  forward speed and turn rate of the robot.
    %   Commands are sent every Sample time and clamped to the limits.
    %
    %   Runs on hardware only (generated code: deploy or External mode). In
    %   normal simulation the outputs are zero. Use one Alvik block per model.

    %#codegen
    %#ok<*EMCA>

    properties (Nontunable)
        %Sample time (s)
        SampleTime = 0.02;
        %Battery sample time (s)
        BatterySampleTime = 1;
        %Output charging status
        OutputCharging (1,1) logical = true;
        %Output connection status
        OutputStatus (1,1) logical = false;

        %Motor command mode
        MotorMode = 'Wheel speeds (left, right)';
        %Wheel speed units
        WheelUnits = 'RPM';
        %Wheel speed limit (in wheel speed units, Inf = none)
        WheelLimit = Inf;
        %Linear speed units
        LinearUnits = 'cm/s';
        %Angular speed units
        AngularUnits = 'deg/s';
        %Linear speed limit (in linear units, Inf = none)
        LinearLimit = Inf;
        %Angular speed limit (in angular units, Inf = none)
        AngularLimit = Inf;
    end

    properties (Constant, Hidden)
        MotorModeSet = matlab.system.StringSet({'Disabled', ...
            'Wheel speeds (left, right)', 'Drive (linear, angular)'});
        WheelUnitsSet   = matlab.system.StringSet({'RPM', 'deg/s', 'rad/s', 'rev/s'});
        LinearUnitsSet  = matlab.system.StringSet({'cm/s', 'mm/s', 'm/s', 'in/s'});
        AngularUnitsSet = matlab.system.StringSet({'deg/s', 'rad/s', 'RPM', 'rev/s'});
    end

    properties (Access = private)
        BattCounter = uint32(0);
        BattValue = single(0);
        ChargingValue = false;
    end

    methods
        function obj = AlvikRobot(varargin)
            coder.allowpcode('plain');
            setProperties(obj, nargin, varargin{:});
        end

        function set.SampleTime(obj, val)
            validateattributes(val, {'numeric'}, ...
                {'scalar', 'real', 'finite', 'positive'}, '', 'Sample time');
            obj.SampleTime = val;
        end

        function set.BatterySampleTime(obj, val)
            validateattributes(val, {'numeric'}, ...
                {'scalar', 'real', 'finite', 'positive'}, '', 'Battery sample time');
            obj.BatterySampleTime = val;
        end

        function set.WheelLimit(obj, val)
            validateattributes(val, {'numeric'}, {'scalar', 'real', 'positive'}, '', 'Wheel speed limit');
            obj.WheelLimit = val;
        end

        function set.LinearLimit(obj, val)
            validateattributes(val, {'numeric'}, {'scalar', 'real', 'positive'}, '', 'Linear speed limit');
            obj.LinearLimit = val;
        end

        function set.AngularLimit(obj, val)
            validateattributes(val, {'numeric'}, {'scalar', 'real', 'positive'}, '', 'Angular speed limit');
            obj.AngularLimit = val;
        end
    end

    methods (Access = protected)
        function validatePropertiesImpl(obj)
            r = obj.BatterySampleTime / obj.SampleTime;
            if r < 1 - 1e-9 || abs(r - round(r)) > 1e-6
                error(['Battery sample time (%g s) must be an integer multiple ' ...
                    'of the Sample time (%g s).'], obj.BatterySampleTime, obj.SampleTime);
            end
        end

        function setupImpl(obj)
            if coder.target('Rtw')
                coder.cinclude('MW_Alvik.h');
                coder.ceval('MW_Alvik_Setup');
            end
        end

        function resetImpl(obj)
            obj.BattCounter = uint32(0);   % read the battery on the first step
            obj.BattValue = single(0);
            obj.ChargingValue = false;
        end

        function varargout = stepImpl(obj, varargin)
            % --- motor commands ---
            switch obj.MotorMode
                case 'Wheel speeds (left, right)'
                    lim = obj.WheelLimit;
                    left  = single(max(-lim, min(lim, double(varargin{1}))));
                    right = single(max(-lim, min(lim, double(varargin{2}))));
                    if coder.target('Rtw')
                        coder.ceval('MW_Alvik_SetWheelSpeeds', left, right, obj.wheelUnitCode());
                    end
                case 'Drive (linear, angular)'
                    lin = single(max(-obj.LinearLimit,  min(obj.LinearLimit,  double(varargin{1}))));
                    ang = single(max(-obj.AngularLimit, min(obj.AngularLimit, double(varargin{2}))));
                    if coder.target('Rtw')
                        coder.ceval('MW_Alvik_Drive', lin, ang, ...
                            obj.linearUnitCode(), obj.angularUnitCode());
                    end
                otherwise
                    % Disabled: no command
            end

            % --- battery, decimated to the battery sample time ---
            if obj.BattCounter == 0
                b = single(0);
                c = false;
                if coder.target('Rtw')
                    b = coder.ceval('MW_Alvik_BatteryCharge');
                    c = coder.ceval('MW_Alvik_BatteryCharging');
                end
                obj.BattValue = b;
                obj.ChargingValue = c;
            end
            obj.BattCounter = obj.BattCounter + uint32(1);
            if obj.BattCounter >= obj.batteryDecimation()
                obj.BattCounter = uint32(0);
            end

            % --- outputs ---
            varargout{1} = obj.BattValue;
            k = 1;
            if obj.OutputCharging
                k = k + 1;
                varargout{k} = obj.ChargingValue;
            end
            if obj.OutputStatus
                k = k + 1;
                s = int8(0);
                if coder.target('Rtw')
                    s = coder.ceval('MW_Alvik_Status');
                end
                varargout{k} = s;
            end
        end

        % ---------- ports ----------
        function n = getNumInputsImpl(obj)
            if strcmp(obj.MotorMode, 'Disabled')
                n = 0;
            else
                n = 2;
            end
        end

        function varargout = getInputNamesImpl(obj)
            if strcmp(obj.MotorMode, 'Drive (linear, angular)')
                varargout = {['v ' obj.LinearUnits], ['w ' obj.AngularUnits]};
            else
                varargout = {['left ' obj.WheelUnits], ['right ' obj.WheelUnits]};
            end
        end

        function n = getNumOutputsImpl(obj)
            n = 1 + double(obj.OutputCharging) + double(obj.OutputStatus);
        end

        function varargout = getOutputNamesImpl(obj)
            names = obj.outputNameList();
            varargout = names;
        end

        function varargout = getOutputSizeImpl(obj)
            varargout = repmat({[1 1]}, 1, obj.getNumOutputsImpl());
        end

        function varargout = getOutputDataTypeImpl(obj)
            types = {'single'};
            if obj.OutputCharging
                types{end+1} = 'logical';
            end
            if obj.OutputStatus
                types{end+1} = 'int8';
            end
            varargout = types;
        end

        function varargout = isOutputComplexImpl(obj)
            varargout = repmat({false}, 1, obj.getNumOutputsImpl());
        end

        function varargout = isOutputFixedSizeImpl(obj)
            varargout = repmat({true}, 1, obj.getNumOutputsImpl());
        end

        function sts = getSampleTimeImpl(obj)
            sts = createSampleTime(obj, 'Type', 'Discrete', 'SampleTime', obj.SampleTime);
        end

        function flag = isInactivePropertyImpl(obj, prop)
            wheel = strcmp(obj.MotorMode, 'Wheel speeds (left, right)');
            drive = strcmp(obj.MotorMode, 'Drive (linear, angular)');
            switch prop
                case {'WheelUnits', 'WheelLimit'}
                    flag = ~wheel;
                case {'LinearUnits', 'AngularUnits', 'LinearLimit', 'AngularLimit'}
                    flag = ~drive;
                otherwise
                    flag = false;
            end
        end

        % ---------- icon ----------
        function cmds = getMaskDisplayImpl(obj)
            switch obj.MotorMode
                case 'Wheel speeds (left, right)'
                    detail = ['wheels  ' obj.WheelUnits];
                case 'Drive (linear, angular)'
                    detail = ['drive  ' obj.LinearUnits ', ' obj.AngularUnits];
                otherwise
                    detail = 'motors off';
            end
            cmds = [ ...
                ['color(''white'');', newline] ...
                ['plot([100,100,100,100]*1,[100,100,100,100]*1);', newline] ...
                ['plot([100,100,100,100]*0,[100,100,100,100]*0);', newline] ...
                ['color(''blue'');', newline] ...
                ['text(50, 90, ''\fontsize{9} ARDUINO NANO ESP32'',''texmode'',''on'',''horizontalAlignment'', ''center'');', newline] ...
                ['color(''black'');', newline] ...
                ['text(50, 55, ''\fontsize{13}\bf Alvik'',''texmode'',''on'',''horizontalAlignment'', ''center'');', newline] ...
                ['text(50, 22, ''\fontsize{8}' detail ''',''texmode'',''on'',''horizontalAlignment'', ''center'');', newline] ...
                ['text(50, 10, ''\fontsize{8}battery every ' num2str(obj.BatterySampleTime) ' s'',''texmode'',''on'',''horizontalAlignment'', ''center'');', newline] ...
                ];
            if ~strcmp(obj.MotorMode, 'Disabled')
                in = cell(1, 2);
                [in{:}] = obj.getInputNamesImpl();
                for k = 1:2
                    cmds = [cmds, sprintf('port_label(''input'', %d, ''%s'');', k, in{k}), newline]; %#ok<AGROW>
                end
            end
            out = obj.outputNameList();
            for k = 1:numel(out)
                cmds = [cmds, sprintf('port_label(''output'', %d, ''%s'');', k, out{k}), newline]; %#ok<AGROW>
            end
        end
    end

    methods (Access = private)
        function names = outputNameList(obj)
            names = {'battery %'};
            if obj.OutputCharging
                names{end+1} = 'charging';
            end
            if obj.OutputStatus
                names{end+1} = 'status';
            end
        end

        function n = batteryDecimation(obj)
            n = uint32(round(obj.BatterySampleTime / obj.SampleTime));
        end

        % unit codes of the Arduino_Alvik library (definitions.h)
        function u = wheelUnitCode(obj)
            switch obj.WheelUnits
                case 'deg/s'
                    u = uint8(1);   % DEG_S
                case 'rad/s'
                    u = uint8(2);   % RAD_S
                case 'rev/s'
                    u = uint8(3);   % REV_S
                otherwise
                    u = uint8(0);   % RPM
            end
        end

        function u = linearUnitCode(obj)
            switch obj.LinearUnits
                case 'mm/s'
                    u = uint8(1);   % MM_S
                case 'm/s'
                    u = uint8(2);   % M_S
                case 'in/s'
                    u = uint8(3);   % IN_S
                otherwise
                    u = uint8(0);   % CM_S
            end
        end

        function u = angularUnitCode(obj)
            switch obj.AngularUnits
                case 'RPM'
                    u = uint8(0);   % RPM
                case 'rad/s'
                    u = uint8(2);   % RAD_S
                case 'rev/s'
                    u = uint8(3);   % REV_S
                otherwise
                    u = uint8(1);   % DEG_S
            end
        end
    end

    methods (Static, Access = protected)
        function simMode = getSimulateUsingImpl
            simMode = 'Interpreted execution';
        end

        function flag = showSimulateUsingImpl
            flag = false;
        end

        function header = getHeaderImpl
            header = matlab.system.display.Header(mfilename('class'), ...
                'Title', 'Arduino Alvik', ...
                'Text', ['Arduino Alvik robot (Arduino Nano ESP32 on the Alvik), through the ' ...
                'Arduino_Alvik library.' newline newline ...
                'Outputs the battery charge (%) every Battery sample time, and optionally the ' ...
                'charging flag and the connection status (0 = OK, -2 = STM32 firmware differs ' ...
                'from the version the library requires).' newline newline ...
                'Motor commands: wheel speeds (left, right) or drive (linear, angular), sent ' ...
                'every Sample time and clamped to the limits.' newline newline ...
                'Hardware only: outputs are zero in normal simulation. Startup waits while ' ...
                'the Alvik power switch is off. Use one Alvik block per model.'], ...
                'ShowSourceLink', false);
        end

        function groups = getPropertyGroupsImpl
            timing = matlab.system.display.Section('Title', 'Timing and outputs', ...
                'PropertyList', {'SampleTime', 'BatterySampleTime', 'OutputCharging', 'OutputStatus'});
            motors = matlab.system.display.Section('Title', 'Motor commands', ...
                'PropertyList', {'MotorMode', 'WheelUnits', 'WheelLimit', ...
                'LinearUnits', 'AngularUnits', 'LinearLimit', 'AngularLimit'});
            groups = [timing, motors];
        end
    end

    methods (Static)
        function name = getDescriptiveName(~)
            name = 'Arduino Alvik';
        end

        function tf = isSupportedContext(~)
            tf = true;
        end

        function updateBuildInfo(buildInfo, context)
            if ~context.isCodeGenTarget('rtw')
                return
            end
            rootDir = fileparts(mfilename('fullpath'));
            addIncludePaths(buildInfo, fullfile(rootDir, 'include'));
            addIncludeFiles(buildInfo, 'MW_Alvik.h');
            if ~isequal(get_param(buildInfo.ModelName, 'SystemTargetFile'), 'ert.tlc')
                return
            end
            addSourcePaths(buildInfo, fullfile(rootDir, 'src'));
            addSourceFiles(buildInfo, 'MW_Alvik.cpp', fullfile(rootDir, 'src'), 'BlockModules');

            % Arduino_Alvik and ucPack library sources
            libRoot = alvikLibraryRoot();
            alvikSrc = fullfile(libRoot, 'Arduino_Alvik', 'src');
            ucpackSrc = fullfile(libRoot, 'ucPack', 'src');
            addIncludePaths(buildInfo, {alvikSrc, ucpackSrc});
            addSourceFiles(buildInfo, 'Arduino_Alvik.cpp', alvikSrc, 'BlockModules');
            addSourceFiles(buildInfo, {'ucPack.cpp', 'CircularBuffer.cpp'}, ...
                {ucpackSrc, ucpackSrc}, 'BlockModules');

            % Wire and EEPROM from the ESP32 core used by the support package
            coreLibs = '$(ARDUINO_ESP32_ROOT)/hardware/esp32/$(ESP32_LIB_VERSION)/libraries';
            addIncludePaths(buildInfo, {[coreLibs '/Wire/src'], [coreLibs '/EEPROM/src']});
            addSourceFiles(buildInfo, 'Wire.cpp', [coreLibs '/Wire/src'], 'BlockModules');
            addSourceFiles(buildInfo, 'EEPROM.cpp', [coreLibs '/EEPROM/src'], 'BlockModules');
        end
    end
end
