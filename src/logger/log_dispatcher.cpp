#include "log_dispatcher.hpp"

namespace horizon {
LogDispatcher::LogDispatcher()
{
    dispatcher.connect([this] {
        if (!handler)
            return;
        std::list<Logger::Item> buf;
        {
            std::lock_guard<std::mutex> guard(mutex);
            buf.splice(buf.begin(), items);
        }
        for (const auto &it : buf) {
            handler(it);
        }
    });
}

void LogDispatcher::set_handler(Logger::log_handler_t h)
{
    handler = h;
}

void LogDispatcher::log(const Logger::Item &item)
{
    {
        std::lock_guard<std::mutex> guard(mutex);
        items.push_front(item);
    }
    dispatcher.emit();
}

} // namespace horizon