#pragma once

#include "StrictnessLattice.h"
#include "code/analysis.h"
#include "interpreter/interp_context.h"
#include <algorithm>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

// TODO - last_forced_ should be provided as an option for all constructors
namespace rir {

class StrictnessState : public State {

    // http://www1.cuni.cz/~obo/r_surprises.html
    // http://stackoverflow.com/questions/27797040/error-value-of-set-string-elt-must-be-a-charsxp-not-a-builtin
    // https://stat.ethz.ch/R-manual/R-devel/library/base/html/data.matrix.html
    // https://cran.r-project.org/doc/manuals/R-exts.html#index-mkChar
    // https://cran.r-project.org/doc/manuals/R-exts.html#Handling-lists
    //
public:
    typedef const char* assumption_type;
    typedef std::list<assumption_type> assumptions_type;

    StrictnessState()
        : leaf_(true)
    {
    }

    StrictnessState(CodeEditor const& code)
        : leaf_(true)
        , arguments_(code.argumentTags())
        , size_(arguments_.size())
        , forced_(StrictnessLattice::make_vector(
              size_, rir::StrictnessLattice::Value::NEVER))
        , contains_(StrictnessLattice::make_vector(
              size_, rir::StrictnessLattice::Value::ALWAYS))
        , order_(StrictnessLattice::make_matrix(
              size_, rir::StrictnessLattice::Value::NEVER))
    {
    }

    StrictnessState(CodeEditor const& code,
        const StrictnessLattice::vector& forced,
        const StrictnessLattice::vector& contains,
        const StrictnessLattice::matrix& order)
        : leaf_(true)
        , arguments_(code.argumentTags())
        , size_(arguments_.size())
        , forced_(forced)
        , contains_(contains)
        , order_(order)
    {
    }

    StrictnessState(const std::vector<SEXP> arguments,
        const StrictnessLattice::vector forced,
        const StrictnessLattice::vector contains,
        const StrictnessLattice::matrix order)
        : leaf_(true)
        , arguments_(arguments)
        , size_(arguments.size())
        , forced_(forced)
        , contains_(contains)
        , order_(order)
    {
    }

    StrictnessState(StrictnessState const& state)
    {
        setArguments(state.getArguments());
        setForced(state.getForced());
        setContains(state.getContains());
        setOrder(state.getOrder());
        setLastForced(state.getLastForced());
        setLeaf(state.isLeaf());
        setCallees(state.getCallees());
        for (auto& callee : getCallees()) {

            setCalleeState(callee, state.getCalleeState(callee));
        }
    }

    bool isLeaf() const { return leaf_; }

    void setLeaf(bool leaf) { leaf_ = leaf; }

    const std::vector<SEXP>& getArguments() const { return arguments_; }

    void setArguments(std::vector<SEXP> arguments)
    {
        size_ = arguments.size();
        arguments_ = arguments;
    }

    const StrictnessLattice::vector& getForced() const { return forced_; }

    void setForced(const StrictnessLattice::vector forced) { forced_ = forced; }

    const StrictnessLattice::vector& getContains() const { return contains_; }

    void setContains(const StrictnessLattice::vector contains)
    {
        contains_ = contains;
    }

    const StrictnessLattice::matrix& getOrder() const { return order_; }

    void setOrder(const StrictnessLattice::matrix order) { order_ = order; }

    const std::unordered_set<std::string>& getCallees() const
    {
        return callees_;
    }

    void setCallees(const std::unordered_set<std::string> callees)
    {
        callees_ = callees;
    }

    const StrictnessState*
    getCalleeState(const std::string& functionName) const
    {
        const auto& iter = states_.find(functionName);
        assert(iter != states_.end());
        return iter->second;
    }

    void setCalleeState(const std::string callee,
        const StrictnessState* state)
    {
        states_[callee] = state->clone();
    }

    const std::vector<int>& getLastForced() const { return last_forced_; }

    void setLastForced(const std::vector<int>& last_forced)
    {
        last_forced_ = last_forced;
    }

    bool operator==(const StrictnessState& other) const
    {
        bool equal = (getOrder() == other.getOrder()) and (getContains() == other.getContains()) and (getForced() == other.getForced()) and (getCallees() == other.getCallees()) and (getLastForced() == other.getLastForced()) and (isLeaf() == other.isLeaf());

        if (not equal)
            return false;

        for (const auto& key_value : states_) {
            if (!(*key_value.second == *other.getCalleeState(key_value.first)))
                return false;
        }
        return true;
    }

