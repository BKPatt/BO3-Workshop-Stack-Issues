$ErrorActionPreference = 'Stop'
$toolsRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Call of Duty Black Ops III 455130'
$gameRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Call of Duty Black Ops III'
if (Get-Process BlackOps3 -ErrorAction SilentlyContinue) { throw 'Exit BO3 before installing fixtures.' }
$priorGame=$env:TA_GAME_PATH; $priorTools=$env:TA_TOOLS_PATH; $priorCache=$env:TA_LOCAL_ASSET_CACHE
try {
    $env:TA_GAME_PATH=$toolsRoot+'\'; $env:TA_TOOLS_PATH=$toolsRoot+'\'; $env:TA_LOCAL_ASSET_CACHE=(Join-Path $toolsRoot 'share/assetconvert')+'\'
    foreach ($letter in @('a','b')) {
        $name='stack_priority_'+$letter
        $modRoot=Join-Path $toolsRoot ('mods/'+$name)
        $localRoot=Join-Path $gameRoot ('mods/'+$name)
        if (Test-Path -LiteralPath $localRoot) { throw "Fixture path already exists: $localRoot" }
        New-Item -ItemType Directory -Force -Path (Join-Path $modRoot 'zone_source'),(Join-Path $modRoot 'zone'),(Join-Path $modRoot 'bo3stackpriority') | Out-Null
        $phases=if ($letter -eq 'a') { @('core_mod') } else { @('core_mod','zm_mod') }
        foreach ($phase in $phases) {
            $leaves=if ($letter -eq 'a') { @('cross','shared') } elseif ($phase -eq 'core_mod') { @('shared') } else { @('cross') }
            $source=@()
            foreach ($leaf in $leaves) {
                [IO.File]::WriteAllText((Join-Path $modRoot ('bo3stackpriority/'+$leaf+'.txt')),($letter.ToUpperInvariant()+'_'+$phase))
                $source+='rawfile,bo3stackpriority/'+$leaf+'.txt'
            }
            $source | Set-Content -LiteralPath (Join-Path $modRoot ('zone_source/'+$phase+'.zone')) -Encoding ascii
            Push-Location -LiteralPath $toolsRoot
            try {
                & (Join-Path $toolsRoot 'bin/linker_modtools.exe') -language english -fs_game $name -modsource $phase
                if ($LASTEXITCODE -ne 0) { throw "Could not link $name/$phase" }
            } finally { Pop-Location }
        }
        @{PublisherID=$name;Title=('Stack priority '+$letter.ToUpperInvariant());Description='Owned inert load-order test fixture; excluded from release.';FolderName=$name;Type='mod';Tags='Mod'} |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $modRoot 'zone/workshop.json') -Encoding utf8
        New-Item -ItemType Directory -Path $localRoot | Out-Null
        Copy-Item -LiteralPath (Join-Path $modRoot 'zone') -Destination $localRoot -Recurse
    }
} finally { $env:TA_GAME_PATH=$priorGame; $env:TA_TOOLS_PATH=$priorTools; $env:TA_LOCAL_ASSET_CACHE=$priorCache }
Write-Output 'Installed two inert owned fixtures for same-phase and cross-phase priority checks.'
