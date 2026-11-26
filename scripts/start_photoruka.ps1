$ErrorActionPreference = "Stop"

$cam1Name = "ACER HD User Facing"
$cam2Name = "Logi C270 HD WebCam"   # Залиш порожнім для тестового потоку 1280x720; коли приїде InnoMaker — впиши її назву сюди

$distro = "Ubuntu"
$cam1Port = 5000
$cam2Port = 5001

$receiver = $null
$ffCam1 = $null
$ffCam2 = $null

Write-Host "=== PHOTORUKA (Dual Camera) ===" -ForegroundColor Cyan

try {
    Write-Host "Getting WSL IP address..." -ForegroundColor Yellow
    $ipOutput = wsl.exe -d $distro -- hostname -I
    if ($LASTEXITCODE -ne 0) { throw "Failed to get WSL IP address" }

    $wslIp = ($ipOutput.Trim() -split '\s+')[0]
    if (-not $wslIp) { throw "WSL IP address is empty" }
    Write-Host "WSL IP: $wslIp" -ForegroundColor Green

    Write-Host "Compiling photoruka..." -ForegroundColor Yellow
    wsl.exe -d $distro -- bash -lc 'cd ~/new_project-1/pc && make photoruka'
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed" }

    Write-Host "Starting PHOTORUKA receiver..." -ForegroundColor Yellow
    $receiver = Start-Process -FilePath "wsl.exe" `
        -ArgumentList "-d $distro -- bash -lc 'cd ~/new_project-1/pc && ./photoruka'" `
        -PassThru `
        -NoNewWindow

    Start-Sleep -Seconds 2
    if ($receiver.HasExited) { throw "Receiver exited unexpectedly" }

    Write-Host "Starting CAM1 ($cam1Name) -> port $cam1Port..." -ForegroundColor Green
    $cam1Args = @(
        "-hide_banner", "-loglevel", "error",
        "-rtbufsize", "100M",
        "-f", "dshow",
        "-i", "`"video=$cam1Name`"",
        "-vf", "scale=640:480",
        "-r", "30",
        "-pix_fmt", "rgb24",
        "-f", "rawvideo",
        "tcp://${wslIp}:$cam1Port"
    )
    $ffCam1 = Start-Process -FilePath "ffmpeg" -ArgumentList $cam1Args -PassThru -NoNewWindow

    if ([string]::IsNullOrWhiteSpace($cam2Name)) {
        Write-Host "Starting CAM2 (TEST STREAM 1280x720@30) -> port $cam2Port..." -ForegroundColor Magenta
        $cam2Args = @(
            "-hide_banner", "-loglevel", "error",
            "-re",
            "-f", "lavfi",
            "-i", "testsrc=size=1280x720:rate=30",
            "-pix_fmt", "rgb24",
            "-f", "rawvideo",
            "tcp://${wslIp}:$cam2Port"
        )
    } else {
        Write-Host "Starting CAM2 ($cam2Name) -> port $cam2Port..." -ForegroundColor Green
        $cam2Args = @(
            "-hide_banner", "-loglevel", "error",
            "-rtbufsize", "100M",
            "-f", "dshow",
            "-i", "`"video=$cam2Name`"",
            "-vf", "scale=1280:720",
            "-r", "30",
            "-pix_fmt", "rgb24",
            "-f", "rawvideo",
            "tcp://${wslIp}:$cam2Port"
        )
    }
    $ffCam2 = Start-Process -FilePath "ffmpeg" -ArgumentList $cam2Args -PassThru -NoNewWindow

    Write-Host "Both streams started. Press Ctrl+C to stop." -ForegroundColor Cyan

    while (-not $receiver.HasExited) {
        Start-Sleep -Milliseconds 300
    }
}
catch {
    Write-Host "ERROR: $_" -ForegroundColor Red
}
finally {
    Write-Host "Stopping PHOTORUKA..." -ForegroundColor Yellow

    foreach ($proc in @($ffCam1, $ffCam2)) {
        if ($proc -and -not $proc.HasExited) {
            Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        }
    }

    wsl.exe -d $distro -- pkill -x photoruka 2>$null

    if ($receiver -and -not $receiver.HasExited) {
        Stop-Process -Id $receiver.Id -Force -ErrorAction SilentlyContinue
    }

    Write-Host "PHOTORUKA stopped." -ForegroundColor Green
}