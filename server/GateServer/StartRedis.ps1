$ErrorActionPreference = 'Stop'

function Test-RedisPort {
    $client = [System.Net.Sockets.TcpClient]::new()
    try {
        $result = $client.BeginConnect('127.0.0.1', 6379, $null, $null)
        if (-not $result.AsyncWaitHandle.WaitOne(300)) {
            return $false
        }
        $client.EndConnect($result)
        return $true
    }
    catch {
        return $false
    }
    finally {
        $client.Dispose()
    }
}

if (Test-RedisPort) {
    exit 0
}

$redisCommand = Get-Command redis-server -ErrorAction SilentlyContinue
if ($redisCommand) {
    $redisServer = $redisCommand.Source
}
else {
    $redisServer = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" `
        -Filter redis-server.exe -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}

if (-not $redisServer) {
    throw 'redis-server.exe was not found. Install taizod1024.redis-windows-fork with winget.'
}

Start-Process -FilePath $redisServer -ArgumentList 'redis.local.conf' `
    -WorkingDirectory $PSScriptRoot -WindowStyle Hidden

for ($attempt = 0; $attempt -lt 20; $attempt++) {
    Start-Sleep -Milliseconds 250
    if (Test-RedisPort) {
        exit 0
    }
}

throw 'Redis did not start on 127.0.0.1:6379.'
