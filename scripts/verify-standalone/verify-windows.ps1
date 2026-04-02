param(
    [Parameter(Mandatory=$true)]
    [string]$targetExe
)

# 1. Validation
if (-not (Test-Path $targetExe)) {
    Write-Host "[!] Error: Target executable not found: $targetExe" -ForegroundColor Red
    exit 1
}

# 2. Environment Check
if (-not (Get-Command dumpbin -ErrorAction SilentlyContinue)) {
    Write-Host "[!] Error: 'dumpbin.exe' not found in PATH." -ForegroundColor Red
    Write-Host "    Please run this script from a 'Developer PowerShell for VS 2022' window." -ForegroundColor Yellow
    exit 1
}

Write-Host "`n>>> Verifying standalone status for: " -NoNewline
Write-Host $targetExe -ForegroundColor Cyan
Write-Host "-------------------------------------------------------------------------------"

# 3. Get immediate dependencies
# Filters: 
# - Trims whitespace
# - Converts to lowercase
# - Skips API-sets and Extensions (api-ms-win-*, ext-ms-win-*)
# - Skips known stable system wrappers (kernel32.dll, user32.dll, etc.)
$deps = dumpbin /dependents $targetExe | Select-String "\.dll" | 
        ForEach-Object { $_.ToString().Trim().ToLower() } | 
        Where-Object { $_ -notmatch "^(api|ext)-ms-win-" } | 
        Sort-Object -Unique

$allClean = $true
$nonSystemDeps = @()

foreach ($dll in $deps) {
    # Locate file in System32 or SysWOW64
    $path = "C:\Windows\System32\$dll", "C:\Windows\SysWOW64\$dll" | Where-Object { Test-Path $_ } | Select-Object -First 1

    if (-not $path) {
        Write-Host "[-] $dll " -NoNewline
        Write-Host "- MISSING from System32/SysWOW64 (Critical: Private or 3rd-party DLL)" -ForegroundColor Red
        $nonSystemDeps += $dll
        $allClean = $false
        continue
    }

    # Validate against Windows naming rules
    $v = (Get-Item $path).VersionInfo
    
    # Heuristic for "System" DLL:
    # 1. Product name matches Windows/Microsoft
    # 2. Company is Microsoft Corporation
    $isWinProduct = ($v.ProductName -match "Microsoft. Windows. Operating. System|Windows®|Internet Explorer")
    $isMSCompany  = ($v.CompanyName -match "Microsoft Corporation")
    
    # Special Check: Runtime Libraries (VCRUNTIME, MSVCP)
    # Even if they are in System32 (installed by redistributables), 
    # we want them linked statically (/MT) for true standalone status.
    if ($dll -match "^(vcruntime|msvcp|concrt|vccorlib|ucrtbase)") {
        Write-Host "[!] $dll " -NoNewline
        Write-Host "- DYNAMIC RUNTIME detected (Linked via /MD instead of /MT?)" -ForegroundColor Red
        $nonSystemDeps += $dll
        $allClean = $false
        continue
    }

    if ($isWinProduct -and $isMSCompany) {
        Write-Host "[+] $dll " -NoNewline
        Write-Host "- System OK" -ForegroundColor Green
    } else {
        Write-Host "[!] $dll " -NoNewline
        Write-Host "- NON-SYSTEM: '$($v.ProductName)' [$($v.CompanyName)]" -ForegroundColor Red
        $nonSystemDeps += $dll
        $allClean = $false
    }
}

# 4. Final Report
Write-Host "-------------------------------------------------------------------------------"
if ($allClean) {
    Write-Host "SUCCESS: Binary appears to be fully self-contained (only core system DLLs detected)." -ForegroundColor Green
} else {
    Write-Host "WARNING: Found $($nonSystemDeps.Count) potential non-standalone dependencies." -ForegroundColor Yellow
    Write-Host "         Ensure all 3rd-party libraries (Boost, etc.) are linked statically." -ForegroundColor Gray
}
Write-Host "`n"
