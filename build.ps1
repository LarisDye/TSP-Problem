param([switch]$Test)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force build | Out-Null
    & g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static src/main.cpp -o build/tsp_sa.exe -lgdi32 -luser32
    if ($LASTEXITCODE -ne 0) { throw 'C++ build failed' }
    if ($Test) {
        & g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static tests/test_cpp.cpp -o build/test_cpp.exe
        if ($LASTEXITCODE -ne 0) { throw 'Test build failed' }
        & ./build/test_cpp.exe
        if ($LASTEXITCODE -ne 0) { throw 'C++ tests failed' }
        & python -m unittest -v
        if ($LASTEXITCODE -ne 0) { throw 'Python tests failed' }
        & python scripts/check_cli.py
        if ($LASTEXITCODE -ne 0) { throw 'Command-line integration tests failed' }
    }
} finally { Pop-Location }
