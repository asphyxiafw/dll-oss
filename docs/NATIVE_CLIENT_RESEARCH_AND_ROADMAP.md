# Native Bedrock Client: Research, Architecture, and Roadmap

**Status:** research and build scaffold; not a verified in-game client yet.

## Product target

Build a native Windows Minecraft Bedrock client-side mod that runs inside the existing Bedrock process. It is not a replacement game engine, launcher, resource/behavior pack, or standalone overlay.

Initial feature priorities:
1. Reliable native bootstrap and an explicit supported-version check.
2. A modular UI/HUD with configurable settings and keybinds.
3. Mid-game resource-pack switching, with safe reload handling and clear recovery if the game must recreate rendering resources.
4. Performance work inspired by Sodium's goals, but only after profiling identifies measurable bottlenecks. Do not claim FPS gains without repeatable before/after measurements.
5. Diagnostics, configuration persistence, version adapters, and tests that make updates maintainable.
6. A third-party module API only after the core lifecycle is stable.

No anti-cheat bypass is a project goal. Do not disguise incompatible builds as supported, and do not invent signatures, offsets, or function layouts.

## Candidate research (public repositories checked 2026-10-18)

| Project | What it is | Useful evidence | Limitations / risks | Role in this project |
|---|---|---|---|---|
| [Flarial dll-oss](https://github.com/flarialmc/dll-oss) | Native Windows client-side DLL/mod with DirectX hooks, a module/event system, HUD and ClickGUI. | The public CMake project builds a shared library; source includes DX11/DX12 swapchain hooks, module registration, texture hooks, and a PackChanger module. Our fork's Windows MSVC workflow successfully built `Flarial.dll`. | Its README explicitly says important portions are private and public signatures/offsets may lag a Minecraft version. A successful compile does not prove injection or runtime compatibility. AGPL-3.0 applies to the public source. | **Current experimental scaffold only.** Reuse and modify under the applicable license; do not mistake the upstream feature set for newly implemented project features. |
| [Selaura](https://github.com/selauraclient/selaura) | Native C++ Bedrock client-side modding framework/client, with a source-defined API and platform-specific proxy. | Public README says Windows 10/11 x64 and Android/Linux targets; CMake requires C++23 and CMake 3.28; GPL-3.0. Its source separates public API headers and implementation. | Last repository push visible in GitHub metadata was 2025-11-09; current Bedrock-version compatibility and a reproducible Windows build have not yet been verified in this project. | **Architecture reference and alternate foundation to evaluate.** Do not switch until a clean build and target-version path are demonstrated. |
| [LeviLamina](https://github.com/LiteLDev/LeviLamina) | Modular Bedrock mod loader, historically focused on the dedicated server, with client-install documentation as well. | Public README describes a C++ mod loader and API; repository is LGPL-3.0 for its open-source portions. | It is a loader/ecosystem rather than a ready-made client UX; client-side feature coverage and current Windows client compatibility need separate verification. Its EULA/usage rules must be read before adoption. | Reference for loader/API design, not the default base. |
| [Kestrel](https://github.com/Falcon-MC/Kestrel) | A standalone reimplementation of a Bedrock client. | Its README describes its own renderer/networking and native mods. | It is a different product architecture: it renders and connects as its own client rather than extending the user's installed Minecraft process. | Not the target architecture for this project. |

## Current scaffold: facts versus unknowns

Verified:
- The fork builds a Release DLL on a Windows MSVC GitHub Actions runner.
- The source contains DX11 and DX12 render-hook paths, a module/event architecture, and a PackChanger implementation.
- The PackChanger already attempts to coordinate pack reload with render/resource lifecycle events. This is upstream-derived code, not a feature newly implemented by this project.

Not verified:
- DLL injection/loading into the target Minecraft process.
- Current game version and rendering backend compatibility.
- Whether the ClickGUI opens and accepts input in a live game.
- Whether pack switching completes safely on the current game version.
- Any real performance improvement.

## Architecture rules

1. **Bootstrap and compatibility:** identify the running executable/game version and renderer; resolve only signatures/offsets with version-specific evidence; fail closed with a readable diagnostic if unsupported.
2. **Hook layer:** keep hooks thin, document each hook's purpose and expected call contract, and centralize version-specific addresses/layouts.
3. **Runtime API:** expose validated events/services to modules instead of letting every feature read arbitrary game memory.
4. **UI/input:** keep UI and input handling on the correct game/render thread; settings and keybinds are user-configurable.
5. **Resource lifecycle:** treat pack reload as a state machine (request → suspend unsafe rendering work → reload → rebuild/recreate dependent resources → restore rendering → report success/failure). Never claim success solely because a reload event fired.
6. **Performance:** collect frame-time and CPU/GPU evidence first. Make one change at a time and compare equivalent scenes/settings; avoid unsupported renderer rewrites.
7. **Diagnostics:** record build commit, detected game version/backend, resolved compatibility profile, hook initialization result, and failure reason without collecting account credentials or unrelated personal data.
8. **Release discipline:** each build artifact identifies its source commit and supported game versions. Build success, runtime startup, feature behavior, and stability are separate acceptance gates.

## Milestones and acceptance tests

### M0 — Research and reproducible build (in progress)
- Compare candidate codebases, licenses, platform scope, architecture, and update activity.
- Produce a repeatable Windows build and artifact tied to a commit.
- **Pass:** clean CI build; artifact and logs available.

### M1 — Runtime proof
- Add explicit compatibility diagnostics and a minimal non-invasive initialization signal.
- Test load, unload/cleanup, and both supported rendering paths on an actual target game installation.
- **Pass:** documented Minecraft version/backend; no crash on startup or exit; no unsupported-version false positive.

### M2 — UI/input proof
- Verify the existing menu key, input focus, rendering, and module toggling in-game.
- **Pass:** repeatable menu open/close and working input on the supported target.

### M3 — Resource-pack switching
- Audit the existing PackChanger path against the target version, including signature availability, reload sequencing, swapchain/device resource recreation, and failure recovery.
- **Pass:** switch packs while in a world, confirm textures actually change, and survive repeated switches plus world exit/re-entry without a crash.

### M4 — Performance measurement
- Establish frame-time baselines, identify the dominant cost, then implement a narrow optimization.
- **Pass:** repeatable measurements show improvement without visual regressions, memory growth, or crashes.

### M5 — Presentable alpha
- Versioned compatibility report, readable error states, persistent settings, basic documentation, and a release package.
- **Pass:** a fresh setup can identify compatibility, load the client, use the initial features, and recover from unsupported versions.

## Immediate next engineering tasks

1. Audit the current bootstrap and signature manager to see what can be validated before hooks are installed.
2. Verify whether the current source exposes a reliable game-version and renderer/backend query.
3. Inspect the PackChanger state machine and its signature dependencies; do not blindly enable it on a newer game.
4. Add a diagnostics path and tests around pure/version-independent logic before attempting risky runtime changes.
5. Only then select the first implementation change and rebuild it in CI.
