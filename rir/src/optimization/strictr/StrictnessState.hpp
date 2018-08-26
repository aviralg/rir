#ifndef __STRICTNESS_STATE_HPP__
#define __STRICTNESS_STATE_HPP__

#include "EnvironmentMap.hpp"
#include "ForcedMap.hpp"
#include "OrderMap.hpp"
#include "StackMap.hpp"
#include "code/State.h"

class StrictnessState : public State {
private:
    AbstractStack<LabelSet> stack_;
    EnvironmentMap env_;
    OrderMap order_;
    ForcedMap forced_;

public:
    StrictnessState() = default;

    StrictnessState(StrictnessState const&) = default;

    StrictnessState* clone() const override;

    bool mergeWith(State const* other) override;

    stack_t const& stack() const;

    stack_t& stack();

    environment_t const& env() const;

    environment_t& env();

    order_t const& order() const;

    order_t& order();

    forced_t const& forced() const;

    forced_t& forced();

    bool mergeWith(AbstractState const& other);

    friend ostream& operator<<(ostream& os, const StrictnessState& ss);
};

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
#endif /* __STRICTNESS_STATE_HPP__ */
