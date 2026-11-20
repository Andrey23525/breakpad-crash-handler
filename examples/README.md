# Examples

Both platforms use the same invocation:

```text
example_client <crash_handler executable> <dump directory>
```

The client starts the handler and deliberately crashes. The handler writes a
minidump and opens its report window. A graphical Qt session is required.

Windows:
```powershell
cmake --build build --target example_client crash_handler
.\build\example_client.exe .\build\crash_handler.exe "$env:TEMP\example dumps"
```

Linux:
```sh
cmake --build build-linux --target example_client crash_handler
./build-linux/example_client ./build-linux/crash_handler "/tmp/example dumps"
```

Windows uses a unique named pipe and waits up to ten seconds for the handler.
Linux passes an inherited report socket and grants the handler ptrace permission.
Both builds compile client/main.cpp, which uses only crash_handler::Client.
Platform implementations are private to src/client and selected by CMake.

The Windows handler/run_handler.ps1 script remains available for integrations
that start the server separately; the example client now starts its own server.
