param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('SINGLE_INVALID', 'THREE_INVALID', 'NO_FRAMES',
                 'WORKER_DELAY', 'HOLD_FRAME', 'RECOVERY_INIT', 'INIT_NO_FRAME')]
    [string]$Scenario,
    [Parameter(Mandatory = $true)][string]$Port,
    [Parameter(Mandatory = $true)][string]$Slug,
    [ValidateRange(10,3600)][int]$Seconds = 40,
    [ValidateRange(0,100)][int]$WorkerIndex = 1,
    [ValidateRange(0,3600)][int]$AfterFirstValidS = 0,
    [ValidateRange(1000,60000)][int]$HoldMs = 6000
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$prefix = Join-Path $repoRoot "reports/001-camera/06-us3-$Slug"
Push-Location $repoRoot
try {
    & ./tools/set_fault_scenario.ps1 -Scenario $Scenario -WorkerIndex $WorkerIndex `
        -AfterFirstValidS $AfterFirstValidS -HoldMs $HoldMs
    & C:/Projects/esp-idf-v5.5.2/export.ps1 | Out-Null
    python C:/Projects/esp-idf-v5.5.2/tools/idf.py build *> "$prefix-build.txt"
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $prefix-build.txt" }
    if (-not (Select-String -Path sdkconfig -Pattern "^CONFIG_BRINGUP_FAULT_SCENARIO_${Scenario}=y$" -Quiet)) {
        throw "Generated sdkconfig did not select $Scenario"
    }
    if ($Scenario -eq 'HOLD_FRAME' -and
        -not (Select-String -Path sdkconfig -Pattern "^CONFIG_BRINGUP_FAULT_HOLD_MS=$HoldMs$" -Quiet)) {
        throw "Generated sdkconfig did not set hold to $HoldMs ms"
    }
    Copy-Item sdkconfig "$prefix-sdkconfig.txt" -Force
    python C:/Projects/esp-idf-v5.5.2/tools/idf.py -p $Port flash *> "$prefix-flash.txt"
    if ($LASTEXITCODE -ne 0) { throw "Flash failed: $prefix-flash.txt" }
    & C:/Users/Asus/.espressif/python_env/idf5.5_py3.14_env/Scripts/python.exe `
        tools/capture_serial.py --port $Port --seconds $Seconds --output "$prefix.log"
    if ($LASTEXITCODE -ne 0) { throw "Capture failed: $prefix.log" }
    Write-Output "Captured ${Scenario}: $prefix.log"
} finally {
    Pop-Location
}
