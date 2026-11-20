param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('PSRAM_UNAVAILABLE', 'PSRAM_MISMATCH', 'PSRAM_ALLOCATE',
                 'PSRAM_READBACK', 'CAMERA_INIT')]
    [string]$Scenario,
    [Parameter(Mandatory = $true)][string]$Port
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$reports = Join-Path $repoRoot 'reports/001-camera'
$slug = $Scenario.ToLowerInvariant().Replace('_', '-')
$prefix = Join-Path $reports "05-us2-$slug"
$expectedCode = switch ($Scenario) {
    'PSRAM_UNAVAILABLE' { 'PSRAM_UNAVAILABLE' }
    'PSRAM_MISMATCH' { 'PSRAM_CONFIG_MISMATCH' }
    'PSRAM_ALLOCATE' { 'PSRAM_TEST_FAILED' }
    'PSRAM_READBACK' { 'PSRAM_TEST_FAILED' }
    'CAMERA_INIT' { 'RECOVERY_EXHAUSTED' }
}
$expectedAttempts = if ($Scenario -eq 'CAMERA_INIT') { 3 } else { 0 }

Push-Location $repoRoot
try {
    & ./tools/set_fault_scenario.ps1 $Scenario
    & C:/Projects/esp-idf-v5.5.2/export.ps1 | Out-Null
    Write-Output "Building $Scenario"
    python C:/Projects/esp-idf-v5.5.2/tools/idf.py build *> "$prefix-build.txt"
    if ($LASTEXITCODE -ne 0) { throw "Build failed; see $prefix-build.txt" }
    if (-not (Select-String -Path sdkconfig -Pattern "^CONFIG_BRINGUP_FAULT_SCENARIO_${Scenario}=y$" -Quiet)) {
        throw "Generated sdkconfig did not select $Scenario"
    }
    Copy-Item sdkconfig "$prefix-sdkconfig.txt" -Force
    Write-Output "Flashing $Scenario on $Port"
    python C:/Projects/esp-idf-v5.5.2/tools/idf.py -p $Port flash *> "$prefix-flash.txt"
    if ($LASTEXITCODE -ne 0) { throw "Flash failed; see $prefix-flash.txt" }
    Write-Output "Capturing $Scenario for 68 seconds"
    & C:/Users/Asus/.espressif/python_env/idf5.5_py3.14_env/Scripts/python.exe `
        tools/capture_serial.py --port $Port --seconds 68 --output "$prefix.log"
    if ($LASTEXITCODE -ne 0) { throw "Capture failed for $Scenario" }
    & ./tools/check_us2_log.ps1 -Path "$prefix.log" -ExpectedCode $expectedCode `
        -ExpectedCameraAttempts $expectedAttempts
} finally {
    Pop-Location
}
