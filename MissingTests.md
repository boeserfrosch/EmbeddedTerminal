# Missing Tests

This file tracks tests that would be useful to add, but are currently missing from the suite.

## Command API Migration

- [ ] Add a compile/runtime test that verifies the README custom-command example uses `ICommand::invoke(CommandInvocation&)` correctly.
- [ ] Add a compile/runtime test that verifies `examples/SimpleCustomCommand` still matches the current `ICommand` API.
- [ ] Add a compile/runtime test that verifies `examples/CustomCommand` still matches the current `ICommand` API.
- [ ] Add a compile/runtime test that verifies `examples/OwnershipExample` still matches the current `ICommand` API.

## Documentation Coverage

- [ ] Add a doc check that keeps the README migration snippet for `trigger(keyword, additional)` -> `invoke(CommandInvocation&)` up to date.

## Interactive Command Behavior

- [ ] Add coverage for a command that returns `Running` and then `WaitingForInput` to ensure `resume()` is documented and exercised.
- [ ] Add coverage for a command that reads from `invocation.streams.input` across multiple chunks.

## Channels & Streams

- [x] Unit tests for `BufferedInput`/`BufferedOutput` verifying chunk boundaries, partial reads/writes, and flush behavior.
- [x] Tests for `BufferedInOut` that exercise simultaneous read/write sequences and stream reopening.
- [x] Test `TeeOutput` to ensure fan-out correctness when one downstream fails or is slow.

## File I/O & Storage

- [x] Unit tests for `FileInput`/`FileOutput` and `ETFile` handling of large files, seek/seek-to-end, and readAll() limits.
- [x] Integration tests for `StorageSystem` + `DirectoryNavigator` covering create/list/read/delete lifecycle across nested directories.
- [ ] Tests for `DefaultStorageMedia` and `StorageMediaAdapter` edge cases (full storage, permission errors, partial writes).

## HAL / Platform Adapters

- [ ] Native/host tests for `NativeFileSystem` and `NativeFile` path edge cases (long paths, unicode, symlinks if applicable).
- [ ] Unit tests for `ConfigurableGpioPolicy`, `DefaultGpioSupport`, and `CompileTimeGpioAuth` to exercise allow/deny rules and configuration parsing.
- [ ] Tests for Arduino/ESP adapter glue (ArduinoFileSystem, ESPIDFFileSystem) where native behavior diverges — run under emulation or mocked HAL.

## Scripting & Runner

- [x] Parser fuzz/regression tests: malformed scripts, unmatched quotes, deeply nested blocks, and large scripts to detect parser stack/limits.
- [x] Runner tests for resume semantics with multiple consecutive `resume()` calls, paused commands that become ready after external input, and error propagation from commands back to the runner.
- [x] Tests for script-level function scoping and variable shadowing across nested scopes.

## Concurrency, Executor & Terminal

- [ ] Tests for `TerminalExecutor` scheduling: long-running commands, preemption, and cancellation behavior.
- [ ] Stress tests that enqueue many short commands to verify queueing, ordering, and memory usage under load.

## Mocks, Test Utilities & Infrastructure

- [ ] Add helper tests that validate the behavior of common mocks (MockFileSystem, MockStream, MockNetworkInterface) so test fixtures stay reliable when refactored.
- [ ] Add a CI/verify test that compiles and links the `examples/` sketches (native or mocked) to prevent future API drift.

## Integration / Regression

- [ ] End-to-end scenario tests combining `Terminal`, `StorageSystem`, and a subset of commands (e.g., create → write → cat → rm) to exercise end-to-end paths.
- [ ] Regression tests that capture previously fixed bugs (e.g., `xxd` split-offset handling, script resume semantics) as automated cases.

## Prioritization Notes

- High: Channels & Streams, Scripting & Runner, File I/O & Storage (these are core runtime paths).
- Medium: HAL adapters, Concurrency/Executor, Integration tests.
- Low: Extended example compile checks and deep fuzzing (valuable but longer-term).

If you'd like, I can start implementing the highest-priority unit tests (Channels & Streams and Runner resume semantics) and add CI checks to compile the examples. Which area should I begin with?
