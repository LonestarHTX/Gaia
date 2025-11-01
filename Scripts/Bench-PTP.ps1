param(
  [string]$UE5Dir = $env:UE5_DIR,
  [string]$Project = "$PSScriptRoot/../Gaia.uproject",
  [int]$Parallel = 1
)

if (-not $UE5Dir -or -not (Test-Path $UE5Dir)) {
  $UE5Dir = "C:\\Program Files\\Epic Games\\UE_5.5"
}
$Project = (Resolve-Path $Project).Path
$editorCmd = Join-Path $UE5Dir "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"

# Ensure vcpkg DLLs are staged for CGAL if needed (no-op if absent)
try {
  if ($env:VCPKG_ROOT) {
    $dllSource = Join-Path $env:VCPKG_ROOT 'installed/x64-windows/bin'
    $dllTarget = Join-Path $PSScriptRoot '..\Plugins\GaiaPTP\Binaries\Win64'
    if (Test-Path $dllSource) {
      if (-not (Test-Path $dllTarget)) { New-Item -ItemType Directory -Path $dllTarget | Out-Null }
      foreach ($n in 'gmp-10.dll','gmpxx-4.dll','mpfr-6.dll') {
        $src = Join-Path $dllSource $n
        if (Test-Path $src) { Copy-Item -Force $src (Join-Path $dllTarget $n) }
      }
    }
  }
} catch {}

$parVal = if ($Parallel -ne 0) { 1 } else { 0 }
$exec = "ptp.profile.start; ptp.parallel $parVal; ptp.bench.rebuild; ptp.profile.stop; Quit"

$args = @(
  '"' + $Project + '"',
  '-unattended','-nop4','-nosplash','-nullrhi',
  '-ExecCmds="' + $exec + '"'
)

Write-Host "Running bench with ptp.parallel=$parVal" -ForegroundColor Cyan
$p = Start-Process -FilePath $editorCmd -ArgumentList $args -PassThru -WindowStyle Hidden
$exited = $p.WaitForExit(240000)
if (-not $exited) { try { Stop-Process -Id $p.Id -Force } catch {}; throw "Bench timeout" }
if ($p.ExitCode -ne 0) { throw "Bench failed with exit code $($p.ExitCode)" }

Write-Host "Bench complete. CSV in Saved/Profiling/CSV" -ForegroundColor Green

