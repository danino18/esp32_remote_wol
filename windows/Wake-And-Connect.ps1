$configPath = Join-Path $PSScriptRoot "config.json"
$wakeScript = Join-Path $PSScriptRoot "Wake-Desktop-Internet.py"

if (-not (Test-Path $configPath)) {
    Write-Host "ERROR: config.json was not found."
    Read-Host "Press Enter to close"
    exit 1
}

$config = Get-Content $configPath -Raw | ConvertFrom-Json

$desktopIP = $config.desktop_tailscale_ip
$rdpPort = [int]$config.rdp_port

Write-Host ""
Write-Host "Sending wake command..."

python $wakeScript

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "Wake command failed."
    Read-Host "Press Enter to close"
    exit 1
}

Write-Host ""
Write-Host "Waiting for the remote computer to become available through Tailscale..."

function Test-TcpPort {
    param(
        [string]$ComputerName,
        [int]$Port,
        [int]$TimeoutMs = 1000
    )

    $client = New-Object System.Net.Sockets.TcpClient

    try {
        $connection = $client.ConnectAsync($ComputerName, $Port)

        if ($connection.Wait($TimeoutMs) -and $client.Connected) {
            return $true
        }

        return $false
    }
    catch {
        return $false
    }
    finally {
        $client.Dispose()
    }
}

$deadline = (Get-Date).AddSeconds(90)

while ((Get-Date) -lt $deadline) {

    if (Test-TcpPort -ComputerName $desktopIP -Port $rdpPort) {
        Write-Host ""
        Write-Host "Remote computer is ready."
        Write-Host "Opening Remote Desktop..."

        Start-Process "mstsc.exe" -ArgumentList "/v:${desktopIP}:$rdpPort"
        exit 0
    }

    Write-Host "." -NoNewline
    Start-Sleep -Seconds 2
}

Write-Host ""
Write-Host ""
Write-Host "The computer did not become available within 90 seconds."
Read-Host "Press Enter to close"
exit 1
