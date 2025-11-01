param(
  [string]$UE5Dir = $env:UE5_DIR,
  [string]$Project = "$PSScriptRoot/../Gaia.uproject",
  [string[]]$Filters
)

if (-not $UE5Dir -or -not (Test-Path $UE5Dir)) {
  $UE5Dir = "C:\\Program Files\\Epic Games\\UE_5.5"
}

$Project = (Resolve-Path $Project).Path

# Ensure VCPKG_ROOT if default location exists
if (-not $env:VCPKG_ROOT) {
  $defaultVcpkg = Join-Path $env:USERPROFILE 'vcpkg'
  if (Test-Path $defaultVcpkg) {
    $env:VCPKG_ROOT = $defaultVcpkg
  }
}

Write-Host "Using UE5 at: $UE5Dir" -ForegroundColor Cyan
Write-Host "Project: $Project" -ForegroundColor Cyan
if ($env:VCPKG_ROOT) { Write-Host "VCPKG_ROOT: $env:VCPKG_ROOT" -ForegroundColor Cyan }

function Stage-CGAL-DLLs {
  try {
    if (-not $env:VCPKG_ROOT) { return }
    $dllSource = Join-Path $env:VCPKG_ROOT 'installed/x64-windows/bin'
    if (-not (Test-Path $dllSource)) { return }
    $dllTarget = Join-Path $PSScriptRoot '..\Plugins\GaiaPTP\Binaries\Win64'
    if (-not (Test-Path $dllTarget)) { New-Item -ItemType Directory -Path $dllTarget | Out-Null }
    $needed = @('gmp-10.dll','gmpxx-4.dll','mpfr-6.dll')
    foreach ($name in $needed) {
      $src = Join-Path $dllSource $name
      if (Test-Path $src) { Copy-Item -Force $src (Join-Path $dllTarget $name) }
    }
  } catch {}
}

# Stage before build (may not exist yet; directory is created if missing)
Stage-CGAL-DLLs

# Ensure vcpkg bin is on PATH for delay-loaded DLL resolution
if ($env:VCPKG_ROOT) {
  $bin = Join-Path $env:VCPKG_ROOT 'installed/x64-windows/bin'
  if (Test-Path $bin) { $env:PATH = "$bin;" + $env:PATH }
}

$build = Join-Path $UE5Dir "Engine/Build/BatchFiles/Build.bat"
& $build GaiaEditor Win64 Development -Project="$Project" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }

# Stage after build (ensure DLLs present if plugin binaries just got created)
Stage-CGAL-DLLs

$editorCmd = Join-Path $UE5Dir "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"

function Invoke-Test($filter) {
  $args = @(
    '"' + $Project + '"',
    '-unattended','-nop4','-nosplash','-nullrhi',
    '-ExecCmds="Automation RunTests ' + $filter + '; Quit"'
  )
  $p = Start-Process -FilePath $editorCmd -ArgumentList $args -PassThru -WindowStyle Hidden
  $exited = $p.WaitForExit(240000)
  if (-not $exited) {
    try { Stop-Process -Id $p.Id -Force } catch {}
    throw "UnrealEditor-Cmd timeout for filter '$filter'"
  }
  if ($p.ExitCode -ne 0) { throw "Tests failed for filter '$filter' with exit code $($p.ExitCode)" }
}

if (-not $Filters -or $Filters.Count -eq 0) {
  $Filters = @(
    "GaiaPTP.Settings.Defaults",
    "GaiaPTP.Component.DefaultsCopied",
    "GaiaPTP.Fibonacci.CountAndRadius",
    "GaiaPTP.Fibonacci.UniformityBins",
    "GaiaPTP.Data.Defaults",
    "GaiaPTP.Data.PlateVelocity",
    "GaiaPTP.Seeding.Basic",
    "GaiaPTP.Adjacency.Smoke",
    "GaiaPTP.Adjacency.Integrity"
    ,"GaiaPTP.Determinism.Sampling"
    ,"GaiaPTP.Determinism.Seeding"
  )
}

foreach ($f in $Filters) {
  Invoke-Test $f
}

exit 0
