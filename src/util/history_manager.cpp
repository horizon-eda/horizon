#include "history_manager.hpp"
#include <stdexcept>
#include <assert.h>

namespace horizon {
void HistoryManager::clear()
{
    undo_stack.clear();
    redo_stack.clear();
    history_current.reset();
    s_signal_changed.emit();
}

void HistoryManager::trim()
{
    while (undo_stack.size() > history_max) {
        undo_stack.pop_front();
    }
}

void HistoryManager::set_history_max(unsigned int m)
{
    history_max = m;
}

bool HistoryManager::can_redo() const
{
    return redo_stack.size();
}

bool HistoryManager::can_undo() const
{
    return undo_stack.size() && history_current;
}

static const std::string empty_string;

const std::string &HistoryManager::get_undo_comment() const
{
    if (can_undo()) {
        return history_current->comment;
    }
    else {
        return empty_string;
    }
}

const std::string &HistoryManager::get_redo_comment() const
{
    if (can_redo()) {
        return redo_stack.back()->comment;
    }
    else {
        return empty_string;
    }
}

const HistoryManager::HistoryItem &HistoryManager::undo()
{
    if (!can_undo())
        throw std::runtime_error("can't undo");
    redo_stack.push_back(history_current);
    history_current = undo_stack.back();
    undo_stack.pop_back();
    s_signal_changed.emit();
    return get_current();
}

const HistoryManager::HistoryItem &HistoryManager::redo()
{
    if (!can_redo())
        throw std::runtime_error("can't redo");
    undo_stack.push_back(history_current);
    history_current = redo_stack.back();
    redo_stack.pop_back();
    s_signal_changed.emit();
    return get_current();
}

const HistoryManager::HistoryItem &HistoryManager::get_current()
{
    return *history_current;
}

void HistoryManager::push(std::unique_ptr<const HistoryItem> it)
{
    if (history_current)
        undo_stack.push_back(history_current);

    redo_stack.clear();
    history_current = std::move(it);

    trim();
    s_signal_changed.emit();
}

} // namespace horizon
