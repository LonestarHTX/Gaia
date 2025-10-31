param(
  [string]$Triplet = "x64-windows",
  [string]$VcpkgRoot = "$env:USERPROFILE\vcpkg"
)

if (-not (Test-Path $VcpkgRoot)) {
  git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot
}
Push-Location $VcpkgRoot
& .\bootstrap-vcpkg.bat
& .\vcpkg.exe install cgal:$Triplet boost-math:$Triplet mpfr:$Triplet gmp:$Triplet
& .\vcpkg.exe integrate install
Pop-Location

Write-Host "Set VCPKG_ROOT environment variable to: $VcpkgRoot" -ForegroundColor Cyan
Write-Host "Then rebuild the project. GaiaPTPCGAL will detect headers and set WITH_CGAL=1." -ForegroundColor Cyan

