param(
    [string]$GameRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Call of Duty Black Ops III',
    [string]$ToolsRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Call of Duty Black Ops III 455130',
    [switch]$SkipLocalInstall,
    [string[]]$Languages = @('english','french','italian','spanish','german','portuguese','russian','polish','japanese','traditionalchinese','simplifiedchinese','englisharabic')
)
$ErrorActionPreference = 'Stop'
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$linker = Join-Path $ToolsRoot 'bin/linker_modtools.exe'
if (!(Test-Path -LiteralPath $linker)) { throw 'Install the base BO3 Mod Tools through Steam first.' }
$modRoot = Join-Path $ToolsRoot 'mods/bo3_workshop_stack'
$zoneRoot = Join-Path $modRoot 'zone'
if (!$SkipLocalInstall -and (Get-Process BlackOps3 -ErrorAction SilentlyContinue)) { throw 'Close BO3 before installing a local test build, or use -SkipLocalInstall.' }
if (!$Languages.Count) { throw 'At least one build language is required.' }
$localizedOutputs = @()
New-Item -ItemType Directory -Path $zoneRoot -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $workspaceRoot 'workshop/ui') -Destination $modRoot -Recurse -Force
Copy-Item -LiteralPath (Join-Path $workspaceRoot 'workshop/zone_source') -Destination $modRoot -Recurse -Force
$oldGamePath = $env:TA_GAME_PATH
$oldToolsPath = $env:TA_TOOLS_PATH
$oldAssetCache = $env:TA_LOCAL_ASSET_CACHE
try {
    $env:TA_GAME_PATH = $ToolsRoot.TrimEnd('\','/') + '\'
    $env:TA_TOOLS_PATH = $ToolsRoot.TrimEnd('\','/') + '\'
    $env:TA_LOCAL_ASSET_CACHE = (Join-Path $ToolsRoot 'share/assetconvert') + '\'
    Push-Location -LiteralPath $ToolsRoot
    try {
        & (Join-Path $ToolsRoot 'gdtdb/gdtdb.exe') /update
        if ($LASTEXITCODE -ne 0) { throw "Mod Tools asset database update failed with code $LASTEXITCODE" }
        foreach ($language in $Languages) {
            $buildOutput = & $linker -language $language -fs_game bo3_workshop_stack -modsource core_mod 2>&1
            $linkExit = $LASTEXITCODE
            $buildOutput | Write-Output
            if ($linkExit -ne 0) { throw "BO3 linker failed for $language with code $linkExit" }
            $languageZone = [regex]::Matches(($buildOutput | Out-String), 'Linking "([a-z]{2}_core_mod)"')
            if ($languageZone.Count -ne 1) { throw "Expected one localized startup fastfile for $language." }
            $localizedFile = $languageZone[0].Groups[1].Value + '.ff'
            if (!(Test-Path -LiteralPath (Join-Path $zoneRoot $localizedFile))) { throw "Missing localized output: $localizedFile" }
            $localizedOutputs += $localizedFile
        }
    } finally { Pop-Location }
} finally {
    $env:TA_GAME_PATH = $oldGamePath
    $env:TA_TOOLS_PATH = $oldToolsPath
    $env:TA_LOCAL_ASSET_CACHE = $oldAssetCache
}
Copy-Item -LiteralPath (Join-Path $workspaceRoot 'native/build/Release/bo3_stack.dll') -Destination $zoneRoot -Force
foreach ($name in @('LICENSE','THIRD_PARTY.md')) {
    Copy-Item -LiteralPath (Join-Path $workspaceRoot $name) -Destination $zoneRoot -Force
}
Copy-Item -LiteralPath (Join-Path $workspaceRoot 'native/third_party/minhook/LICENSE.txt') -Destination (Join-Path $zoneRoot 'MinHook-LICENSE.txt') -Force
$metadataPath = Join-Path $zoneRoot 'workshop.json'
Copy-Item -LiteralPath (Join-Path $workspaceRoot 'workshop/preview.png') -Destination (Join-Path $modRoot 'preview.png') -Force
$publisherId = '0'
if (Test-Path -LiteralPath $metadataPath) {
    $publisherId = (Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json).PublisherID
}
@{
    PublisherID = $publisherId
    Title = 'BO3 Workshop Stack (Experimental)'
    Description = (Get-Content -LiteralPath (Join-Path $workspaceRoot 'workshop/description.txt') -Raw)
    Thumbnail = (Join-Path $modRoot 'preview.png')
    Type = 'mod'
    FolderName = 'bo3_workshop_stack'
    Tags = 'Mod,UI,WIP,Zombies'
} | ConvertTo-Json | Set-Content -LiteralPath $metadataPath -Encoding utf8
$publishFiles = @('bo3_stack.dll','core_mod.ff','LICENSE','MinHook-LICENSE.txt','THIRD_PARTY.md','workshop.json') + $localizedOutputs
$unexpectedFiles = @(Get-ChildItem -LiteralPath $zoneRoot -Force | Where-Object { $_.PSIsContainer -or $_.Name -notin $publishFiles })
if ($unexpectedFiles.Count) { throw ('Unexpected files in publish folder; move them out before uploading: ' + ($unexpectedFiles.Name -join ', ')) }
if (!$SkipLocalInstall) {
    $installedRoot = Join-Path $GameRoot 'mods/bo3_workshop_stack'
    New-Item -ItemType Directory -Path $installedRoot -Force | Out-Null
    Copy-Item -LiteralPath $zoneRoot -Destination $installedRoot -Recurse -Force
    Write-Output "Installed local mod: $installedRoot"
}
Write-Output "Built publish folder: $zoneRoot"
