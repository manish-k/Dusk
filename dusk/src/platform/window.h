#pragma once

#include "dusk.h"
#include "events/event.h"

namespace dusk
{
    class Window
    {
    public:
        enum class Vsync
        {
            Off,
            On,
            Default
        };

        enum class Mode
        {
            Headless,
            Fullscreen,
            FullscreenBorderless,
            Default
        };

        struct Properties
        {
            std::string title = "Dusk";
            uint32_t width = 1600;
            uint32_t height = 900;
            bool resizable = true;
            Mode mode = Mode::Default;
            Vsync vsync = Vsync::Default;

            static Properties defaultWindowProperties() { return Properties{}; }
        };

    public:
        virtual ~Window() = default;

        virtual void onUpdate(float dt) = 0;

        virtual uint32_t getHeight() const = 0;
        virtual uint32_t getWidth() const = 0;
        virtual bool isResized() const = 0;

        virtual void toggleFullScreen() = 0;
        virtual void toggleFullScreenBorderless() = 0;

        virtual void setEventCallback(const EventCallbackFn& cb) = 0;

        virtual void* getNativeWindow() const = 0;

        static Unique<Window> createWindow(Properties& props);
    };
} // namespace dusk