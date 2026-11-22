
$ErrorActionPreference = "Stop"

$camera = "ACER HD User Facing"
$distro = "Ubuntu"
$port = 5000
$receiver = $null

Write-Host "=== FOTORUKA ===" -ForegroundColor Cyan

try {
    Write-Host "Compiling C program..." -ForegroundColor Yellow

    wsl.exe -d $distro -- bash -lc 'cd ~/new_project-1/pc && make stream_test'

    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed"
    }

    Write-Host "Starting TCP receiver..." -ForegroundColor Yellow

    $receiver = Start-Process -FilePath "wsl.exe" `
        -ArgumentList "-d Ubuntu -- bash -lc 'cd ~/new_project-1/pc && ./stream_test'" `
        -PassThru `
        -NoNewWindow

    Start-Sleep -Seconds 2

    if ($receiver.HasExited) {
        throw "Receiver exited unexpectedly"
    }

    Write-Host "Starting camera..." -ForegroundColor Green
    Write-Host "Press Ctrl+C to stop." -ForegroundColor Cyan

    & ffmpeg -hide_banner -loglevel error `
        -rtbufsize 100M `
        -f dshow `
        -video_size 640x480 `
        -framerate 30 `
        -vcodec mjpeg `
        -i "video=$camera" `
        -pix_fmt rgb24 `
        -f rawvideo `
        "tcp://127.0.0.1:$port"
}
catch {
    Write-Host "ERROR: $_" -ForegroundColor Red
}
finally {
    Write-Host "Stopping Fotoruka..." -ForegroundColor Yellow

    wsl.exe -d $distro -- pkill -x stream_test 2>$null

    if ($receiver -and -not $receiver.HasExited) {
        Stop-Process -Id $receiver.Id -Force -ErrorAction SilentlyContinue
    }

    Write-Host "Fotoruka stopped." -ForegroundColor Green
}
