param([int]$First=1,[int]$Last=18)
$ErrorActionPreference = 'Stop'
$toolsRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Call of Duty Black Ops III 455130'
$gameRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Call of Duty Black Ops III'
if (Get-Process BlackOps3 -ErrorAction SilentlyContinue) { throw 'Close BO3 before installing stress fixtures.' }
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$fixtureManifest = @()
$manifestPath = Join-Path $workspaceRoot 'dev/stress-fixtures-1.2.0.json'
if (Test-Path -LiteralPath $manifestPath) { $fixtureManifest = @(Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json) }
$priorGame = $env:TA_GAME_PATH
$priorTools = $env:TA_TOOLS_PATH
$priorCache = $env:TA_LOCAL_ASSET_CACHE
try {
    $env:TA_GAME_PATH = $toolsRoot + '\'
    $env:TA_TOOLS_PATH = $toolsRoot + '\'
    $env:TA_LOCAL_ASSET_CACHE = (Join-Path $toolsRoot 'share/assetconvert') + '\'
    for ($fixture = $First; $fixture -le $Last; $fixture++) {
        $name = 'stack_stress_' + $fixture
        $modRoot = Join-Path $toolsRoot ('mods/' + $name)
        $rawRoot = Join-Path $modRoot ('bo3stackstress/' + $fixture)
        $sourceRoot = Join-Path $modRoot 'zone_source'
        New-Item -ItemType Directory -Path $rawRoot,$sourceRoot,(Join-Path $modRoot 'zone') -Force | Out-Null
        $source = @()
        $assetCount = if ($fixture -le 6) { 1000 } else { 1 }
        for ($entry = 0; $entry -lt $assetCount; $entry++) {
            $leaf = 'raw' + $entry.ToString('D4') + '.txt'
            [IO.File]::WriteAllText((Join-Path $rawRoot $leaf), 'Workshop Stack isolated capacity fixture ' + $fixture + '/' + $entry)
            $source += 'rawfile,bo3stackstress/' + $fixture + '/' + $leaf
        }
        $source | Set-Content -LiteralPath (Join-Path $sourceRoot 'core_mod.zone') -Encoding ascii
        Push-Location -LiteralPath $toolsRoot
        try {
            & (Join-Path $toolsRoot 'bin/linker_modtools.exe') -language english -fs_game $name -modsource core_mod
            if ($LASTEXITCODE -ne 0) { throw "Fixture $fixture failed to link." }
        } finally { Pop-Location }
        $zoneRoot = Join-Path $modRoot 'zone'
        if (!(Test-Path -LiteralPath (Join-Path $zoneRoot 'core_mod.ff'))) { throw "Fixture $fixture fastfile missing." }
        @{PublisherID=('stack_stress_' + $fixture);Title=('Stack stress fixture ' + $fixture);Description='Local generated test data; not a gameplay mod or release payload.';FolderName=$name;Type='mod';Tags='Mod'} |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $zoneRoot 'workshop.json') -Encoding utf8
        $localRoot = Join-Path $gameRoot ('mods/' + $name)
        if (Test-Path -LiteralPath $localRoot) { throw "Existing local path: $localRoot" }
        New-Item -ItemType Directory -Path $localRoot | Out-Null
        Copy-Item -LiteralPath $zoneRoot -Destination $localRoot -Recurse
        $fixtureManifest += @{name=$name;path=$localRoot;raw_assets=$assetCount;sha256=(Get-FileHash -LiteralPath (Join-Path $zoneRoot 'core_mod.ff')).Hash.ToLowerInvariant()}
    }
} finally {
    $env:TA_GAME_PATH = $priorGame
    $env:TA_TOOLS_PATH = $priorTools
    $env:TA_LOCAL_ASSET_CACHE = $priorCache
}
$fixtureManifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $workspaceRoot 'dev/stress-fixtures-1.2.0.json') -Encoding utf8
