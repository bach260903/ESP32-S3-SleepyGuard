param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('NONE', 'PSRAM_UNAVAILABLE', 'PSRAM_MISMATCH',
                 'PSRAM_ALLOCATE', 'PSRAM_READBACK', 'CAMERA_INIT',
                 'SINGLE_INVALID', 'THREE_INVALID', 'NO_FRAMES',
                 'WORKER_DELAY', 'HOLD_FRAME', 'RECOVERY_INIT', 'INIT_NO_FRAME')]
    [string]$Scenario,
    [ValidateRange(0,100)][int]$WorkerIndex = 1,
    [ValidateRange(0,3600)][int]$AfterFirstValidS = 0,
    [ValidateRange(1000,60000)][int]$HoldMs = 6000
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$configPath = Join-Path $repoRoot 'sdkconfig'
if (-not (Test-Path -LiteralPath $configPath)) {
    throw 'Generate sdkconfig with idf.py build before selecting a scenario.'
}

$configLines = Get-Content -LiteralPath $configPath |
    Where-Object { $_ -notmatch '^(# )?CONFIG_BRINGUP_FAULT_' }
if ($Scenario -eq 'NONE') {
    $configLines += '# CONFIG_BRINGUP_FAULT_INJECTION is not set'
} else {
    $configLines += 'CONFIG_BRINGUP_FAULT_INJECTION=y'
    $configLines += "CONFIG_BRINGUP_FAULT_SCENARIO_${Scenario}=y"
    $configLines += "CONFIG_BRINGUP_FAULT_WORKER_INDEX=$WorkerIndex"
    $configLines += "CONFIG_BRINGUP_FAULT_AFTER_FIRST_VALID_S=$AfterFirstValidS"
    if ($Scenario -eq 'HOLD_FRAME') {
        $configLines += "CONFIG_BRINGUP_FAULT_HOLD_MS=$HoldMs"
    }
}
Set-Content -LiteralPath $configPath -Value $configLines -Encoding utf8
Write-Output "Selected fault scenario: $Scenario"
