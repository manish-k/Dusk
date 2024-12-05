#pragma once

#include "dusk.h"

#include <functional>
#include <iostream>
#include <sstream>

namespace dusk
{
/**
 * @brief Supported event types for windos, app, keyboard and mouse
 */
enum class EventType
{
    None = 0,

    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,

    AppTick,
    AppUpdate,
    AppRender,

    KeyPressed,
    KeyReleased,
    KeyRepeat,

    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled
};

/**
 * @brief Bit representation for each event category
 */
enum EventCategory
{
    None                     = 0,
    EventCategoryApplication = BIT(0),
    EventCategoryInput       = BIT(1),
    EventCategoryKeyboard    = BIT(2),
    EventCategoryMouse       = BIT(3),
    EventCategoryMouseButton = BIT(4)
};

// Macro for defining functions of event class
#define EVENT_CLASS_TYPE(type)                                                    \
    static EventType    getStaticType() { return EventType::type; }               \
    virtual EventType   getEventType() const override { return getStaticType(); } \
    virtual const char* getName() const override { return #type; }

// Macro for defining function for event class category
#define EVENT_CLASS_CATEGORY(category) \
    virtual int getCategoryFlags() const override { return category; }

/**
 * @brief Event base class for defining different category of events
 * in the system
 */
class Event
{
public:
    virtual ~Event() = default;

    /**
     * @brief Get event type of the event
     * @return Possible EventType of the event
     */
    virtual EventType getEventType() const = 0;

    /**
     * @brief Get name of the event
     * @return name c string
     */
    virtual const char* getName() const          = 0;

    /**
     * @brief Get category flag as per bit defined in EventCategory enum
     * @return Flag value
     */
    virtual int         getCategoryFlags() const = 0;

    /**
     * @brief Get event name
     * @return name std::string
     */
    virtual std::string toString() const { return getName(); }

    /**
     * @brief Check if event is of give category
     * @param category to check against
     * @return true if category matches else false
     */
    bool isInCategory(EventCategory category) { return getCategoryFlags() & category; }

    /**
     * @brief Check if event is already handled
     * @return true if handled else false
     */
    bool isHandled() const { return m_handled; }

    /**
     * @brief Set the handled status of the event. If event is handled then it will not
     * be propogated further.
     * @param handledStatus true to mark event as handled else set it to false for further
     * propogation
     */
    void setHandled(bool handledStatus) { m_handled = handledStatus; }

protected:
    bool m_handled = false;
};

/**
 * @brief Dispatcher class to handle event listeners
 */
class EventDispatcher
{
public:
    EventDispatcher(Event& event) :
        m_event(event) { }

    /**
     * @brief Call the function when a particular event is received
     * @tparam T type of the event
     * @tparam F function to invoke on event
     * @param func to invoke on event
     * @return true if we want to mark event as handled else return false for continuing
     * propogation
     */
    template <typename T, typename F>
    bool dispatch(const F& func)
    {
        if (m_event.getEventType() == T::getStaticType())
        {
            m_event.setHandled(m_event.isHandled() | func(*(T*)&m_event));
            return true;
        }
        return false;
    }

private:
    Event& m_event;
};

inline std::ostream& operator<<(std::ostream& os, const Event& e) { return os << e.toString(); }

using EventCallbackFn = std::function<void(Event&)>;
} // namespace dusk