param(
  [string]$UE5Dir = $env:UE5_DIR,
  [string]$Project = "$PSScriptRoot/../Gaia.uproject"
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

Write-Host "`nBuild completed successfully!" -ForegroundColor Green
exit 0
