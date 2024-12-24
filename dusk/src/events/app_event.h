#pragma once

#include "event.h"

namespace dusk
{
class WindowResizeEvent : public Event
{
public:
    WindowResizeEvent(unsigned int width, unsigned int height) :
        m_width(width), m_height(height) { }

    unsigned int getWidth() const { return m_width; }
    unsigned int getHeight() const { return m_height; }

    std::string  toString() const override
    {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << m_width << ", " << m_height;
        return ss.str();
    }

    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
private:
    unsigned int m_width;
    unsigned int m_height;
};

class WindowCloseEvent : public Event
{
public:
    WindowCloseEvent() = default;

    EVENT_CLASS_TYPE(WindowClose)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class WindowIconifiedEvent : public Event
{
public:
    WindowIconifiedEvent(bool iconified) :
        m_iconified(iconified) { };

    bool        isIconified() const { return m_iconified; }

    std::string toString() const override
    {
        std::stringstream ss;
        ss << "WindowIconifiedEvent: status=" << m_iconified;
        return ss.str();
    }

    EVENT_CLASS_TYPE(WindowIconify)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
private:
    bool m_iconified;
};

class AppTickEvent : public Event
{
public:
    AppTickEvent() = default;

    EVENT_CLASS_TYPE(AppTick)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppUpdateEvent : public Event
{
public:
    AppUpdateEvent() = default;

    EVENT_CLASS_TYPE(AppUpdate)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppRenderEvent : public Event
{
public:
    AppRenderEvent() = default;

    EVENT_CLASS_TYPE(AppRender)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};
} // namespace dusk