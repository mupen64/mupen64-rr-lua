#pragma once

#include <Windows.h>
#include <d2d1.h>

/**
 * A presenter is responsible for the following:
 *
 *	- Holding a Direct2D render target and exposing it to its Lua instance
 *via dc()
 *	- Drawing the Direct2D render target graphics to a control provided
 *externally
 *	- Blitting its graphics to arbitrary DCs
 *
 *	The init() method must be called and have succeeded prior to calling
 *other functions.
 *
 *	This class is not thread-safe.
 */
class Presenter {
public:
  /**
   * Destroys the presenter and cleans up its resources
   */
  virtual ~Presenter() = default;

  /**
   * \brief Initializes the presenter
   * \param hwnd The window associated with the presenter. The window must be a
   * child window and have the WS_EX_LAYERED style. \return Whether the
   * operation succeeded
   */
  virtual bool init(HWND hwnd) = 0;

  /**
   * Gets the presenter's DC
   */
  virtual ID2D1RenderTarget *dc() const = 0;

  /**
   * Gets the backing texture size
   */
  virtual D2D1_SIZE_U size() = 0;

  /**
   * \brief Adjusts the provided render target clear color to fit the
   * presenter's clear color \param color The color to adjust \return The
   * nearest clear color which fits the presenter
   */
  virtual D2D1::ColorF adjust_clear_color(const D2D1::ColorF color) const {
    return color;
  }

  /**
   * Begins graphics presentation. Called before any painting happens.
   */
  virtual void begin_present() = 0;

  /**
   * Ends graphics presentation. Called after all painting has happened.
   */
  virtual void end_present() = 0;

  /**
   * \brief Blits the presenter's contents to a DC at the specified position
   * \param hdc The target DC
   * \param rect The target bounds
   * \remark Fully transparent pixels in the presenter buffer will remain
   * unchanged in the target DC
   */
  virtual void blit(HDC hdc, RECT rect) = 0;
};
