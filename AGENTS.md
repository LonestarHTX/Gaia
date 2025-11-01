# Repository Guidelines

## Project Structure & Module Organization
- `Gaia.uproject` — project descriptor.
- `Source/Gaia/` — runtime C++ module; `Source/GaiaEditor/` — editor‑only code.
- `Plugins/<Name>/Source/` — optional plugins (runtime/editor modules).
- `Content/` — assets (`.uasset`, `.umap`); keep large files in Git LFS.
- `Config/` — project settings (`Default*.ini`).
- Generated: `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` (do not commit).

## Build, Test, and Development Commands
Set `UE5_DIR` to your Unreal Engine root.
- Build (Win64, Editor): `"%UE5_DIR%\Engine\Build\BatchFiles\Build.bat" GaiaEditor Win64 Development -Project="%CD%\Gaia.uproject" -WaitMutex`
- Launch Editor: `"%UE5_DIR%\Engine\Binaries\Win64\UnrealEditor.exe" "%CD%\Gaia.uproject"`
- Run automation tests (headless): `"%UE5_DIR%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\Gaia.uproject" -unattended -nop4 -nosplash -ExecCmds="Automation RunTests Gaia; Quit"`
- Package (overview): use `RunUAT BuildCookRun` with your target platform; keep artifacts under `Releases/`.
 - One-liner build + test: `PowerShell ./Scripts/BuildAndTest-PTP.ps1`

## Coding Style & Naming Conventions
- C++: 4‑space indent, one type per file (`.h/.cpp`).
- Unreal prefixes: `U`/`A`/`F`/`I`/`E` for classes, actors, structs, interfaces, enums. Booleans start with `b` (e.g., `bIsActive`).
- Types/methods: PascalCase; variables: camelCase; macros/constants: UPPER_SNAKE_CASE.
- Use `UPROPERTY`/`UFUNCTION` with clear categories (e.g., `Category="Gaia|Subsystem"`).
- If a `.clang-format` exists, run it before committing; otherwise follow Unreal coding standard.

## Testing Guidelines
- Use Unreal Automation Testing Framework (Specs or Simple tests).
- Place tests under `Source/<Module>/Private/Tests/` (or `.../Specs/`).
- Name suites with `GaiaPTP.*` to leverage the filter above (e.g., `GaiaPTP.Settings.Defaults`).
- Add a test for each bug fix; prefer fast, deterministic tests.
- In‑Editor: Window → Developer Tools → Session Frontend → Automation.

## Agent Workflow (Required)
- Always code, then build, then run tests. After any change under `Plugins/GaiaPTP/`, run `./Scripts/BuildAndTest-PTP.ps1` (or the commands above).
- Use `Project Settings → Gaia → PTP` to adjust:
  - `NumSamplePoints` (simulation truth)
  - `DebugDrawStride` (only affects visualization)
- Visualization uses `RealtimeMeshComponent`; do not switch to slower components.
- Never “optimize” by lowering simulation size to pass tests; throttle debug draw instead.
- If build or tests fail, stop and fix before proceeding. Update or add Automation tests for new features.

## Commit & Pull Request Guidelines
- Use Conventional Commits: `feat:`, `fix:`, `chore:`, `refactor:`, `test:`, etc. Scope optional (e.g., `feat(editor): ...`).
- Branch names: `feature/<scope>` or `fix/<scope>`.
- PRs must include: summary, rationale, testing steps, risk/rollback, and screenshots/video for UX changes. Link issues.

## Security & Configuration Tips
- Do not commit secrets; prefer environment variables or per‑user config in `Saved/Config/` (excluded).
- Track large asset types with Git LFS: `git lfs install && git lfs track "*.uasset" "*.umap" "*.ubulk" "*.uexp"`.

## Agent‑Specific Notes
- Never modify or commit generated folders listed above.
- Keep changes minimal and aligned with module boundaries.
- Update this file if build/test commands or paths change.
