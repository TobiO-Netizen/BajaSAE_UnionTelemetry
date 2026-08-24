%% ================================================================
%  LiveTelemetry.m — Real-time car telemetry dashboard
%
%  Reads LoRa receiver serial data and plots live subplots for
%  speed, temperature, acceleration, gyroscope, RSSI, and SNR.
%
%  LoRa receiver format:
%    +RCV=<addr>,<len>,<payload>,<RSSI>,<SNR>
%
%  Payload format (pipe-delimited):
%    pkt|aX|aY|aZ|gX|gY|gZ|spdMph|objF|forceN|lat|lon|sats
%
%  Usage:
%    LiveTelemetry("COM5")            % Windows
%    LiveTelemetry("/dev/ttyUSB0")    % Linux / Mac
%    LiveTelemetry("COM5", 300)       % custom window of 300 samples
% =================================================================

function LiveTelemetry(comPort, maxPoints)

    arguments
        comPort   string
        maxPoints double = 200   % rolling window size
    end

    BAUD = 115200;

    % -- Field indices inside the pipe-delimited payload -----------
    F_PKT   = 1;
    F_AX    = 2;   F_AY  = 3;   F_AZ  = 4;
    F_GX    = 5;   F_GY  = 6;   F_GZ  = 7;
    F_SPEED = 8;
    F_TEMP  = 9;
    F_FORCE = 10;
    F_LAT   = 11;  F_LON = 12;
    F_SATS  = 13;

    % -- Preallocate circular buffers --------------------------------
    N       = maxPoints;
    time_s  = NaN(1, N);      % elapsed seconds
    ax_buf  = NaN(1, N);  ay_buf  = NaN(1, N);  az_buf  = NaN(1, N);
    gx_buf  = NaN(1, N);  gy_buf  = NaN(1, N);  gz_buf  = NaN(1, N);
    spd_buf = NaN(1, N);
    tmp_buf = NaN(1, N);
    frc_buf = NaN(1, N);
    rssi_buf= NaN(1, N);
    snr_buf = NaN(1, N);
    sat_buf = NaN(1, N);
    lat_buf = NaN(1, N);  lon_buf = NaN(1, N);
    idx     = 0;

    % -- Open serial port --------------------------------------------
    fprintf("Opening %s at %d baud...\n", comPort, BAUD);
    try
        s = serialport(comPort, BAUD);
        configureTerminator(s, "LF");
        s.Timeout = 10;
    catch err
        error("Could not open %s: %s", comPort, err.message);
    end
    cleanupObj = onCleanup(@() delete(s));   % auto-close on exit

    fprintf("Connected. Waiting for data...\n\n");

    % -- Build figure & subplots -------------------------------------
    fig = figure('Name', 'Car Telemetry — Live', ...
                 'NumberTitle', 'off', ...
                 'Color', [0.12 0.12 0.14], ...
                 'Position', [50 50 1400 900]);

    tileset = tiledlayout(fig, 4, 3, 'TileSpacing', 'compact', ...
                          'Padding', 'compact');
    title(tileset, 'Live Car Telemetry', ...
          'Color', 'w', 'FontSize', 16, 'FontWeight', 'bold');

    % Acceleration X / Y / Z
    axAx = nexttile(tileset);  hAx = animatedline(axAx, 'Color', [0.25 0.80 0.90], 'LineWidth', 1.2);
    stylePlot(axAx, 'Accel X', 'g');

    axAy = nexttile(tileset);  hAy = animatedline(axAy, 'Color', [0.40 0.85 0.50], 'LineWidth', 1.2);
    stylePlot(axAy, 'Accel Y', 'g');

    axAz = nexttile(tileset);  hAz = animatedline(axAz, 'Color', [0.95 0.55 0.25], 'LineWidth', 1.2);
    stylePlot(axAz, 'Accel Z', 'g');

    % Gyroscope X / Y / Z
    axGx = nexttile(tileset);  hGx = animatedline(axGx, 'Color', [0.25 0.80 0.90], 'LineWidth', 1.2);
    stylePlot(axGx, 'Gyro X', '°/s');

    axGy = nexttile(tileset);  hGy = animatedline(axGy, 'Color', [0.40 0.85 0.50], 'LineWidth', 1.2);
    stylePlot(axGy, 'Gyro Y', '°/s');

    axGz = nexttile(tileset);  hGz = animatedline(axGz, 'Color', [0.95 0.55 0.25], 'LineWidth', 1.2);
    stylePlot(axGz, 'Gyro Z', '°/s');

    % Speed / Temp / Force
    axSpd = nexttile(tileset);  hSpd = animatedline(axSpd, 'Color', [0.95 0.30 0.35], 'LineWidth', 1.4);
    stylePlot(axSpd, 'Speed', 'mph');

    axTmp = nexttile(tileset);  hTmp = animatedline(axTmp, 'Color', [1.0 0.75 0.20], 'LineWidth', 1.4);
    stylePlot(axTmp, 'Object Temp', '°F');

    axFrc = nexttile(tileset);  hFrc = animatedline(axFrc, 'Color', [0.70 0.45 0.95], 'LineWidth', 1.4);
    stylePlot(axFrc, 'Force', 'N');

    % RSSI / SNR / Satellites
    axRssi = nexttile(tileset);  hRssi = animatedline(axRssi, 'Color', [0.90 0.35 0.55], 'LineWidth', 1.2);
    stylePlot(axRssi, 'RSSI', 'dBm');

    axSnr = nexttile(tileset);  hSnr = animatedline(axSnr, 'Color', [0.35 0.75 0.95], 'LineWidth', 1.2);
    stylePlot(axSnr, 'SNR', 'dB');

    axSat = nexttile(tileset);  hSat = animatedline(axSat, 'Color', [0.50 0.90 0.40], 'LineWidth', 1.2);
    stylePlot(axSat, 'Satellites', '#');

    % -- Status text (lat/lon, packet #) -----------------------------
    statusTxt = annotation(fig, 'textbox', [0.01 0.005 0.98 0.03], ...
        'String', 'Waiting for first packet...', ...
        'Color', [0.7 0.7 0.7], 'EdgeColor', 'none', ...
        'FontSize', 10, 'FontName', 'Consolas', ...
        'HorizontalAlignment', 'center', ...
        'FitBoxToText', 'off');

    startTic = tic;

    % -- Main read loop ----------------------------------------------
    while isvalid(fig)
        try
            line = readline(s);
        catch
            continue;   % timeout — just retry
        end

        if strlength(line) == 0, continue; end

        % Parse +RCV=<addr>,<len>,<payload>,<RSSI>,<SNR>
        [payload, rssi, snr, ok] = parseRCV(line);
        if ~ok, continue; end

        % Split pipe-delimited payload
        fields = split(payload, '|');
        if numel(fields) < F_SATS, continue; end

        % -- Extract values ------------------------------------------
        elapsed = toc(startTic);
        pkt     = str2double(fields{F_PKT});
        aX      = str2double(fields{F_AX});
        aY      = str2double(fields{F_AY});
        aZ      = str2double(fields{F_AZ});
        gX      = str2double(fields{F_GX});
        gY      = str2double(fields{F_GY});
        gZ      = str2double(fields{F_GZ});
        speed   = str2double(fields{F_SPEED});
        objTemp = str2double(fields{F_TEMP});
        forceN  = str2double(fields{F_FORCE});
        lat     = str2double(fields{F_LAT});
        lon     = str2double(fields{F_LON});
        sats    = str2double(fields{F_SATS});

        % -- Console output ------------------------------------------
        fprintf(['[#%04d]  Accel: %+.2f %+.2f %+.2f g  |  ' ...
                 'Gyro: %+.1f %+.1f %+.1f  |  ' ...
                 'Spd: %.1f mph  |  Temp: %.1fF  |  ' ...
                 'Force: %.1f N  |  ' ...
                 'RSSI: %d  SNR: %d  |  ' ...
                 'Sats: %d  (%.6f, %.6f)\n'], ...
                pkt, aX, aY, aZ, gX, gY, gZ, ...
                speed, objTemp, forceN, rssi, snr, sats, lat, lon);

        % -- Push into animated lines ---------------------------------
        addpoints(hAx,  elapsed, aX);
        addpoints(hAy,  elapsed, aY);
        addpoints(hAz,  elapsed, aZ);
        addpoints(hGx,  elapsed, gX);
        addpoints(hGy,  elapsed, gY);
        addpoints(hGz,  elapsed, gZ);
        addpoints(hSpd, elapsed, speed);
        addpoints(hTmp, elapsed, objTemp);
        addpoints(hFrc, elapsed, forceN);
        addpoints(hRssi,elapsed, rssi);
        addpoints(hSnr, elapsed, snr);
        addpoints(hSat, elapsed, sats);

        % Slide x-axis window
        if elapsed > 60
            windowStart = elapsed - 60;
            setAxLimits([axAx axAy axAz axGx axGy axGz ...
                         axSpd axTmp axFrc axRssi axSnr axSat], ...
                        windowStart, elapsed);
        end

        % -- Update status bar ----------------------------------------
        statusTxt.String = sprintf( ...
            'Pkt #%d  |  Lat: %.6f  Lon: %.6f  |  Sats: %d  |  RSSI: %d dBm  |  SNR: %d dB  |  Elapsed: %.0fs', ...
            pkt, lat, lon, sats, rssi, snr, elapsed);

        drawnow limitrate;
    end

    fprintf("\nFigure closed — stopping.\n");
end

%% ================================================================
%  Parse a +RCV line from the LoRa receiver
%  Format: +RCV=<addr>,<len>,<payload>,<RSSI>,<SNR>
% =================================================================
function [payload, rssi, snr, ok] = parseRCV(line)
    payload = ""; rssi = 0; snr = 0; ok = false;

    line = strip(line);
    if ~startsWith(line, "+RCV="), return; end

    % Strip the "+RCV=" prefix
    body = extractAfter(line, "+RCV=");

    % Split on comma — but payload itself has no commas (pipe-delimited)
    parts = split(body, ',');
    % Expected: addr, len, payload, RSSI, SNR  (5 parts)
    if numel(parts) < 5, return; end

    payload = parts{3};
    rssi    = str2double(parts{end-1});
    snr     = str2double(parts{end});
    ok      = true;
end

%% ================================================================
%  Apply consistent dark styling to a subplot
% =================================================================
function stylePlot(ax, titleStr, unitStr)
    title(ax, titleStr, 'Color', 'w', 'FontSize', 11);
    ylabel(ax, unitStr, 'Color', [0.7 0.7 0.7]);
    ax.Color           = [0.15 0.15 0.18];
    ax.XColor          = [0.5 0.5 0.5];
    ax.YColor          = [0.5 0.5 0.5];
    ax.GridColor       = [0.3 0.3 0.3];
    ax.GridAlpha       = 0.6;
    ax.XGrid           = 'on';
    ax.YGrid           = 'on';
    ax.Box             = 'on';
    ax.FontSize        = 9;
end

%% ================================================================
%  Slide x-axis limits for all axes
% =================================================================
function setAxLimits(axes_array, xmin, xmax)
    for i = 1:numel(axes_array)
        axes_array(i).XLim = [xmin, xmax];
    end
end
