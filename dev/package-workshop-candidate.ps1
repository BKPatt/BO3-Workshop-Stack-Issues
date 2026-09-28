param(
    [string]$ToolsRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Call of Duty Black Ops III 455130',
    [string]$Version = '1.1.1'
)
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Use a numeric semantic version.' }
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$zoneRoot = Join-Path $ToolsRoot 'mods/bo3_workshop_stack/zone'
$prefixes = @('bp','ea','en','es','fr','ge','it','ja','po','ru','sc','tc')
$manifest = @('bo3_stack.dll','core_mod.ff','LICENSE','MinHook-LICENSE.txt','THIRD_PARTY.md','workshop.json') + @($prefixes | ForEach-Object { $_ + '_core_mod.ff' })
$unexpected = @(Get-ChildItem -LiteralPath $zoneRoot -Force | Where-Object { $_.PSIsContainer -or $_.Name -notin $manifest })
if ($unexpected.Count) { throw ('Unexpected candidate files: ' + ($unexpected.Name -join ', ')) }
foreach ($name in $manifest) {
    $path = Join-Path $zoneRoot $name
    if (!(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Item -LiteralPath $path).Length -eq 0) { throw "Missing or empty candidate file: $name" }
}
$expectedDll = (Get-FileHash -LiteralPath (Join-Path $workspaceRoot 'native/build/Release/bo3_stack.dll')).Hash
if ((Get-FileHash -LiteralPath (Join-Path $zoneRoot 'bo3_stack.dll')).Hash -ne $expectedDll) { throw 'Workshop candidate DLL differs from the native release build.' }
$sums = @($manifest | ForEach-Object { '{0}  {1}' -f (Get-FileHash -LiteralPath (Join-Path $zoneRoot $_)).Hash.ToLowerInvariant(), $_ })
$distRoot = Join-Path $workspaceRoot 'dist'
New-Item -ItemType Directory -Path $distRoot -Force | Out-Null
$archive = Join-Path $distRoot "BO3-Workshop-Stack-$Version-Workshop-Candidate.zip"
Compress-Archive -LiteralPath @($manifest | ForEach-Object { Join-Path $zoneRoot $_ }) -DestinationPath $archive -Force
$sums | Set-Content -LiteralPath (Join-Path $distRoot "BO3-Workshop-Stack-$Version-Workshop-Candidate-SHA256SUMS.txt") -Encoding ascii
Write-Output "Candidate archive only; nothing uploaded: $archive"
