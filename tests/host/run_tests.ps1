param(
    [string]$Compiler = 'C:/msys64/mingw64/bin/gcc.exe'
)

$ErrorActionPreference = 'Stop'
$testDir = $PSScriptRoot
$outputDir = Join-Path $testDir '.build'
$smokeExe = Join-Path $outputDir 'test_smoke.exe'
$logicSource = Join-Path $testDir '../../components/bringup_logic/bringup_logic.c'
$logicInclude = Join-Path $testDir '../../components/bringup_logic/include'
$logicObject = Join-Path $outputDir 'bringup_logic.o'

if (-not (Test-Path -LiteralPath $Compiler)) {
    throw "Host C compiler not found: $Compiler"
}
$env:PATH = "$(Split-Path -Parent $Compiler);$env:PATH"
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
Write-Output "HOST_COMPILER=$Compiler"
$version = & $Compiler -dumpfullversion
if ($LASTEXITCODE -ne 0) { throw 'Host compiler could not run' }
Write-Output "HOST_COMPILER_VERSION=$version"
& $Compiler -std=c11 -Wall -Wextra -Werror (Join-Path $testDir 'test_smoke.c') -o $smokeExe
if ($LASTEXITCODE -ne 0) { throw 'Host smoke compilation failed' }
& $smokeExe
if ($LASTEXITCODE -ne 0) { throw 'Host smoke execution failed' }
& $Compiler -std=c11 -Wall -Wextra -Werror -I $logicInclude -c $logicSource -o $logicObject
if ($LASTEXITCODE -ne 0) { throw 'Host logic compilation failed' }
Write-Output 'HOST_LOGIC_COMPILE PASS'
