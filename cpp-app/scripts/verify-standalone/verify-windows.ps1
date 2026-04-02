param(
    [Parameter(Mandatory=$true)]
    [string]$targetExe
)

if (-not (Test-Path $targetExe)) {
    Write-Error "Target executable not found: $targetExe"
    exit 1
}

# Ensure dumpbin is in the PATH
if (-not (Get-Command dumpbin -ErrorAction SilentlyContinue)) {
    Write-Error "dumpbin.exe not found in PATH. Please run this from a Visual Studio Developer Command Prompt."
    exit 1
}

Write-Host "Verifying standalone status for: $targetExe" -ForegroundColor Cyan

# Get unique DLLs, skipping API sets immediately
$deps = dumpbin /dependents $targetExe | Select-String "\.dll" | 
        ForEach-Object { $_.ToString().Trim().ToLower() } | 
        Where-Object { $_ -notmatch "^(api|ext)-ms-win-" } | Sort-Object -Unique

$allClean = $true

foreach ($dll in $deps) {
    # Locate file in System32 or SysWOW64
    $path = "C:\Windows\System32\$dll", "C:\Windows\SysWOW64\$dll" | Where-Object { Test-Path $_ } | Select-Object -First 1

    if (-not $path) {
        Write-Host "[-] $dll - Missing from System32/SysWOW64 (Potential non-system dependency)" -ForegroundColor Yellow
        $allClean = $false
        continue
    }

    # Validate against Windows naming rules
    $v = (Get-Item $path).VersionInfo
    $isWin = ($v.ProductName -match "Microsoft. Windows. Operating. System|Windows®") -or ($v.ProductName -eq "Internet Explorer")

    if (-not $isWin) {
        Write-Host "[!] $dll - Non-Windows: '$($v.ProductName)' [$($v.CompanyName)]" -ForegroundColor Red
        $allClean = $false
    } else {
        Write-Host "[+] $dll - System OK" -ForegroundColor Green
    }
}

if ($allClean) {
    Write-Host "`nSUCCESS: Binary appears to be fully self-contained (only system DLLs detected)." -ForegroundColor Green
} else {
    Write-Host "`nWARNING: Potential non-system dependencies detected." -ForegroundColor Yellow
}
