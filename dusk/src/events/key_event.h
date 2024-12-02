#pragma once

#include "event.h"
#include "core/key_codes.h"

namespace dusk
{
class KeyEvent : public Event
{
public:
    KeyCode getKeyCode() const { return m_keyCode; }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput);

protected:
    KeyEvent(const KeyCode keyCode) :
        m_keyCode(keyCode) { }

    KeyCode m_keyCode;
};

class KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(const KeyCode keyCode, bool isRepeat = false) :
        KeyEvent(keyCode) { }

    std::string toString() const override
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << m_keyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyPressed)
};

class KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(const KeyCode keyCode) :
        KeyEvent(keyCode) { }

    std::string toString() const override
    {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << m_keyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyRepeatEvent : public KeyEvent
{
public:
    KeyRepeatEvent(const KeyCode keyCode) :
        KeyEvent(keyCode) { }

    std::string toString() const override
    {
        std::stringstream ss;
        ss << "KeyRepeatEvent: " << m_keyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyRepeat)
};
} // namespace dusk