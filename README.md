# Build and Run

## Configure the Project

Run this once inside the project root:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
```

## Compile

```powershell
cmake --build build
```

## Run

```powershell
.\\build\\lexor.exe tests\\phase1_lexer.lexor
```

## Rebuild After Changes

After editing source files, rebuild with:

```powershell
cmake --build build
```

## Clean Build Folder

If you encounter generator/configuration issues:

```powershell
Remove-Item -Recurse -Force build
```

Then reconfigure:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
```
