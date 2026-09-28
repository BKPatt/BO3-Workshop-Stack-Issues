$ErrorActionPreference = 'Stop'
if (Get-Process BlackOps3 -ErrorAction SilentlyContinue) { throw 'Exit BO3 before retiring fixtures.' }
$workspaceRoot=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$gameModsRoot=(Resolve-Path 'C:\Program Files (x86)\Steam\steamapps\common\Call of Duty Black Ops III\mods').Path
$archiveRoot=[IO.Path]::GetFullPath((Join-Path $workspaceRoot 'research/priority-fixtures-1.1.0-tested'))
if (!$archiveRoot.StartsWith($workspaceRoot+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Archive escaped workspace.' }
$moves=foreach ($name in @('stack_priority_a','stack_priority_b')) {
    $source=[IO.Path]::GetFullPath((Join-Path $gameModsRoot $name))
    $destination=[IO.Path]::GetFullPath((Join-Path $archiveRoot $name))
    if (!$source.StartsWith($gameModsRoot+'\',[StringComparison]::OrdinalIgnoreCase) -or !$destination.StartsWith($archiveRoot+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected fixture target.' }
    if (!(Test-Path -LiteralPath $source)) { continue }
    if ((Resolve-Path -LiteralPath $source).Path -ne $source -or ((Get-Item -LiteralPath $source).Attributes -band [IO.FileAttributes]::ReparsePoint) -or (Test-Path -LiteralPath $destination)) { throw 'Unexpected fixture redirection or existing archive.' }
    @{source=$source;destination=$destination}
}
New-Item -ItemType Directory -Path $archiveRoot -Force | Out-Null
foreach ($move in $moves) { Move-Item -LiteralPath $move.source -Destination $move.destination }
@{version='1.1.0';removed_from_game=@($moves);published=$false} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'priority-fixture-cleanup-1.1.0.json') -Encoding utf8
$progressPath=Join-Path $PSScriptRoot 'compatibility-progress-1.1.0.json'
$progress=Get-Content -LiteralPath $progressPath -Raw | ConvertFrom-Json
$progress.fixtures_retired=$true
$progress | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $progressPath -Encoding utf8
Write-Output ('Retired '+@($moves).Count+' owned priority fixtures.')
