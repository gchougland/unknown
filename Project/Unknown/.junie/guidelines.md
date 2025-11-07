### Project-specific development guidelines (UE 5.6.1)

This project is a C++ Unreal Engine 5.6.1 project with a single runtime module `Unknown` and an editor target `UnknownEditor`. The guidance below focuses on build/config details and an automation testing workflow that has been validated on this project and engine version.

#### Build and configuration
- Engine version and path
  - UE: 5.6.1
  - Typical path (confirmed): `C:\Program Files\Epic Games\UE_5.6`
- Targets and modules
  - Editor target: `Source\UnknownEditor.Target.cs` (Type = Editor, `IncludeOrderVersion = Unreal5_6`).
  - Runtime module: `Source\Unknown\Unknown.Build.cs` with dependencies: `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`.
  - PCH mode: `PCHUsageMode.UseExplicitOrSharedPCHs`.
- Required tooling
  - Visual Studio toolchain as per `.vsconfig` and Rider/VS integration (Windows toolchain with MSVC, Windows 10/11 SDK). Rider is configured for C++/GameDev/Unreal.
- Generating project files (optional)
  - Solution already present: `Unknown.sln`. To regenerate from the uproject, right-click `Unknown.uproject` → “Generate Visual Studio project files” (or via UnrealVersionSelector), or use the UE “Refresh C++ Project” from Editor.
- Clean builds and rebuilds
  - Use Unreal Build Tool (UBT) to force compilation (especially after adding new C++ files like tests):
    ```powershell
    & "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" `
      -Mode=Build `
      -Project="C:\Users\gchou\Documents\Projects\Unknown\Project\Unknown\Unknown.uproject" `
      -Target="UnknownEditor Win64 Development" `
      -WaitMutex -NoHotReloadFromIDE
    ```
  - For a clean: add `-Clean` or delete `Intermediate`/`Binaries` with care (not usually required unless toolchain or module structure changes).

#### Running the Editor (headless for CI/local testing)
- Use `UnrealEditor-Cmd.exe` with headless flags for automation runs:
  ```powershell
  & "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
    "C:\Users\gchou\Documents\Projects\Unknown\Project\Unknown\Unknown.uproject" `
    -unattended -nop4 -NullRHI -NoSound -NoSplash -log `
    -ReportOutputPath="C:\Users\gchou\Documents\Projects\Unknown\Project\Unknown\Saved\AutomationReports\CLI" `
    -ExecCmds='Automation RunTests Project.Smoke.Sample; Quit'
  ```
- Notes:
  - `-NullRHI` avoids GPU requirements; `-unattended`, `-nop4`, `-NoSound`, and `-NoSplash` are CI-friendly.
  - `-ReportOutputPath` writes HTML/JSON reports to the specified folder.
  - Exit code 0 indicates success; non-zero indicates error/failure. Look for `**** TEST COMPLETE. EXIT CODE: 0 ****` in logs.

#### Automation testing
- Where project tests live
  - Place C++ tests under `Source\Unknown\Private\Tests` (or any private folder compiled into the `Unknown` module).
  - File naming: Conventional `*.spec.cpp` (UE does not require the suffix, but it helps organization).
- Minimal test pattern (Simple Automation Test)
  ```cpp
  // Source/Unknown/Private/Tests/MyFeature.spec.cpp
  #if WITH_DEV_AUTOMATION_TESTS
  #include "Misc/AutomationTest.h"

  IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMyFeatureSmokeTest,
      "Project.Smoke.MyFeature",
      EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

  bool FMyFeatureSmokeTest::RunTest(const FString& Parameters)
  {
      TestTrue(TEXT("Sanity"), true);
      return true;
  }
  #endif // WITH_DEV_AUTOMATION_TESTS
  ```
  - Flags:
    - Use `EditorContext` for tests that run in the Editor target.
    - Use `ProductFilter` so tests are visible to default CLI filters (as opposed to `EngineFilter`).
  - After adding a new test file, force a build (UBT command above) before running, otherwise the test won’t be discovered.
- Alternative pattern (Spec-style)
  - For BDD-style tests, use `DEFINE_SPEC`/`BEGIN_DEFINE_SPEC` with `Describe`/`It`. The same flags guidance (EditorContext + ProductFilter) applies.
- Discovering tests
  - List all available tests:
    ```powershell
    & "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
      "C:\Users\gchou\Documents\Projects\Unknown\Project\Unknown\Unknown.uproject" `
      -unattended -nop4 -NullRHI -NoSound -NoSplash -log `
      -ExecCmds='Automation List; Quit'
    ```
  - The log prints total count and names; reports can be exported too.
- Running subsets
  - By test name prefix: `Automation RunTests Project.Smoke` will run all tests under that path.
  - By exact name: `Automation RunTests Project.Smoke.MyFeature`.
  - In-Editor: Window → Developer Tools → Session Frontend → Automation tab (filter by `Project.`).
- Reports and artifacts
  - JSON/HTML reports are written under `Saved\AutomationReports\<tag>` when `-ReportOutputPath` is used; otherwise under `Saved\Automation`.
  - For CI parsing, consume `index.json` in the report folder.

#### Adding new tests — checklist
- [ ] Create `Source/Unknown/Private/Tests/<Name>.spec.cpp`.
- [ ] Guard with `#if WITH_DEV_AUTOMATION_TESTS`.
- [ ] Prefer `EditorContext | ProductFilter` flags for Editor-based tests.
- [ ] Build the `UnknownEditor` target via UBT (or from Rider/VS) to compile the new file.
- [ ] Run via CLI using `UnrealEditor-Cmd.exe` with `-ExecCmds='Automation RunTests <filter>; Quit'`.
- [ ] Verify exit code and check the report folder.

#### Code style and module tips
- Follow Unreal Engine C++ coding standard (PascalCase types, F/U/A prefixes, minimal includes, use forward declarations where possible).
- Respect the project’s `IncludeOrderVersion = Unreal5_6` and PCH strategy; avoid adding heavy includes to shared PCH.
- Keep runtime dependencies in `Unknown.Build.cs` minimal; add private dependencies when possible to reduce compile time.
- Input stack: the module already depends on `EnhancedInput`; prefer Enhanced Input mappings and context assets.

#### Troubleshooting
- Test not found after adding a file
  - Ensure the new file is inside a compiled module path (e.g., `Source/Unknown/Private/...`).
  - Rebuild the Editor target with UBT; hot reload may not pick up new source files for automation discovery.
  - Confirm flags include `ProductFilter` if invoking from CLI with defaults.
- Headless run crashes or exits early
  - Remove `-NullRHI` temporarily if a test depends on rendering or slate widgets; otherwise keep it for speed/stability.
  - Add `-log` and review `Saved\Logs\Unknown.log`.
- Too many assets scanned (slow start)
  - Use a dedicated, minimal test map if maps are required; otherwise, pure unit-style tests avoid map loads.

#### Validated example (executed successfully)
- We validated a smoke test named `Project.Smoke.Sample` in this project by:
  1) Building with UBT: `UnknownEditor Win64 Development`.
  2) Running: `Automation RunTests Project.Smoke.Sample` via `UnrealEditor-Cmd.exe` with `-NullRHI -unattended`.
  3) Observing exit code `0` and report export to `Saved\AutomationReports\CLI`.

This document targets advanced Unreal/C++ developers and focuses on the specifics of this repository (engine version, module layout, UBT commands, and proven test workflow).