#include "LabelSet.hpp"

void LabelSet::insert(label_t label)
{
    if (!contains(label))
        container_.push_back(label);
}

void LabelSet::remove(label_t label)
{
    iterator_t it = position_(label);
    if (it != container_.end())
        container_.erase(it);
}

size_t LabelSet::size()
{
    return container_.size();
}

void LabelSet::clear()
{
    container_.clear();
}

bool mergeWith(LabelSet const& other)
{
    size_t initial = size();
    for (auto label : other.container_) {
        insert(label);
    }
    return initial != size();
}

bool mergeWith(LabelSet const* other)
{
    assert(dynamic_cast<LabelSet const*>(other) != nullptr);
    return mergeWith(*dynamic_cast<LabelSet const*>(other));
}

bool LabelSet::contains(label_t label)
{
    return (position_(label) != container_.end());
}

LabelSet::iterator_t LabelSet::position_(label_t label)
{
    return find(container_.begin(), container_.end(), label);
}
