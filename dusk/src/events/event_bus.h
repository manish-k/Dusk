#pragma once

#include "dusk.h"
#include "event.h"

#include <list>

namespace dusk
{
class EventBus
{
public:
    void   push(Unique<Event>& event) { m_eventQueue.push_back(std::move(event)); }

    size_t size() { return m_eventQueue.size(); }

    void   clear() { m_eventQueue.clear(); }

private:
    // TODO: Replace vector with a ring buffer
    std::list<Unique<Event>> m_eventQueue;
};
} // namespace dusk