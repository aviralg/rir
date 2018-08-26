#pragma once

#include "StrictnessState.h"
#include "code/analysis.h"
#include <algorithm>

namespace rir {

class IntraproceduralStrictnessAnalysis
    : public ForwardAnalysisFinal<StrictnessState>,
      InstructionDispatcher::Receiver {
  public:
    void print() override { finalState().print(); }

    IntraproceduralStrictnessAnalysis()
        : dispatcher_(InstructionDispatcher(*this)) {}

  protected:
    void ldfun_(CodeEditor::Iterator ins) override {
        BC bc = *ins;
        current().forceArgument(bc.immediateConst());
        setCallee(CHAR(PRINTNAME(bc.immediateConst())));
    }

    void call_(CodeEditor::Iterator ins) override {
        current().setAsNotLeaf();
        current().call(getCallee());
    }
    void ldddvar_(CodeEditor::Iterator ins) override {}

    void ldarg_(CodeEditor::Iterator ins) override {
        BC bc = *ins;
        current().forceArgument(bc.immediateConst());
    }

    void ldvar_(CodeEditor::Iterator ins) override {
        BC bc = *ins;
        current().forceArgument(bc.immediateConst());
    }

    void stvar_(CodeEditor::Iterator ins) override {
        BC bc = *ins;
        current().storeArgument(bc.immediateConst());
    }

    Dispatcher& dispatcher() override { return dispatcher_; }

    StrictnessState* initialState() override {
        return new StrictnessState(*code_);
    }

  private:
    std::string callee_;
    const std::string& getCallee() { return callee_; }
    void setCallee(const char* callee) { callee_ = callee; }
    void setCallee(const std::string& callee) { callee_ = callee; }

    InstructionDispatcher dispatcher_;
};
}
