param([string]$Destination = 'build/3d/package/RollingStone3D', [string]$ExampleDirectory = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$target = [IO.Path]::GetFullPath((Join-Path $root $Destination))
if (Test-Path -LiteralPath $target) { throw 'Choose a new package directory; existing packages are never overwritten.' }
$binary = Join-Path $root 'build/3d_final/bin/RollingStone3D.exe'
if (!(Test-Path -LiteralPath $binary)) { throw 'Run build_3d.cmd Release first.' }
New-Item -ItemType Directory -Path $target | Out-Null
Copy-Item -LiteralPath $binary -Destination $target
Copy-Item -LiteralPath (Join-Path $root 'run_elastic_3d.cmd') -Destination $target
Copy-Item -LiteralPath (Join-Path $root 'run_obstacles_3d.cmd') -Destination $target
New-Item -ItemType Directory -Path (Join-Path $target 'selftests') | Out-Null
Get-ChildItem -LiteralPath (Join-Path $root 'build/3d_final/bin') -Filter 'rs3d_*.exe' | Copy-Item -Destination (Join-Path $target 'selftests')
Copy-Item -LiteralPath (Join-Path $root 'configs3d') -Destination $target -Recurse
New-Item -ItemType Directory -Path (Join-Path $target 'assets') | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'assets/fonts') -Destination (Join-Path $target 'assets/fonts') -Recurse # Noto Sans SC (OFL)
Copy-Item -LiteralPath (Join-Path $root 'assets/textures') -Destination (Join-Path $target 'assets/textures') -Recurse # default terrain textures (CC0)
$docs = Join-Path $target 'docs'
New-Item -ItemType Directory -Path $docs | Out-Null
foreach ($name in @('README.md','DEPENDENCIES.md','VALIDATION.md','SHAPE_SAMPLER.md','ELASTIC_BARRIER.md','ANALYSIS_AND_TIMELINE.md','OBSTACLES_HEATMAP_TIMELINE_VALIDATION.md','OBSTACLE_DISTRIBUTION_FIX.md','ELASTIC_DEMO_FIX.md','FINAL_FIXES_20261113.md','RECHECK_20261113.md')) {
    Copy-Item -LiteralPath (Join-Path $root ('docs/3d/'+$name)) -Destination $docs
}
Copy-Item -LiteralPath (Join-Path $root 'docs/3d/RELEASE_README.md') -Destination (Join-Path $target 'README.md')
# The release documentation is one level below its README; the source docs are two.
$docReadme = [IO.File]::ReadAllText((Join-Path $docs 'README.md')).Replace('(../../README.md)', '(../README.md)')
[IO.File]::WriteAllText((Join-Path $docs 'README.md'),$docReadme,[Text.UTF8Encoding]::new($false))
foreach ($name in @('FINAL_ACCEPTANCE.json','RECHECK_ACCEPTANCE.json')) {
    $acceptance = Join-Path $root ('docs/3d/'+$name)
    if (Test-Path -LiteralPath $acceptance) { Copy-Item -LiteralPath $acceptance -Destination $docs }
}
Copy-Item -LiteralPath (Join-Path $root 'docs/3d/evidence') -Destination (Join-Path $docs 'evidence') -Recurse
if ($ExampleDirectory) {
    $example = [IO.Path]::GetFullPath((Join-Path $root $ExampleDirectory))
    if (!(Test-Path -LiteralPath (Join-Path $example 'FILES_COMPLETE.txt'))) { throw 'Example must be a committed result archive.' }
    New-Item -ItemType Directory -Path (Join-Path $target 'examples') | Out-Null
    Copy-Item -LiteralPath $example -Destination (Join-Path $target 'examples/demo_results') -Recurse
}
Copy-Item -LiteralPath (Join-Path $root 'docs/3d/images') -Destination (Join-Path $docs 'images') -Recurse
$notices = Join-Path $target 'licenses'
New-Item -ItemType Directory -Path $notices | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'licenses/terrain-textures-CC0.txt') -Destination $notices
Copy-Item -LiteralPath (Join-Path $root 'licenses/NotoSansSC-OFL.txt') -Destination $notices
foreach ($vendor in @('JoltPhysics','raylib')) {
    Copy-Item -LiteralPath (Join-Path $root ('.deps3d/'+$vendor+'/LICENSE')) -Destination (Join-Path $notices ($vendor+'.txt'))
}
# Several bundled raylib components carry notices inside their source headers.
# Preserve the complete upstream files instead of guessing which paragraphs apply.
Copy-Item -LiteralPath (Join-Path $root '.deps3d/raylib/src/external') -Destination (Join-Path $notices 'raylib_external') -Recurse
$launcher = @'
@echo off
setlocal
pushd "%~dp0"
if not exist RollingStone3D.exe (
  echo ERROR: RollingStone3D.exe is missing from this package.
  popd
  exit /b 1
)
RollingStone3D.exe %*
set "RS3D_EXIT=%errorlevel%"
popd
exit /b %RS3D_EXIT%
'@
[IO.File]::WriteAllText((Join-Path $target 'run_3d.cmd'),$launcher,[Text.Encoding]::ASCII)
$rows = Get-ChildItem -LiteralPath $target -Recurse -File | ForEach-Object {
    [pscustomobject]@{path=$_.FullName.Substring($target.Length+1);sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
}
$rows | ConvertTo-Json -Depth 3 | Set-Content -Encoding utf8 (Join-Path $target 'package-sha256.json')
Write-Output "Packaged $target"
