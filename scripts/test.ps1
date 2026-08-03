$ErrorActionPreference = 'Stop'

$compiler = (Get-Command gcc -ErrorAction Stop).Source
New-Item -ItemType Directory -Path 'build' -Force | Out-Null
& $compiler -std=c11 -O0 -g -Wall -Wextra -Wpedantic -Wconversion -Iinclude `
    tests/test_repository.c src/instrument.c src/input.c src/repository.c `
    -o build/lab-manager-tests.exe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& .\build\lab-manager-tests.exe
exit $LASTEXITCODE
