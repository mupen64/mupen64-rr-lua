Here you can find frequently asked questions about Mupen64 and its components.

# General

**Q: Why is Fast Forward not going above 60 FPS?**

**A:** Unless you have a really slow machine which is causing the slowness, you've likely selected a faulty audio plugin.

Verify that you aren't using shunyuan’s HLE audio plugin. If you are using another video plugin, check that no settings are enabled in the audio plugin that may indicate synchronization of the emulator to audio (e.g. "Sync game to Audio" in Jabo's). 

**Q: Can I go back a frame?**

**A:** It's possible to rewind by however many frames you want during movie recording or playback.

To do so, you need to enable Seek Savestates in the settings and use either the Seeker (Ctrl + G) or Multi-Frame Advance (Ctrl + \\)

# Piano Roll

**Q: Why is the Piano Roll grayed out and unclickable?**

**A:** Editing functionality is only available during movie recording and when the seek savestate interval is non-0.

Check the settings and change "Savestate interval" to something like 120 or 240.

---

**Q: Why can I click the Piano Roll, but not the joystick?**

**A:** Verify that you have a selection in the list. The joystick only affects selected frames and when there are none, it's grayed out.

If you have a selection but the input groupbox has the caption "Warping...", you will have to wait until the warp modify operation completes to apply further modifications.

---

**Q: Why are some clicks in the seek column being ignored?**

**A:** If more than one controller is active, the Piano Roll may behave weirdly due to it not having dedicated columns for each controller.

Editing multicontroller movies is possible because all controllers' input are flattened into the Piano Roll, but comes with confusing drawbacks such as the playhead being limited to multiples of the controller count.
