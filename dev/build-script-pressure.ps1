param([int]$Count=1200)
$ErrorActionPreference = 'Stop'
$toolsRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Call of Duty Black Ops III 455130'
$name = 'stack_stress_scripts'
$modRoot = Join-Path $toolsRoot ('mods/' + $name)
$scriptsRoot = Join-Path $modRoot 'scripts/zm/stackstress'
$sourceRoot = Join-Path $modRoot 'zone_source'
New-Item -ItemType Directory -Path $scriptsRoot,$sourceRoot,(Join-Path $modRoot 'zone') -Force | Out-Null
$source = @('>mode,core','>type,common','#include "core_mod.class"')
for ($entry = 0; $entry -lt $Count; $entry++) {
    $tag = $entry.ToString('D4')
    $leaf = 'script' + $tag + '.gsc'
    [IO.File]::WriteAllText((Join-Path $scriptsRoot $leaf), '#namespace stackstress_' + $tag + ";`nfunction marker() { return; }`n")
    $source += 'scriptparsetree,scripts/zm/stackstress/' + $leaf
}
$source | Set-Content -LiteralPath (Join-Path $sourceRoot 'core_mod.zone') -Encoding ascii
$priorGame = $env:TA_GAME_PATH
$priorTools = $env:TA_TOOLS_PATH
$priorCache = $env:TA_LOCAL_ASSET_CACHE
try {
    $env:TA_GAME_PATH = $toolsRoot + '\'
    $env:TA_TOOLS_PATH = $toolsRoot + '\'
    $env:TA_LOCAL_ASSET_CACHE = (Join-Path $toolsRoot 'share/assetconvert') + '\'
    Push-Location -LiteralPath $toolsRoot
    try {
        & (Join-Path $toolsRoot 'bin/linker_modtools.exe') -language english -fs_game $name -modsource core_mod
        if ($LASTEXITCODE -ne 0) { throw 'Script pressure fixture failed to link.' }
    } finally { Pop-Location }
} finally {
    $env:TA_GAME_PATH = $priorGame
    $env:TA_TOOLS_PATH = $priorTools
    $env:TA_LOCAL_ASSET_CACHE = $priorCache
}
@{PublisherID=$name;Title='Stack script pressure fixture';Description='Generated local test data; not a release payload.';FolderName=$name;Type='mod';Tags='Mod'} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $modRoot 'zone/workshop.json') -Encoding utf8
Write-Output 'Built fixture only. Close BO3 before installing it for the next test.'
