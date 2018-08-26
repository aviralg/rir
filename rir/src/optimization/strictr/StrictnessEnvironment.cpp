#include "AbstractStrictnessEnvironment.hpp"

AbstractStrictnessEnvironment::AbstractStrictnessEnvironment()
{
}

AbstractStrictnessEnvironment::AbstractStrictnessEnvironment(AbstractStrictnessEnvironment const& from)
    : env_(from.env_)
{
}

AbstractStrictnessEnvironment* AbstractStrictnessEnvironment::clone() const override
{
    return new AbstractStrictnessEnvironment(*this);
}

bool AbstractStrictnessEnvironment::mergeWith(State const* other) override
{
    assert(dynamic_cast<AbstractStrictnessEnvironment const*>(other) != nullptr);
    return mergeWith(*dynamic_cast<AbstractStrictnessEnvironment const*>(other));
}

bool AbstractStrictnessEnvironment::mergeWith(AbstractStrictnessEnvironment const& other)
{
    bool result = false;

    for (auto i = other.env_.begin(), e = other.env_.end(); i != e; ++i) {
        auto own = env_.find(i->first);
        if (own == env_.end()) {
            // if there is a variable in other that does not exist here, we
            // merge it with the Absent value.
            AVALUE missing = i->second;
            missing.mergeWith(AVALUE::Absent());
            env_.emplace(i->first, missing);
            result = true;
        } else {
            // otherwise try merging it with our variable
            result = own->second.mergeWith(i->second) or result;
        }
    }
    for (auto i = env_.begin(), e = env_.end(); i != e; ++i) {
        auto them = other.env_.find(i->first);
        if (them == other.env_.end()) {
            // The other env has is missing this value, we must treat this
            // as absent
            result = i->second.mergeWith(AVALUE::Absent()) or result;
        }
    }
}

// TODO is this what we want wrt parents?
bool AbstractStrictnessEnvironment::empty() const
{
    return env_.empty();
}

bool AbstractStrictnessEnvironment::contains(KEY name) const
{
    return env_.find(name) != env_.end();
}

AVALUE const& AbstractStrictnessEnvironment::find(KEY name) const
{
    auto i = env_.find(name);
    if (i == env_.end())
        return AVALUE::top();
    else
        return i->second;
}

AVALUE const& AbstractStrictnessEnvironment::operator[](KEY name) const
{
    auto i = env_.find(name);
    if (i == env_.end())
        return AVALUE::top();
    else
        return i->second;
}

AVALUE& AbstractStrictnessEnvironment::operator[](KEY name)
{
    auto i = env_.find(name);
    if (i == env_.end()) {
        // so that we do not demand default constructor on values
        env_.insert(std::pair<KEY, AVALUE>(name, AVALUE::top()));
        i = env_.find(name);
        return i->second;
    } else {
        return i->second;
    }
}

string AbstractStrictnessEnvironment::to_string() const
{
    stringstream ss;
    for (auto i : env_) {
        ss << "[" << i.first.to_string() << "] => " << i.second.to_string() << endl;
    }
    return ss.str();
}

void AbstractStrictnessEnvironment::mergeAll(AVALUE v)
{
    for (auto& e : env_)
        e.second.mergeWith(v);
}
