param([switch]$Retry)
$ErrorActionPreference = 'Stop'
if (Get-Process BlackOps3 -ErrorAction SilentlyContinue) { throw 'Close BO3 before retiring test fixtures.' }
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$gameModsRoot = (Resolve-Path 'C:\Program Files (x86)\Steam\steamapps\common\Call of Duty Black Ops III\mods').Path
$archiveLeaf = if ($Retry) { 'research/capacity-fixtures-1.2.0-retry-tested' } else { 'research/capacity-fixtures-1.2.0-tested' }
$recordLeaf = if ($Retry) { 'fixture-cleanup-1.2.0-retry.json' } else { 'fixture-cleanup-1.2.0.json' }
$archiveRoot = [IO.Path]::GetFullPath((Join-Path $workspaceRoot $archiveLeaf))
if (!$archiveRoot.StartsWith($workspaceRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Archive escaped the workspace.' }
$fixtures = @(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'stress-fixtures-1.2.0.json') -Raw | ConvertFrom-Json)
$scriptFixture = Join-Path $gameModsRoot 'stack_stress_scripts'
if (Test-Path -LiteralPath $scriptFixture) { $fixtures += @{name='stack_stress_scripts';path=$scriptFixture} }
$moves = foreach ($fixture in $fixtures) {
    if ($fixture.name -notmatch '^stack_stress_(?:[1-9]|1[0-8]|scripts)$') { throw 'Unexpected fixture name.' }
    $sourcePath = [IO.Path]::GetFullPath($fixture.path)
    $expected = Join-Path $gameModsRoot $fixture.name
    if ($sourcePath -ne $expected -or !$sourcePath.StartsWith($gameModsRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture escaped the intended game mods directory.' }
    if (!(Test-Path -LiteralPath $sourcePath)) { continue }
    $resolvedSource = (Resolve-Path -LiteralPath $sourcePath).Path
    if ($resolvedSource -ne $expected -or ((Get-Item -LiteralPath $sourcePath).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Unexpected fixture redirection.' }
    $destination = [IO.Path]::GetFullPath((Join-Path $archiveRoot $fixture.name))
    if (!$destination.StartsWith($archiveRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $destination)) { throw 'Archive target is unsafe or already exists.' }
    @{source=$resolvedSource;destination=$destination}
}
New-Item -ItemType Directory -Path $archiveRoot -Force | Out-Null
foreach ($move in $moves) { Move-Item -LiteralPath $move.source -Destination $move.destination }
@{version='1.2.0';removed_from_game=$moves;published=$false} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot $recordLeaf)
Write-Output ('Retired ' + $moves.Count + ' owned fixtures; preserved in ' + $archiveRoot)