    void call(const std::string callee)
    {
        StrictnessState* currentState = new StrictnessState(
            getArguments(), getForced(), getContains(), getOrder());
        auto iterator = states_.find(callee);
        if (iterator == states_.end()) {
            states_[callee] = currentState;
            callees_.insert(callee);
        } else {
            StrictnessState* previousState = iterator->second;
            previousState->mergeWith(currentState);
            delete currentState;
        }
    }

    void print() const
    {
        Rprintf("Leaf function:       ");
        if (isLeaf())
            Rprintf("yes\n");
        else
            Rprintf("no\n");
        Rprintf("Argument evaluation: \n");
        for (size_t i = 0; i < arguments_.size(); ++i) {
            Rprintf("    ");
            Rprintf(CHAR(PRINTNAME(arguments_[i])));
            // Rprintf(Lattice::to_string(forced_[i]));
        }
    }

    /** TODO
     *  \param name argument
     *  \return if the argument `name` has been evaluated.
     */
    StrictnessLattice::Value isArgumentEvaluated(SEXP name)
    {
        size_t index = getArgumentIndex(name);
        assert(index != size_ and "Not an argument");
        return forced_[index];
    }

    /**
     * Returns a list of <'names', 'forced', 'contains', 'directions'>
     */
    SEXP exportToR() const
    {

        SEXP arguments = PROTECT(allocVector(STRSXP, size_));
        for (size_t i = 0; i < size_; ++i) {

            SET_STRING_ELT(arguments, i, PRINTNAME(arguments_[i]));
        }

        SEXP forced = StrictnessLattice::to_r_vector(forced_);
        setAttrib(forced, R_NamesSymbol, arguments);

        SEXP contains = StrictnessLattice::to_r_vector(contains_);
        setAttrib(contains, R_NamesSymbol, arguments);

        SEXP order = StrictnessLattice::to_r_matrix(order_);
        SEXP dirdimnames = PROTECT(allocVector(VECSXP, 2));
        SET_VECTOR_ELT(dirdimnames, 0, arguments);
        SET_VECTOR_ELT(dirdimnames, 1, arguments);
        setAttrib(order, R_DimNamesSymbol, dirdimnames);

        SEXP assumptions = PROTECT(allocVector(VECSXP, assumptions_.size()));
        size_t index = 0;
        for (const auto& assumption : assumptions_) {
            SET_VECTOR_ELT(assumptions, index, mkString(assumption));
            index += 1;
        }

        SEXP callees = PROTECT(allocVector(VECSXP, callees_.size()));
        index = 0;
        for (auto callee : callees_) {

            SET_VECTOR_ELT(callees, index, mkString(callee.c_str()));
            index += 1;
        }

        SEXP leaf = PROTECT(allocVector(LGLSXP, 1));
        LOGICAL(leaf)
        [0] = leaf_;

        SEXP dimnames = PROTECT(allocVector(VECSXP, 7));
        SET_VECTOR_ELT(dimnames, 0, mkString("arguments"));
        SET_VECTOR_ELT(dimnames, 1, mkString("forced"));
        SET_VECTOR_ELT(dimnames, 2, mkString("contains"));
        SET_VECTOR_ELT(dimnames, 3, mkString("order"));
        SET_VECTOR_ELT(dimnames, 4, mkString("callees"));
        SET_VECTOR_ELT(dimnames, 5, mkString("assumptions"));
        SET_VECTOR_ELT(dimnames, 6, mkString("leaf"));

        SEXP result = PROTECT(allocVector(VECSXP, 7));
        SET_VECTOR_ELT(result, 0, arguments);
        SET_VECTOR_ELT(result, 1, forced);
        SET_VECTOR_ELT(result, 2, contains);
        SET_VECTOR_ELT(result, 3, order);
        SET_VECTOR_ELT(result, 4, callees);
        SET_VECTOR_ELT(result, 5, assumptions);
        SET_VECTOR_ELT(result, 6, leaf);
        setAttrib(result, R_NamesSymbol, dimnames);

        UNPROTECT(11);
        return result;
    }

    StrictnessState* clone() const override
    {
        return new StrictnessState(*this);
    }

    bool mergeOrder(const StrictnessState& other)
    {
        const StrictnessLattice::vector& otherForced = other.getForced();
        const StrictnessLattice::matrix& otherOrder = other.getOrder();
        StrictnessLattice::Value o;
        bool result = false;
        for (size_t row = 0; row < size_; ++row) {
            for (size_t col = 0; col < size_; ++col) {

                // We check before merging directions, if the source is forced
                // in both places or not.
                // if source `i` is not forced in the other state, then copy the
                // edge from current state.
                // This means that if we reach `i` then we definitely go to 'j'
                // if source `i` is not forced in this state, then copy edge
                // from other state
                // If source `i` is forced in both states, then merge the edges
                if (otherForced[row] == StrictnessLattice::Value::NEVER) {
                    o = order_[row][col];
                } else if (forced_[row] == StrictnessLattice::Value::NEVER) {
                    o = otherOrder[row][col];
                } else {
                    o = StrictnessLattice::merge(order_[row][col],
                        otherOrder[row][col]);
                }
                result = result or (o != order_[row][col]);
                order_[row][col] = o;
            }
        }
        return result;
    }

