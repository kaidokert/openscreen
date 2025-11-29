###

[Current State Analysis](CURRENT_STATE.md) | [Refactoring Plan](REFACTORING_PLAN.md)

The objective is to port `osp_demo` to Windows.

Our strategy is **layered**, relying heavily on stubs, hacks, and isolation.

**The Golden Rule:** `autoninja -C out\debug` (no target specified) must **ALWAYS** succeed. If a change breaks the default build, it is wrong. Revert or conditionalize it immediately.

#### The Battle Plan (5 Layers)

**Layer 0: Platform & Base (The Foundation)**
- Verify `compile_smoketest` targets (`logging`, `task_runner`, `time`, `udp_socket`).
- If these fail, stop. Fix them first.

**Layer 1: Third-Party Isolation (The Supply Lines)**
- Verify `//third_party/getopt` and `//third_party/quiche` compile on Windows.
- Use `compile_smoketest` entries to isolate these. If `getopt` fails, stub it or fix it before moving up.

**Layer 2: Discovery & Networking (The Hidden Beast)**
- Verify `//discovery:dnssd` compiles.
- This pulls in heavy networking logic. Isolate failures here before they pollute OSP.

**Layer 3: OSP Core (The Logic)**
- Verify `//osp:osp` (`impl` + `public`) compiles.
- Link the protocol logic. Use `#ifdef _WIN32` stubs liberally. Mark stubs as `OSP_UNIMPLEMENTED`.

**Layer 4: The Demo (The Summit)**
- Link `//osp:osp_demo`.
- **Technique: "The Hollow Demo"**. Modify `osp_demo.cc` to wrap the real logic in `#ifndef _WIN32`.
- Provide a minimal, "Platform Init Only" `main` for Windows initially.
- This proves we can link without debugging runtime crashes yet.

#### Execution Tactics
1.  **GN Generation:** Stub out dependencies in `BUILD.gn` files to get `gn gen` to pass.
2.  **Compilation:** Isolate hard compilation errors into tiny `compile_smoketest` targets. Fix them there, then generalize.
3.  **Linking:** Provide stub implementations (structs/functions) for missing symbols.
4.  **Runtime:** Slowly fill in the "Hollow Demo" with real logic.

Use `git diff main` frequently to track the extent of your hacks.
