#include "StrictnessState.hpp"

StrictnessState::StrictnessState* clone() const override
{
    return new StrictnessState(*this);
}

bool StrictnessState::mergeWith(State const* other) override
{
    assert(dynamic_cast<StrictnessState const*>(other) != nullptr);
    return mergeWith(*dynamic_cast<StrictnessState const*>(other));
}

stack_t const& StrictnessState::stack() const { return stack_; }

stack_t& StrictnessState::stack() { return stack_; }

environment_t const& StrictnessState::env() const { return env_; }

environment_t& StrictnessState::env() { return env_; }

order_t const& StrictnessState::order() const { return order_; }

order_t& StrictnessState::order() { return order_; }

forced_t const& StrictnessState::forced() const { return forced_; }

forced_t& StrictnessState::forced() { return forced_; }

bool StrictnessState::mergeWith(AbstractState const& other)
{
    bool result = false;
    result = stack_.mergeWith(other.stack_) or result;
    result = env_.mergeWith(other.env_) or result;
    result = order_.mergeWith(other.order_) or result;
    result = forced_.mergeWith(other.forced_) or result;
    return result;
}

ostream& operator<<(ostream& os, const StrictnessState& ss)
{
    return os << "Stack Map"
              << "\n"
              << ss.stack_ << "\n"
              << "Environment Map"
              << "\n"
              << ss.env_ << "\n"
              << "Order Map"
              << "\n"
              << ss.order_ << "\n"
              << "Forced Map"
              << "\n"
              << ss.forced_ << "\n";
}

// AVALUE pop()
// {
//     return stack_.pop();
// }

// AVALUE& top() { return stack_.top(); }

// AVALUE const& top() const { return stack_.top(); }

// void pop(size_t num)
// {
//     stack_.pop(num);
// }

// void push(AVALUE value)
// {
//     stack_.push(value);
// }

// AVALUE const& operator[](size_t index) const
// {
//     return stack_[index];
// }

// AVALUE& operator[](size_t index)
// {
//     return stack_[index];
// }

// AVALUE const& operator[](SEXP name) const
// {
//     return env_[name];
// }

// AVALUE& operator[](SEXP name)
// {
//     return env_[name];
// }
