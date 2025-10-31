param(
  [string]$VcpkgRoot = $env:VCPKG_ROOT,
  [string]$Triplet = "x64-windows"
)

if (-not $VcpkgRoot) {
  $VcpkgRoot = Join-Path $env:USERPROFILE 'vcpkg'
}

$root = Join-Path $PSScriptRoot '..\Plugins\GaiaPTP\ThirdParty\PTP_CGAL'
$root = (Resolve-Path $root).Path
$build = Join-Path $root 'build'
New-Item -Force -ItemType Directory -Path $build | Out-Null

$toolchain = Join-Path $VcpkgRoot 'scripts\buildsystems\vcpkg.cmake'

# Clear cache if present
$cache = Join-Path $build 'CMakeCache.txt'
if (Test-Path $cache) { Remove-Item $cache -Force }

$prefix = Join-Path $VcpkgRoot ("installed/" + $Triplet)
cmake -S "$root" -B "$build" -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DVCPKG_TARGET_TRIPLET=$Triplet -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$prefix"
cmake --build "$build" --config Release --target ptp_cgal -- /m

Write-Host "ptp_cgal built. Look for ptp_cgal.lib under: $build" -ForegroundColor Cyan
