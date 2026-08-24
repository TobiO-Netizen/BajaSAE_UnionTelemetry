%% ================================================================
%  PlotTelemetry.m — Plot all parameters from SD card CSV log
%
%  Usage:
%    PlotTelemetry("TEL_001.csv")
%    PlotTelemetry("TEL_001.csv", true)   % save plots as PNG
% =================================================================

function PlotTelemetry(csvFile, savePlots)

    arguments
        csvFile   string
        savePlots logical = false
    end

    % ── Read CSV ───────────────────────────────────────────────
    fprintf("Reading %s...\n", csvFile);
    opts = detectImportOptions(csvFile);
    opts.VariableNamingRule = 'preserve';
    T = readtable(csvFile, opts);

    fprintf("Loaded %d rows, %d columns.\n\n", height(T), width(T));
    disp(T.Properties.VariableNames');

    % ── Time axis — convert millis to seconds ──────────────────
    millis = T{:,1};
    t = (millis - millis(1)) / 1000;   % elapsed seconds from start

    % ── Column indices (matching CSV_HEADER in MainSystem.cpp) ─
    % millis,packetNum,
    % accelX_g,accelY_g,accelZ_g,
    % gyroX,gyroY,gyroZ,
    % objTempF,ambTempF,
    % forceN,forceLbf,
    % lat,lon,alt_m,speedMph,satellites,
    % timestamp

    accelX   = T{:,3};
    accelY   = T{:,4};
    accelZ   = T{:,5};
    gyroX    = T{:,6};
    gyroY    = T{:,7};
    gyroZ    = T{:,8};
    objTempF = T{:,9};
    ambTempF = T{:,10};
    forceN   = T{:,11};
    forceLbf = T{:,12};
    lat      = T{:,13};
    lon      = T{:,14};
    alt      = T{:,15};
    speedMph = T{:,16};
    sats     = T{:,17};

    % ── Plot style ─────────────────────────────────────────────
    mk   = 'o';           % marker type
    msz  = 4;             % marker size
    lw   = 0.8;           % line width connecting markers

    % ── Figure 1: Acceleration ─────────────────────────────────
    fig1 = figure('Name', 'Acceleration', 'NumberTitle', 'off', ...
                  'Position', [50 500 1200 500]);

    subplot(3,1,1);
    plot(t, accelX, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.2 0.6 0.9]);
    ylabel('g'); title('Accel X'); grid on;

    subplot(3,1,2);
    plot(t, accelY, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.3 0.8 0.4]);
    ylabel('g'); title('Accel Y'); grid on;

    subplot(3,1,3);
    plot(t, accelZ, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.9 0.5 0.2]);
    ylabel('g'); title('Accel Z'); grid on;
    xlabel('Time (s)');

    sgtitle('Acceleration (g)');

    % ── Figure 2: Gyroscope ────────────────────────────────────
    fig2 = figure('Name', 'Gyroscope', 'NumberTitle', 'off', ...
                  'Position', [50 400 1200 500]);

    subplot(3,1,1);
    plot(t, gyroX, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.2 0.6 0.9]);
    ylabel('°/s'); title('Gyro X (Roll)'); grid on;

    subplot(3,1,2);
    plot(t, gyroY, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.3 0.8 0.4]);
    ylabel('°/s'); title('Gyro Y (Pitch)'); grid on;

    subplot(3,1,3);
    plot(t, gyroZ, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.9 0.5 0.2]);
    ylabel('°/s'); title('Gyro Z (Yaw)'); grid on;
    xlabel('Time (s)');

    sgtitle('Gyroscope (°/s)');

    % ── Figure 3: Speed ────────────────────────────────────────
    fig3 = figure('Name', 'Speed', 'NumberTitle', 'off', ...
                  'Position', [50 300 1200 350]);

    plot(t, speedMph, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.9 0.2 0.25]);
    ylabel('mph'); title('GPS Speed'); xlabel('Time (s)'); grid on;

    % ── Figure 4: Temperature ──────────────────────────────────
    fig4 = figure('Name', 'Temperature', 'NumberTitle', 'off', ...
                  'Position', [50 200 1200 400]);

    subplot(2,1,1);
    plot(t, objTempF, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.95 0.3 0.3]);
    ylabel('°F'); title('Object Temperature (IR)'); grid on;

    subplot(2,1,2);
    plot(t, ambTempF, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.2 0.6 0.85]);
    ylabel('°F'); title('Ambient Temperature'); grid on;
    xlabel('Time (s)');

    sgtitle('Temperature (°F)');

    % ── Figure 5: Force ────────────────────────────────────────
    fig5 = figure('Name', 'Force', 'NumberTitle', 'off', ...
                  'Position', [50 100 1200 400]);

    subplot(2,1,1);
    plot(t, forceN, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.6 0.3 0.85]);
    ylabel('N'); title('Force (Newtons)'); grid on;

    subplot(2,1,2);
    plot(t, forceLbf, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.8 0.4 0.9]);
    ylabel('lbf'); title('Force (Pound-force)'); grid on;
    xlabel('Time (s)');

    sgtitle('Strain Gauge Force');

    % ── Figure 6: GPS Position ─────────────────────────────────
    fig6 = figure('Name', 'GPS Position', 'NumberTitle', 'off', ...
                  'Position', [50 50 1200 500]);

    subplot(2,2,[1 3]);
    plot(lon, lat, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.2 0.7 0.5]);
    hold on;
    plot(lon(1), lat(1), 'g^', 'MarkerSize', 10, 'MarkerFaceColor', 'g');   % start
    plot(lon(end), lat(end), 'rs', 'MarkerSize', 10, 'MarkerFaceColor', 'r'); % end
    hold off;
    xlabel('Longitude'); ylabel('Latitude');
    title('GPS Track'); legend('Track', 'Start', 'End', 'Location', 'best');
    grid on; axis equal;

    subplot(2,2,2);
    plot(t, alt, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.4 0.7 0.3]);
    ylabel('m'); title('Altitude'); grid on;

    subplot(2,2,4);
    plot(t, sats, ['-' mk], 'MarkerSize', msz, 'LineWidth', lw, 'Color', [0.3 0.8 0.5]);
    ylabel('#'); title('Satellites'); xlabel('Time (s)'); grid on;

    sgtitle('GPS Data');

    % ── Figure 7: All-in-one summary dashboard ─────────────────
    fig7 = figure('Name', 'Telemetry Dashboard', 'NumberTitle', 'off', ...
                  'Position', [100 50 1400 900]);

    tiledlayout(4, 3, 'TileSpacing', 'compact', 'Padding', 'compact');

    nexttile; plot(t, accelX, ['-' mk], 'MarkerSize', 3, 'Color', [0.2 0.6 0.9]);
    title('Accel X'); ylabel('g'); grid on;

    nexttile; plot(t, accelY, ['-' mk], 'MarkerSize', 3, 'Color', [0.3 0.8 0.4]);
    title('Accel Y'); ylabel('g'); grid on;

    nexttile; plot(t, accelZ, ['-' mk], 'MarkerSize', 3, 'Color', [0.9 0.5 0.2]);
    title('Accel Z'); ylabel('g'); grid on;

    nexttile; plot(t, gyroX, ['-' mk], 'MarkerSize', 3, 'Color', [0.2 0.6 0.9]);
    title('Gyro X'); ylabel('°/s'); grid on;

    nexttile; plot(t, gyroY, ['-' mk], 'MarkerSize', 3, 'Color', [0.3 0.8 0.4]);
    title('Gyro Y'); ylabel('°/s'); grid on;

    nexttile; plot(t, gyroZ, ['-' mk], 'MarkerSize', 3, 'Color', [0.9 0.5 0.2]);
    title('Gyro Z'); ylabel('°/s'); grid on;

    nexttile; plot(t, speedMph, ['-' mk], 'MarkerSize', 3, 'Color', [0.9 0.2 0.25]);
    title('Speed'); ylabel('mph'); grid on;

    nexttile; plot(t, objTempF, ['-' mk], 'MarkerSize', 3, 'Color', [0.95 0.55 0.2]);
    title('Obj Temp'); ylabel('°F'); grid on;

    nexttile; plot(t, forceN, ['-' mk], 'MarkerSize', 3, 'Color', [0.6 0.3 0.85]);
    title('Force'); ylabel('N'); grid on;

    nexttile; plot(lon, lat, ['-' mk], 'MarkerSize', 3, 'Color', [0.2 0.7 0.5]);
    title('GPS Track'); xlabel('Lon'); ylabel('Lat'); grid on; axis equal;

    nexttile; plot(t, alt, ['-' mk], 'MarkerSize', 3, 'Color', [0.4 0.7 0.3]);
    title('Altitude'); ylabel('m'); grid on;

    nexttile; plot(t, sats, ['-' mk], 'MarkerSize', 3, 'Color', [0.3 0.8 0.5]);
    title('Satellites'); ylabel('#'); grid on;

    sgtitle('Telemetry Dashboard', 'FontSize', 16, 'FontWeight', 'bold');

    % ── Save plots if requested ────────────────────────────────
    if savePlots
        [~, name] = fileparts(csvFile);
        exportgraphics(fig1, name + "_acceleration.png", 'Resolution', 200);
        exportgraphics(fig2, name + "_gyroscope.png",    'Resolution', 200);
        exportgraphics(fig3, name + "_speed.png",        'Resolution', 200);
        exportgraphics(fig4, name + "_temperature.png",  'Resolution', 200);
        exportgraphics(fig5, name + "_force.png",        'Resolution', 200);
        exportgraphics(fig6, name + "_gps.png",          'Resolution', 200);
        exportgraphics(fig7, name + "_dashboard.png",    'Resolution', 200);
        fprintf("Plots saved as %s_*.png\n", name);
    end

    % ── Print summary stats ────────────────────────────────────
    fprintf("\n===== Summary =====\n");
    fprintf("Duration:       %.1f seconds (%.1f minutes)\n", t(end), t(end)/60);
    fprintf("Data points:    %d\n", length(t));
    fprintf("Sample rate:    ~%.1f Hz\n", length(t) / t(end));
    fprintf("Max speed:      %.1f mph\n", max(speedMph));
    fprintf("Max accel:      %.2f g\n", max(abs(accelX)));
    fprintf("Max lateral:    %.2f g\n", max(abs(accelY)));
    fprintf("Max force:      %.1f N (%.1f lbf)\n", max(forceN), max(forceLbf));
    fprintf("Temp range:     %.1f - %.1f °F (object)\n", min(objTempF), max(objTempF));
    fprintf("Avg satellites: %.1f\n", mean(sats));
    fprintf("===================\n");
end
