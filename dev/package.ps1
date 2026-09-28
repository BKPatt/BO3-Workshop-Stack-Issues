$ErrorActionPreference = 'Stop'
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$binaryRoot = Join-Path $workspaceRoot 'native/build/Release'
$packageRoot = Join-Path $workspaceRoot 'dist/BO3-Workshop-Stack'
New-Item -ItemType Directory -Path $packageRoot -Force | Out-Null
foreach ($name in @('BO3Stack.exe', 'bo3_stack.dll')) {
    $sourceFile = Join-Path $binaryRoot $name
    $targetFile = Join-Path $packageRoot $name
    if ((Test-Path -LiteralPath $targetFile) -and ((Get-FileHash -LiteralPath $sourceFile).Hash -eq (Get-FileHash -LiteralPath $targetFile).Hash)) { continue }
    Copy-Item -LiteralPath $sourceFile -Destination $targetFile
}
foreach ($name in @('README.md', 'QUICK_START.md', 'COMPATIBILITY.md', 'CHANGELOG.md', 'VERIFICATION.md', 'T7_FIX_REPORT.md', 'CAPACITY_REPORT.md', 'STREAMING_FIX_REPORT.md', 'LICENSE', 'THIRD_PARTY.md')) {
    Copy-Item -LiteralPath (Join-Path $workspaceRoot $name) -Destination (Join-Path $packageRoot $name)
}
Copy-Item -LiteralPath (Join-Path $workspaceRoot 'native/third_party/minhook/LICENSE.txt') -Destination (Join-Path $packageRoot 'MinHook-LICENSE.txt')
@'
@echo off
"%~dp0BO3Stack.exe"
echo.
pause
'@ | Set-Content -LiteralPath (Join-Path $packageRoot 'Start Stack.cmd') -Encoding Ascii
$manifest = @('BO3Stack.exe', 'bo3_stack.dll', 'Start Stack.cmd', 'README.md', 'QUICK_START.md', 'COMPATIBILITY.md', 'CHANGELOG.md', 'VERIFICATION.md', 'T7_FIX_REPORT.md', 'CAPACITY_REPORT.md', 'STREAMING_FIX_REPORT.md', 'LICENSE', 'THIRD_PARTY.md', 'MinHook-LICENSE.txt')
$checksums = foreach ($name in $manifest) {
    $digest = Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $packageRoot $name)
    '{0}  {1}' -f $digest.Hash.ToLowerInvariant(), $name
}
$checksums | Set-Content -LiteralPath (Join-Path $packageRoot 'SHA256SUMS.txt') -Encoding Ascii
$archiveFiles = @($manifest + 'SHA256SUMS.txt' | ForEach-Object { Join-Path $packageRoot $_ })
# Explicit allowlist excludes runtime logs, PDBs, research and Workshop content.
Compress-Archive -LiteralPath $archiveFiles -DestinationPath (Join-Path $workspaceRoot 'dist/BO3-Workshop-Stack.zip') -Force
Write-Output (Join-Path $workspaceRoot 'dist/BO3-Workshop-Stack.zip')
