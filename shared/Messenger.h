#pragma once

#include <any>
#include <functional>

namespace Messenger {
/**
 * \brief Types of messages
 */
enum class Message {
  /**
   * \brief Debug message used for benchmarking which should not be subscribed
   * to.
   */
  None,

  /**
   * \brief The emulator launched state has changed
   */
  EmuLaunchedChanged,

  /**
   * \brief The core executing state has changed
   */
  CoreExecutingChanged,

  /**
   * \brief The emulator is beginning the termination process
   */
  EmuStopping,

  /**
   * \brief The emulator paused state has changed
   */
  EmuPausedChanged,

  /**
   * \brief The video capturing state has changed
   */
  CapturingChanged,

  /**
   * \brief The statusbar visibility has changed
   */
  StatusbarVisibilityChanged,

  /**
   * \brief The main window size changed
   */
  SizeChanged,

  /**
   * \brief The movie loop state changed
   */
  MovieLoopChanged,

  /**
   * \brief The VCR read-only state changed
   */
  ReadonlyChanged,

  /**
   * \brief The VCR task changed
   */
  TaskChanged,

  /**
   * \brief The current VCR sample index changed
   */
  CurrentSampleChanged,

  /**
   * \brief A VCR unfreeze operation has completed
   */
  UnfreezeCompleted,

  /**
   * \brief The VCR warp modify status has changed
   */
  WarpModifyStatusChanged,

  /**
   * \brief The VCR engine has created or destroyed a seek savestate at the
   * specified frame.
   */
  SeekSavestateChanged,

  /**
   * \brief The Lua engine has started a script
   */
  ScriptStarted,

  /**
   * \brief The main window has been created
   */
  AppReady,

  /**
   * \brief The emulator has finished resetting
   */
  ResetCompleted,

  /**
   * \brief The user has requested a reset. The reset won't be performed after
   * the message is handled.
   */
  ResetRequested,

  /**
   * \brief The config will begin saving soon
   */
  ConfigSaving,

  /**
   * \brief The config has been loaded and values have changed
   */
  ConfigLoaded,

  /**
   * \brief The rerecord count of the currently recorded movie changed
   */
  RerecordsChanged,

  /**
   * \brief The currently selected save slot changed
   */
  SlotChanged,

  /**
   * \brief A VCR seek operation has completed
   */
  SeekCompleted,

  /**
   * \brief The seek status has changed.
   */
  SeekStatusChanged,

  /**
   * \brief The core speed modifier has changed
   */
  SpeedModifierChanged,

  /**
   * \brief The threhsold of VIs since the last input poll has been exceeded
   */
  LagLimitExceeded,

  /**
   * \brief The emu has begun or stopped its starting process
   */
  EmuStartingChanged,

  /**
   * \brief The fullscreen mode has changed
   */
  FullscreenChanged,

  /**
   * \brief The audio dacrate has changed
   */
  DacrateChanged,

  /**
   * \brief The core fast-forward flag (g_vr_fast_forward) needs updating.
   */
  FastForwardNeedsUpdate,

  /**
   * \brief The debugger-tracked CPU state has changed
   */
  DebuggerCpuStateChanged,

  /**
   * \brief The CPU resumed state has changed
   */
  DebuggerResumedChanged,
};

using t_user_callback = std::function<void(std::any)>;

/**
 * \brief Initializes the messenger
 */
void init();

/**
 * \brief Broadcasts a message to all listeners
 * \param message The message type
 * \param data The message data
 * \remark This method is thread-safe.
 */
void broadcast(Message message, std::any data);

/**
 * \brief Subscribe to a message
 * \param message The message type to listen for
 * \param callback The callback to be invoked upon receiving the specified
 * message type \return A function which, when called, unsubscribes from the
 * message \remark This method is thread-safe.
 */
std::function<void()> subscribe(Message message, t_user_callback callback);
} // namespace Messenger
