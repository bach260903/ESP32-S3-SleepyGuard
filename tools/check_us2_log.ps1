param(
    [Parameter(Mandatory = $true)][string]$Path,
    [Parameter(Mandatory = $true)][string]$ExpectedCode,
    [Parameter(Mandatory = $true)][int]$ExpectedCameraAttempts
)

$ErrorActionPreference = 'Stop'
$lines = Get-Content -LiteralPath $Path
$boots = @($lines | Where-Object { $_ -match 'ESP-ROM:' })
$camera = @($lines | Where-Object { $_ -match 'bringup: CAMERA_INIT attempt=' })
$faults = @($lines | Where-Object { $_ -match 'FAULT code=' })
if ($boots.Count -ne 1) { throw "Expected one ROM boot; got $($boots.Count)" }
if ($camera.Count -ne $ExpectedCameraAttempts) {
    throw "Expected $ExpectedCameraAttempts camera init attempts; got $($camera.Count)"
}
for ($i = 0; $i -lt $camera.Count; ++$i) {
    if ($camera[$i] -notmatch "CAMERA_INIT attempt=$($i + 1) result=failure") {
        throw "Unexpected camera init order or result: $($camera[$i])"
    }
}
if ($faults.Count -lt 13) { throw "Expected at least 13 FAULT lines; got $($faults.Count)" }
if ($camera.Count -gt 0) {
    $firstFaultIndex = [array]::IndexOf($lines, $faults[0])
    if (@($lines | Select-Object -Skip ($firstFaultIndex + 1) |
          Where-Object { $_ -match 'bringup: CAMERA_INIT attempt=' }).Count -ne 0) {
        throw 'Camera initialization occurred after entering FAULT'
    }
}

$faultTimes = @()
foreach ($line in $faults) {
    if ($line -notmatch "FAULT code=$ExpectedCode(?:\s|$)") {
        throw "Unexpected fault code: $line"
    }
    if ($line -notmatch '\((\d+)\)') { throw "Missing firmware timestamp: $line" }
    $faultTimes += [int64]$Matches[1]
}

$span = $faultTimes[-1] - $faultTimes[0]
if ($span -lt 60000) { throw "FAULT observation shorter than 60 seconds: $span ms" }
$gaps = @()
for ($i = 1; $i -lt $faultTimes.Count; ++$i) {
    $gap = $faultTimes[$i] - $faultTimes[$i - 1]
    $gaps += $gap
    if ($gap -lt 4900 -or $gap -gt 5100) {
        throw "FAULT report gap outside 5 seconds +/- 100 ms: $gap ms"
    }
}
Write-Output "PASS path=$Path boots=1 camera_attempts=$($camera.Count) fault_code=$ExpectedCode fault_lines=$($faults.Count) fault_span_ms=$span gap_min_ms=$(($gaps|Measure-Object -Minimum).Minimum) gap_max_ms=$(($gaps|Measure-Object -Maximum).Maximum)"
