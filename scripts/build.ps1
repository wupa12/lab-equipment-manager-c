$ErrorActionPreference = 'Stop'

$compiler = (Get-Command gcc -ErrorAction Stop).Source
New-Item -ItemType Directory -Path 'build' -Force | Out-Null
& $compiler -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Iinclude `
    src/main.c src/cli.c src/instrument.c src/input.c src/repository.c `
    -o build/lab-manager.exe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host 'Built build/lab-manager.exe'
