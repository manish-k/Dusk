#pragma once

#include "dusk.h"
#include "events/event.h"
#include "core/dtime.h"

namespace dusk
{
/**
 * @brief Window abstract class for interfacing different supported windows
 */
class Window
{
public:
    /**
     * @brief State of vsync
     */
    enum class Vsync
    {
        Off,
        On,
        Default
    };

    /**
     * @brief Supported window modes
     */
    enum class Mode
    {
        Headless,
        Fullscreen,
        FullscreenBorderless,
        Default
    };

    /**
     * @brief Properties needed for creation of window
     */
    struct Properties
    {
        std::string       title     = "Dusk";
        uint32_t          width     = 1600;
        uint32_t          height    = 900;
        bool              resizable = true;
        Mode              mode      = Mode::Default;
        Vsync             vsync     = Vsync::Default;

        static Properties defaultWindowProperties() { return Properties {}; }
    };

public:
    virtual ~Window()                                                 = default;

    /**
     * @brief Update loop of the window
     * @param dt duration in seconds since last frame
     */
    virtual void          onUpdate(TimeStep dt)                       = 0;

    /**
     * @brief Get current height of the window
     * @return height
     */
    virtual uint32_t      getHeight() const                           = 0;

    /**
     * @brief Get current width of the window
     * @return width
     */
    virtual uint32_t      getWidth() const                            = 0;

    /**
     * @brief Get the resized state of the window
     * @return true if window has been resized, otherwise false
     */
    virtual bool          isResized() const                           = 0;

    /**
     * @brief Toggle between fullscreen and default window's size
     */
    virtual void          toggleFullScreen()                          = 0;

    /**
     * @brief Toggle between borderless fullscreen and default window's size
     */
    virtual void          toggleFullScreenBorderless()                = 0;

    /**
     * @brief Set callback to trigger when window events are received
     * @param cb function to call on events
     */
    virtual void          setEventCallback(const EventCallbackFn& cb) = 0;

    /**
     * @brief Get native window pointer
     * @return raw pointer to the window
     */
    virtual void*         getNativeWindow() const                     = 0;

    /**
     * @brief Static function to create a window
     * @param window properties required for creation
     * @return unique pointer to the window
     */
    static Unique<Window> createWindow(Properties& props);
};
} // namespace dusk