    bool mergeWith(State const* other) override
    {

        StrictnessState const& ss = *dynamic_cast<StrictnessState const*>(other);
        // Rprintf("\n\nmergeWith called\n\n");
        // Rf_PrintValue(exportToR());
        // Rf_PrintValue(ss.exportToR());
        bool changed = mergeOrder(ss);
        changed = StrictnessLattice::merge(contains_, ss.getContains()) or changed;
        changed = StrictnessLattice::merge(forced_, ss.getForced()) or changed;

        if (leaf_ and not ss.isLeaf()) {
            leaf_ = false;
            changed = true;
        }

        const auto& otherCallees = ss.getCallees();
        callees_.insert(otherCallees.begin(), otherCallees.end());
        if (callees_.size() != states_.size()) {
            changed = true;
        }

        for (const auto& otherCallee : otherCallees) {
            const StrictnessState* otherState = ss.getCalleeState(otherCallee);
            auto iter = states_.find(otherCallee);
            if (iter == states_.end()) {
                changed = true;
                states_[otherCallee] = otherState->clone();
            } else {
                StrictnessState* currentState = iter->second;
                changed = currentState->mergeWith(otherState) or changed;
            }
        }

        for (int const& index : ss.getLastForced()) {
            auto it = std::find(last_forced_.begin(), last_forced_.end(), index);
            if (it == last_forced_.end()) {
                changed = true;
                last_forced_.push_back(index);
            }
        }

        return changed;
    }

    ~StrictnessState()
    {
        for (auto const& key_value : states_) {
            delete key_value.second;
        }
    }

protected:
    friend class InterproceduralStrictnessAnalysis;
    friend class IntraproceduralStrictnessAnalysis;

    inline size_t getArgumentIndex(SEXP name)
    {
        return std::distance(
            arguments_.begin(),
            std::find(arguments_.begin(), arguments_.end(), name));
    }

    void setAsNotLeaf() { leaf_ = false; }

    void makeEdge(int next)
    {
        // if something has already been forced then draw an edge.
        if (!last_forced_.empty()) {
            for (int const& index : last_forced_) {
                order_[index][next] = StrictnessLattice::Value::ALWAYS;
            }
        }
        // update the pointer to last forced promise
        // and make it point to this promise
        last_forced_.clear();
        last_forced_.push_back(next);
    }

    void forceArgument(SEXP name)
    {

        size_t index = getArgumentIndex(name);
        if (index == size_) {
            return;
        }

        // parameter has been reassigned somewhere so it does
        // not point to the promise. This means this parameter
        // is no longer important and the promise associated with
        // it will not be forced.
        if (contains_[index] == StrictnessLattice::Value::NEVER) {
            return;
        }

        // If the promise has not been forced before, only then
        // draw an edge. If the promise was probably forced, even
        // then draw an edge as we know for certain that it is
        // getting forced now.
        if (forced_[index] != StrictnessLattice::Value::ALWAYS) {
            makeEdge(index);
        }

        // Force the promise
        forced_[index] = StrictnessLattice::Value::ALWAYS;
    }

    void storeArgument(SEXP name)
    {
        size_t index = getArgumentIndex(name);
        if (index == size_) {
            return;
        }

        contains_[index] = StrictnessLattice::Value::NEVER;
    }

    void addAssumption(SEXP name)
    {
        assumptions_.push_front(CHAR(PRINTNAME(name)));
    }

    // TODO: What is value here? Be more specific once you understand it better.
    void push(SEXP value)
    {
        ostack_push(stack, result);
    }

    void peek(uint_t position)
    {
        ostack_at(stack, position);
    }

    void pop(uint_t count = 1)
    {
        ostack_popn(context, count);
    }

private:
    bool leaf_;
    std::vector<int> last_forced_;
    std::vector<SEXP> arguments_;
    size_t size_;
    StrictnessLattice::vector forced_;
    StrictnessLattice::vector contains_;
    StrictnessLattice::matrix order_;
    std::unordered_map<std::string, StrictnessState*> states_;
    std::unordered_set<std::string> callees_;
    assumptions_type assumptions_;
};
}

// TODO
// merge callees list
// merge contains, forced and state
