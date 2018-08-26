#pragma once

#include "analysis_framework/analysis.h"
#include "analysis_framework/dispatchers.h"

namespace rir {

class StrictnessState : public State {
  public:
    SEXP exportToR() const { return R_NilValue; }

    StrictnessState* clone() const override { return new StrictnessState(); }

    /** Merges the information from the other state, returns true if changed.
     */
    bool mergeWith(State const* other) override { return true; }
};

class StrictnessAnalyzer : public ForwardAnalysisFinal<StrictnessState>,
                           InstructionDispatcher::Receiver {
  public:
    StrictnessAnalyzer() : dispatcher_(InstructionDispatcher(*this)) {}
    Dispatcher& dispatcher() override { return dispatcher_; }

  private:
    InstructionDispatcher dispatcher_;
};
}
