$ErrorActionPreference = 'Stop'
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (Get-Process -Name BlackOps3 -ErrorAction SilentlyContinue) { throw 'Exit BO3 normally before restoring display settings.' }
$progressPath = Join-Path $PSScriptRoot 'compatibility-progress-1.1.0.json'
$progress = Get-Content -LiteralPath $progressPath -Raw | ConvertFrom-Json
if ($progress.helper_pid) {
    $testHelper = Get-Process -Id $progress.helper_pid -ErrorAction SilentlyContinue
    if ($testHelper) {
        $expectedHelper = (Resolve-Path (Join-Path $workspaceRoot 'research/t7patch-3.04/t7patch_3.04.Windows.Only/t7dwidm_protect.exe')).Path
        if ($testHelper.Path -ne $expectedHelper) { throw 'The test-helper PID belongs to a different executable.' }
        Stop-Process -Id $testHelper.Id
    }
}
$configPath = 'C:\Program Files (x86)\Steam\steamapps\common\Call of Duty Black Ops III\players\config.ini'
$saved = Get-Content -LiteralPath (Join-Path $workspaceRoot $progress.display_backup) -Raw | ConvertFrom-Json
$original = [IO.File]::ReadAllText($configPath)
$restored = $original
foreach ($key in @('FullScreenMode','WindowSize','AutoRestoreFullscreen')) {
    $pattern = '(?m)^(' + [regex]::Escape($key) + '\s*=\s*")[^"]*(")'
    if ([regex]::Matches($restored,$pattern).Count -ne 1) { throw "Expected one display key: $key" }
    $value = $saved.$key
    $restored = [regex]::Replace($restored,$pattern, { param($match) $match.Groups[1].Value + $value + $match.Groups[2].Value })
}
[IO.File]::WriteAllText($configPath,$restored,[Text.UTF8Encoding]::new($false))
if ([IO.File]::ReadAllText($configPath) -ne $restored) { throw 'Display restore verification failed.' }
$progress.game_pid = $null
$progress.helper_pid = $null
$progress.display_restore_pending = $false
$progress | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $progressPath -Encoding utf8
@{version='1.1.0';game_exited=$true;owned_t7_helper_stopped=$true;restored_keys=$saved;other_config_text_preserved=$true;published=$false} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'compatibility-cleanup-1.1.0.json') -Encoding utf8
Write-Output 'Restored the three saved display settings and stopped the owned test helper.'
