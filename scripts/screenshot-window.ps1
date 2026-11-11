# screenshot-window.ps1
#
# Captures exactly the visible bounds of the process window to a PNG: no
# desktop margin, no invisible resize border, no drop shadow. Used to snapshot
# the crash-handler window for documentation.
#
#     & ".\scripts\screenshot-window.ps1" -ProcessId 1234 -Output "media\crash_handler.png"

param(
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [Parameter(Mandatory = $true)][string]$Output
)

$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential)]
public struct NativeRect {
    public int Left;
    public int Top;
    public int Right;
    public int Bottom;
}

public static class NativeWindow {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out NativeRect rect);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern int SetProcessDpiAwarenessContext(IntPtr value);

    [DllImport("dwmapi.dll")]
    public static extern int DwmGetWindowAttribute(IntPtr hwnd, int attribute, out NativeRect rect, int size);
}
'@

# Per-monitor V2 DPI awareness so GetWindowRect and CopyFromScreen agree on
# physical pixels.
[NativeWindow]::SetProcessDpiAwarenessContext([IntPtr](-4)) | Out-Null

$script:targetPid = [uint32]$ProcessId
$script:targetWindow = [IntPtr]::Zero

$callback = [NativeWindow+EnumWindowsProc]{
    param([IntPtr]$hWnd, [IntPtr]$lParam)
    $procId = [uint32]0
    [NativeWindow]::GetWindowThreadProcessId($hWnd, [ref]$procId) | Out-Null
    if ($procId -eq $script:targetPid -and [NativeWindow]::IsWindowVisible($hWnd)) {
        $script:targetWindow = $hWnd
        return $false
    }
    return $true
}

[NativeWindow]::EnumWindows($callback, [IntPtr]::Zero) | Out-Null

if ($script:targetWindow -eq [IntPtr]::Zero) {
    throw "No visible window found for process $ProcessId"
}

[NativeWindow]::SetForegroundWindow($script:targetWindow) | Out-Null
Start-Sleep -Milliseconds 400

# DWMWA_EXTENDED_FRAME_BOUNDS (9) is the visible rectangle, excluding the
# invisible resize border. Fall back to GetWindowRect if it is unavailable.
$rect = New-Object NativeRect
$dwmResult = [NativeWindow]::DwmGetWindowAttribute($script:targetWindow, 9, [ref]$rect, 16)
if ($dwmResult -ne 0) {
    [NativeWindow]::GetWindowRect($script:targetWindow, [ref]$rect) | Out-Null
}

$width  = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top

if ($width -le 0 -or $height -le 0) {
    throw "Window rect is empty (${width}x${height})"
}

$bitmap = New-Object System.Drawing.Bitmap($width, $height)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
$graphics.Dispose()

$bitmap.Save($Output, [System.Drawing.Imaging.ImageFormat]::Png)
$bitmap.Dispose()

Write-Output $Output
