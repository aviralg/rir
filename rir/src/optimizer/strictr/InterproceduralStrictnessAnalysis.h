#pragma once

#include "code/analysis.h"
#include "code/dataflow.h"

#include "IntraproceduralStrictnessAnalysis.h"
#include "StrictnessState.h"

#include <algorithm>
#include <list>
#include <unordered_map>

namespace rir {

class InterproceduralStrictnessAnalysis
    : public ForwardAnalysisFinal<StrictnessState>,
      InstructionDispatcher::Receiver {

  public:
    InterproceduralStrictnessAnalysis()
        : dispatcher_(InstructionDispatcher(*this)) {}

    // ~InterproceduralStrictnessAnalysis() {
    //     for (auto const& key_value : signatures_) {
    //         delete key_value.second;
    //     }
    // }
    static SEXP exportToR() {
        SEXP dimnames = PROTECT(allocVector(VECSXP, signatures_.size()));
        SEXP result = PROTECT(allocVector(VECSXP, signatures_.size()));
        size_t index = 0;
        for (const auto& key_value : signatures_) {
            SET_VECTOR_ELT(dimnames, index, mkString(key_value.first.c_str()));
            SET_VECTOR_ELT(result, index, key_value.second->exportToR());
            ++index;
        }
        setAttrib(result, R_NamesSymbol, dimnames);
        UNPROTECT(2);
        return result;
    }

    static const StrictnessState& preprocess(std::string callee, SEXP body) {
        auto analyzer = IntraproceduralStrictnessAnalysis();
        CodeEditor ce(body);
        analyzer.analyze(ce);
        StrictnessState* state = analyzer.finalState().clone();
        signatures_[callee] = state;
        binaries_[callee] = body;
        for (auto const& called : state->getCallees()) {
            updateCallGraph(callee, called);
        }
        return *state;
    }

    void print() override { finalState().print(); }

    static const StrictnessState* getSignature(const std::string& callee) {
        const auto& iter = signatures_.find(callee);
        assert(iter != signatures_.end());
        return iter->second;
    }

    static void computeFixedPoint() { worklistAlgorithm(); }

    static void worklistAlgorithm() {
        const auto& names = functionNames();
        std::list<std::string> worklist(names.begin(), names.end());
        for (auto iterator = worklist.begin(); iterator != worklist.end();) {
            const std::string callee = *iterator;

            iterator = worklist.erase(iterator);
            // true if analysis result has changed
            if (fixedPointStep(callee)) {

                // Rf_PrintValue(signatures_[functionName]->exportToR());
                auto callers = getCallers(callee);
                // all dependents for this function need to be reanalyzed;
                worklist.insert(worklist.end(), callers.begin(), callers.end());
            }
        }
    }

    static bool fixedPointStep(const std::string callee) {
        auto analyzer = InterproceduralStrictnessAnalysis();
        CodeEditor ce(binaries_[callee]);
        analyzer.setName(callee);
        analyzer.analyze(ce);
        if (*getSignature(callee) == analyzer.finalState()) {
            return false;
        }
        signatures_[callee] = analyzer.finalState().clone();
        return true;
    }

  protected:
    void ldfun_(CodeEditor::Iterator ins) override {
        BC bc = *ins;
        current().forceArgument(bc.immediateConst());
        setCallee(CHAR(PRINTNAME(bc.immediateConst())));
    }

    // TODO - copy constructor for StrictnessState
    // TODO - destructor for this class
    void call_(CodeEditor::Iterator ins) override {
        current().setAsNotLeaf();
        current().call(getCallee());
        const StrictnessState* state = getSignature(getCallee());
        current().setForced(state->getForced());
        current().setContains(state->getContains());
        current().setOrder(state->getOrder());
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
        const std::string& callee = getName();
        const auto& callers = getCallers(callee);
        if (callers.size() == 0)
            return new StrictnessState(*code_);
        auto iter = callers.begin();

        StrictnessState* state =
            getSignature(*iter)->getCalleeState(callee)->clone();

        ++iter;
        for (; iter != callers.end(); ++iter) {

            state->mergeWith(getSignature(*iter)->getCalleeState(callee));
        }
        return state;
    }

  public:
    static std::unordered_set<std::string> functionNames() {
        std::unordered_set<std::string> names;
        for (auto& key_value : binaries_) {
            names.insert(key_value.first);
        }
        return names;
    }

    static const std::unordered_set<std::string>&
    getCallees(const std::string& caller) {
        return callees_[caller];
    }

    static const std::unordered_set<std::string>&
    getCallers(const std::string& callee) {
        return callers_[callee];
    }

    static void addCallee(const std::string caller, const std::string callee) {
        callees_[caller].insert(callee);
    }

    static void addCaller(const std::string callee, const std::string caller) {
        callers_[callee].insert(caller);
    }

    static void updateCallGraph(const std::string caller,
                                const std::string callee) {
        addCallee(caller, callee);
        addCaller(callee, caller);
    }

    void setCallee(const std::string callee) { callee_ = callee; }
    // void setCallee(const char* callee) { callee_ = callee; }
    const std::string& getCallee() const { return callee_; };
    void setName(const std::string name) { name_ = name; }
    // void setName(const char* name) { name_ = name; }
    const std::string& getName() const { return name_; };

    InstructionDispatcher dispatcher_;

    static std::unordered_map<std::string, std::unordered_set<std::string>>
        callers_;
    static std::unordered_map<std::string, std::unordered_set<std::string>>
        callees_;
    static std::unordered_map<std::string, SEXP> binaries_;
    static std::unordered_map<std::string, StrictnessState*> signatures_;
    std::string callee_;
    std::string name_;
};

std::unordered_map<std::string, std::unordered_set<std::string>>
    InterproceduralStrictnessAnalysis::callees_;
std::unordered_map<std::string, std::unordered_set<std::string>>
    InterproceduralStrictnessAnalysis::callers_;
std::unordered_map<std::string, SEXP>
    InterproceduralStrictnessAnalysis::binaries_;
std::unordered_map<std::string, StrictnessState*>
    InterproceduralStrictnessAnalysis::signatures_;
}
