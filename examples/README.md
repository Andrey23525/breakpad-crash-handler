# Examples

Two minimal examples showing the crash handler used end to end.

## Client

`client/main.cpp` is the "protected application" side. It registers with the
crash handler over a named pipe using Breakpad's out-of-process
`ExceptionHandler`, then crashes on purpose.

Build it from the repository root (see the top-level README):

```powershell
cmake --build build --target example_client
```

Run the handler first, then the client:

```powershell
# terminal 1 — the handler
.\examples\handler\run_handler.ps1

# terminal 2 — the client (crashes and pops the handler window)
.\build\example_client.exe \\.\pipe\example_crash
```

The handler shows its window and writes the minidump into the directory set by
`run_handler.ps1` (default `%TEMP%\example_dumps`).

## Handler usage

`handler/run_handler.ps1` launches the built `crash_handler.exe` with a named
pipe and a dump directory. Adjust `-Pipe`, `-Dumps`, and `-AppName` to match
your integration. A clean exit of the client produces no window; a crash or an
abnormal exit makes the handler show its window.
