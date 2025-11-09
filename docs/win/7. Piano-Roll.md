# Introduction

The Piano Roll is the flagship feature of Mupen64 1.1.9 and allows seeking through movies visually and editing inputs prior to the current frame.

![grafik](https://github.com/user-attachments/assets/0889b3e4-6201-40d4-b09e-860c13479c1b)

# Structure

The Piano Roll is divided into 3 main sections:

## List

The list is comprised of columns representing the playhead, the frame index, controller joystick and buttons, and rows containing the input data for a specific frame.   

To move the playhead, click or drag in the first column. The piano roll will become gray and non-interactable until the emulator has reached the desired frame.

To change the selection, click on either the "Frame", "X", or "Y" columns while optionally holding <kbd>Ctrl</kbd> or <kbd>Shift</kbd>.

To modify the buttons, left- or right-click any of the button cells.


## Joystick

The joystick allows modifying the joystick values in the current selection. 

If there's no selection or the Piano Roll is in read-only mode, the joystick won't be interactable and display grayed out.

## History

The history listbox contains snapshots of the piano roll's state at certain points in time. Such "states" are comprised of the input buffer, the selection, and the selected frame.

Users can't remove items from the history stack, as it's managed automatically. The maximum size of the stack is 100 by default, but can be changed in the settings.

Selecting an item in the history listbox will restore the accompanying piano roll state.

Utilizing Undo/Redo changes the selection in the history listbox. 

## Context Menu

The Piano Roll has a feature-rich context menu accessible by right-clicking while hovering over any non-button column.






