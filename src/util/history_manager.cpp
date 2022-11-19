#include "history_manager.hpp"
#include <stdexcept>
#include <assert.h>

namespace horizon {
void HistoryManager::clear()
{
    history.clear();
    history_current = -1;
    s_signal_changed.emit();
}

void HistoryManager::trim()
{
    while (history.size() > history_max) {
        history.pop_front();
        history_current--;
    }
}

void HistoryManager::set_history_max(unsigned int m)
{
    history_max = m;
}

bool HistoryManager::can_redo() const
{
    return history_current + 1 != (int)history.size();
}

bool HistoryManager::can_undo() const
{
    return history_current > 0;
}

static const std::string empty_string;

const std::string &HistoryManager::get_undo_comment() const
{
    if (can_undo()) {
        return history.at(history_current)->comment;
    }
    else {
        return empty_string;
    }
}

const std::string &HistoryManager::get_redo_comment() const
{
    if (can_redo()) {
        return history.at(history_current + 1)->comment;
    }
    else {
        return empty_string;
    }
}

const HistoryManager::HistoryItem &HistoryManager::undo()
{
    if (!can_undo())
        throw std::runtime_error("can't undo");
    history_current--;
    s_signal_changed.emit();
    return get_current();
}

const HistoryManager::HistoryItem &HistoryManager::redo()
{
    if (!can_redo())
        throw std::runtime_error("can't redo");
    history_current++;
    s_signal_changed.emit();
    return get_current();
}

const HistoryManager::HistoryItem &HistoryManager::get_current()
{
    return *history.at(history_current);
}

void HistoryManager::push(std::unique_ptr<const HistoryItem> it)
{
    while (history_current + 1 != (int)history.size()) {
        history.pop_back();
    }
    assert(history_current + 1 == (int)history.size());
    history.push_back(std::move(it));
    history_current++;
    trim();
    s_signal_changed.emit();
}

} // namespace horizon
