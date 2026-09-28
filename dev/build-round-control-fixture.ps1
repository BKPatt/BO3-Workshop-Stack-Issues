param(
    [string]$ToolsRoot = 'C:/Program Files (x86)/Steam/steamapps/common/Call of Duty Black Ops III 455130',
    [string]$GameRoot = 'C:/Program Files (x86)/Steam/steamapps/common/Call of Duty Black Ops III'
)
$ErrorActionPreference = 'Stop'
if (Get-Process BlackOps3 -ErrorAction SilentlyContinue) { throw 'Exit BO3 before installing the fixture.' }
$fixtureName = 'stack_round_control'
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$modRoot = Join-Path $ToolsRoot ('mods/' + $fixtureName)
$installRoot = Join-Path $GameRoot ('mods/' + $fixtureName)
if (Test-Path -LiteralPath $installRoot) { throw 'Fixture already installed; inspect and retire it first.' }
New-Item -ItemType Directory -Force -Path (Join-Path $modRoot 'scripts/zm/gametypes'),(Join-Path $modRoot 'zone_source'),(Join-Path $modRoot 'zone') | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'fixtures/round-control/stack_round_control.gsc') -Destination (Join-Path $modRoot 'scripts/zm/stack_round_control.gsc')
# Preserve the stock server-settings script, adding only an import. Keep copied
# game source in the local Mod Tools folder, outside this repository and releases.
$stock = [IO.File]::ReadAllText((Join-Path $ToolsRoot 'share/raw/scripts/zm/gametypes/_serversettings.gsc'))
[IO.File]::WriteAllText((Join-Path $modRoot 'scripts/zm/gametypes/_serversettings.gsc'), ('#using scripts\zm\stack_round_control;' + "`r`n" + $stock))
@('#include "zm_mod.class"', 'scriptparsetree,scripts/zm/gametypes/_serversettings.gsc', 'scriptparsetree,scripts/zm/stack_round_control.gsc') | Set-Content -LiteralPath (Join-Path $modRoot 'zone_source/zm_mod.zone') -Encoding ascii
$priorGame = $env:TA_GAME_PATH
$priorTools = $env:TA_TOOLS_PATH
$priorCache = $env:TA_LOCAL_ASSET_CACHE
try {
    $env:TA_GAME_PATH = $ToolsRoot + '\'
    $env:TA_TOOLS_PATH = $ToolsRoot + '\'
    $env:TA_LOCAL_ASSET_CACHE = (Join-Path $ToolsRoot 'share/assetconvert') + '\'
    Push-Location -LiteralPath $ToolsRoot
    try {
        & (Join-Path $ToolsRoot 'bin/linker_modtools.exe') -language english -fs_game $fixtureName -modsource zm_mod
        if ($LASTEXITCODE -ne 0) { throw 'Round-control fixture failed to link.' }
    } finally { Pop-Location }
} finally {
    $env:TA_GAME_PATH = $priorGame
    $env:TA_TOOLS_PATH = $priorTools
    $env:TA_LOCAL_ASSET_CACHE = $priorCache
}
@{PublisherID=$fixtureName;Title='Stack Round Control (TEST ONLY)';Description='Local round completion fixture. Crouch + Use, or stack_test_end_round 1 in console. Excluded from releases.';FolderName=$fixtureName;Type='mod';Tags='Mod,Zombies'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $modRoot 'zone/workshop.json') -Encoding utf8
New-Item -ItemType Directory -Path $installRoot | Out-Null
Copy-Item -LiteralPath (Join-Path $modRoot 'zone') -Destination $installRoot -Recurse
Get-FileHash -LiteralPath (Join-Path $installRoot 'zone/zm_mod.ff')
