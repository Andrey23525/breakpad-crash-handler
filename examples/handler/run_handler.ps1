# run_handler.ps1
#
# Example launcher for the crash handler. Build crash_handler.exe first (see
# the top-level README), then run this script. The handler stays alive and
# waits for a client to crash; stop it with Ctrl+C or by closing its window.

param(
    [string]$Handler = "build\crash_handler.exe",
    [string]$Pipe = "example_crash",
    [string]$Dumps = "$env:TEMP\example_dumps",
    [string]$AppName = "Example App"
)

New-Item -ItemType Directory -Force $Dumps | Out-Null

# The pipe must be given in the \\.\pipe\<name> form.
& $Handler "--pipe=\\.\pipe\$Pipe" "--dumps=$Dumps" "--app-name=$AppName"
