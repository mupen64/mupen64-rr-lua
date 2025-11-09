# Interface

## Pause when unfocused

Pauses emulation when the main window isn't in focus.

## Lua Presenter

The presenter implementation used for displaying and capturing Lua graphics.

### DirectComposition (default)

Uses [DirectComposition](https://learn.microsoft.com/en-us/windows/win32/directcomp/directcomposition-portal) to composite Lua graphics to the screen. This approach allows true transparency and has excellent performance characteristics.

### GDI

Uses GDI to composite Lua graphics to the screen. This option is only offered for compatibility purposes, as it has undesirable performance characteristics and limited functionality.

## Silent Mode

Suppresses all modal dialogs while choosing reasonable defaults on multi-choice ones.

Recommended in automation scenarios, such as encoding bots.

## Keep working directory

Keeps the working directory specified by the calling program at startup.

When disabled, the working directory is set to the executable's containing folder at startup.

## Use Async Executor

Uses the new task queue-based Async Executor implementation.

On - Lower interaction latency, longer queue processing times when actions are spammed
Off - Higher interaction latency

# Statusbar

## Zero-index

> [!NOTE]  
> This option is only kept around for powerusers. Note that the indicies disabled when it's enabled don't represent the real state of the VCR engine and may cause confusion when combined with other tools, such as the Piano Roll.

Shows indicies in the statusbar in the statusbar relative to 0 instead of 1.

## Scale down to fit window

Allows the statusbar to scale down its segments to fit inside the window.

## Scale up to fill window

Allows the statusbar to scale up its segments to fill the window.

