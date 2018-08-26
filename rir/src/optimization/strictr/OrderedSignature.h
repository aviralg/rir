#pragma once

#include "code/analysis.h"
#include "Signature.h"
#include <algorithm>

namespace rir {

class OrderedSignature : public State {

    // http://www1.cuni.cz/~obo/r_surprises.html
    // http://stackoverflow.com/questions/27797040/error-value-of-set-string-elt-must-be-a-charsxp-not-a-builtin
    // https://stat.ethz.ch/R-manual/R-devel/library/base/html/data.matrix.html
    // https://cran.r-project.org/doc/manuals/R-exts.html#index-mkChar
    // https://cran.r-project.org/doc/manuals/R-exts.html#Handling-lists
    //
  public:
    OrderedSignature() : leaf_(true), last_forced_(std::vector<int>()) {}

    OrderedSignature(CodeEditor const& code)
        : leaf_(true), last_forced_(std::vector<int>()),
          arguments_(code.arguments()),
          forced_(std::vector<Bool3>(arguments_.size(), Bool3::no)),
          contains_(std::vector<Bool3>(arguments_.size(), Bool3::yes)),
          begin_(std::vector<Bool3>(arguments_.size(), Bool3::no)),
          end_(std::vector<Bool3>(arguments_.size(), Bool3::no)),
          direction_(std::vector<std::vector<Bool3>>(arguments_.size(),
                                                     std::vector<Bool3>(0))) {
        size_t size = arguments_.size();
        for (size_t i = 0; i < size; ++i) {
            direction_[i] = std::vector<Bool3>(size, Bool3::no);
        }
    }

    OrderedSignature(OrderedSignature const&) = default;

    void print() const {
        Rprintf("Leaf function:       ");
        if (isLeaf())
            Rprintf("yes\n");
        else
            Rprintf("no\n");
        Rprintf("Argument evaluation: \n");
        for (size_t i = 0; i < arguments_.size(); ++i) {
            Rprintf("    ");
            Rprintf(CHAR(PRINTNAME(arguments_[i])));
            switch (forced_[i]) {
                case Bool3::no:
                    Rprintf(" no\n");
                    break;
                case Bool3::maybe:
                    Rprintf(" maybe\n");
                    break;
                case Bool3::yes:
                    Rprintf(" yes\n");
                    break;
            }
        }
    }

    bool isLeaf() const { return leaf_; }

    /** TODO
     *  \param name argument
     *  \return if the argument `name` has been evaluated.
     */
    Bool3 isArgumentEvaluated(SEXP name) {
        std::vector<SEXP>::iterator i =
            std::find(arguments_.begin(), arguments_.end(), name);
        assert(i != arguments_.end() and "Not an argument");
        return forced_[i - arguments_.begin()];
    }

    SEXP bool3ToCHARSXP(Bool3 bool3) const {
        if (bool3 == Bool3::no) {
            return mkChar("no");
        } else if (bool3 == Bool3::yes) {
            return mkChar("yes");
        }
        return mkChar("maybe");
    }

    /**
     * Returns a list of <'names', 'forced', 'contains', 'directions'>
     */
    SEXP exportToR() const {

        size_t size = arguments_.size();

        SEXP arguments = PROTECT(allocVector(STRSXP, size));

        SEXP forced = PROTECT(allocVector(STRSXP, size));
        setAttrib(forced, R_NamesSymbol, arguments);

        SEXP contains = PROTECT(allocVector(STRSXP, size));
        setAttrib(contains, R_NamesSymbol, arguments);

        SEXP begin = PROTECT(allocVector(STRSXP, size));
        setAttrib(begin, R_NamesSymbol, arguments);

        SEXP end = PROTECT(allocVector(STRSXP, size));
        setAttrib(end, R_NamesSymbol, arguments);

        SEXP directions = PROTECT(allocVector(STRSXP, size * size));

        SEXP leaf = PROTECT(allocVector(LGLSXP, 1));
        LOGICAL(leaf)[0] = leaf_;

        for (size_t i = 0; i < size; ++i) {
            SET_STRING_ELT(arguments, i, PRINTNAME(arguments_[i]));
            SET_STRING_ELT(forced, i, bool3ToCHARSXP(forced_[i]));
            SET_STRING_ELT(contains, i, bool3ToCHARSXP(contains_[i]));
            SET_STRING_ELT(begin, i, bool3ToCHARSXP(begin_[i]));
            SET_STRING_ELT(end, i, bool3ToCHARSXP(end_[i]));
            for (size_t j = 0; j < size; ++j) {
                SET_STRING_ELT(directions, i + j * size,
                               bool3ToCHARSXP(direction_[i][j]));
            }
        }

        SEXP dim = PROTECT(allocVector(INTSXP, 2));
        INTEGER(dim)[0] = size;
        INTEGER(dim)[1] = size;
        setAttrib(directions, R_DimSymbol, dim);

        SEXP dirdimnames = PROTECT(allocVector(VECSXP, 2));
        SET_VECTOR_ELT(dirdimnames, 0, arguments);
        SET_VECTOR_ELT(dirdimnames, 1, arguments);
        setAttrib(directions, R_DimNamesSymbol, dirdimnames);

        SEXP dimnames = PROTECT(allocVector(VECSXP, 7));
        SET_VECTOR_ELT(dimnames, 0, mkString("arguments"));
        SET_VECTOR_ELT(dimnames, 1, mkString("forced"));
        SET_VECTOR_ELT(dimnames, 2, mkString("contains"));
        SET_VECTOR_ELT(dimnames, 3, mkString("begin"));
        SET_VECTOR_ELT(dimnames, 4, mkString("end"));
        SET_VECTOR_ELT(dimnames, 5, mkString("directions"));
        SET_VECTOR_ELT(dimnames, 6, mkString("leaf"));

        SEXP result = PROTECT(allocVector(VECSXP, 7));
        SET_VECTOR_ELT(result, 0, arguments);
        SET_VECTOR_ELT(result, 1, forced);
        SET_VECTOR_ELT(result, 2, contains);
        SET_VECTOR_ELT(result, 3, begin);
        SET_VECTOR_ELT(result, 4, end);
        SET_VECTOR_ELT(result, 5, directions);
        SET_VECTOR_ELT(result, 6, leaf);
        setAttrib(result, R_NamesSymbol, dimnames);

        UNPROTECT(11);
        return result;
    }

    OrderedSignature* clone() const override {
        return new OrderedSignature(*this);
    }

    /**
     * +-------------+-------+-------+-------+
     * | BOOL3 MERGE | yes   | no    | maybe |
     * +-------------+-------+-------+-------+
     * | yes         | yes   | maybe | maybe |
     * | no          | maybe | no    | maybe |
     * | maybe       | maybe | maybe | maybe |
     * +-------------+-------+-------+-------+
     *
     */

    Bool3 mergeBool3(const Bool3 a, const Bool3 b) {
        return a == b ? a : Bool3::maybe;
    }

    bool mergeWith(State const* other) override {

        OrderedSignature const& pes =
            *dynamic_cast<OrderedSignature const*>(other);

        bool result = false;
        size_t size = arguments_.size();
        Bool3 f, c, d, b, e;

        if (leaf_ and not pes.leaf_) {
            leaf_ = false;
            result = true;
        }

        for (size_t i = 0; i < size; ++i) {
            f = mergeBool3(forced_[i], pes.forced_[i]);
            c = mergeBool3(contains_[i], pes.contains_[i]);
            b = mergeBool3(begin_[i], pes.begin_[i]);
            e = mergeBool3(end_[i], pes.end_[i]);
            result = result or (f != forced_[i]) or (c != contains_[i]);
            for (size_t j = 0; j < size; ++j) {
                // We check before merging directions, if the source is forced
                // in both places or not.
                // if source `i` is not forced in the other state, then copy the
                // edge from current state.
                // This means that if we reach `i` then we definitely go to 'j'
                // if source `i` is not forced in this state, then copy edge
                // from other state
                // If source `i` is forced in both states, then merge the edges
                if (pes.forced_[i] == Bool3::no) {
                    d = direction_[i][j];
                } else if (forced_[i] == Bool3::no) {
                    d = pes.direction_[i][j];
                } else {
                    d = mergeBool3(direction_[i][j], pes.direction_[i][j]);
                }
                result = result or (d != direction_[i][j]);
                direction_[i][j] = d;
            }
            forced_[i] = f;
            contains_[i] = c;
            begin_[i] = b;
            end_[i] = e;
        }

        for (int const& index : pes.last_forced_) {
            auto it =
                std::find(last_forced_.begin(), last_forced_.end(), index);
            if (it == last_forced_.end()) {
                result = true;
                last_forced_.push_back(index);
            }
        }
        return result;
    }

  protected:
    friend class OrderedSignatureAnalysis;

    void setAsNotLeaf() { leaf_ = false; }

    void make_edge(int next) {
        // if something has already been forced then draw an edge.
        if (last_forced_.empty()) {
            begin_[next] = Bool3::yes;
        } else {
            for (int const& index : last_forced_) {
                end_[index] = Bool3::no;
                direction_[index][next] = Bool3::yes;
            }
        }

        // update the pointer to last forced promise
        // and make it point to this promise
        last_forced_.clear();
        last_forced_.push_back(next);
        end_[next] = Bool3::yes;
    }

    void forceArgument(SEXP name) {

        auto iterator = std::find(arguments_.begin(), arguments_.end(), name);

        if (iterator == arguments_.end()) {
            return;
        }

        size_t index = iterator - arguments_.begin();
        // parameter has been reassigned somewhere so it does
        // not point to the promise. This means this parameter
        // is no longer important and the promise associated with
        // it will not be forced.
        if (contains_[index] == Bool3::no) {
            return;
        }

        // If the promise has not been forced before, only then
        // draw an edge. If the promise was probably forced, even
        // then draw an edge as we know for certain that it is
        // getting forced now.
        if (forced_[index] != Bool3::yes) {
            make_edge(index);
        }

        // Force the promise
        forced_[index] = Bool3::yes;
    }

    void storeArgument(SEXP name) {

        auto iterator = std::find(arguments_.begin(), arguments_.end(), name);

        if (iterator == arguments_.end()) {
            return;
        }

        contains_[iterator - arguments_.begin()] = Bool3::no;
    }

  private:
    bool leaf_;
    std::vector<int> last_forced_;
    std::vector<SEXP> arguments_;
    std::vector<Bool3> forced_;
    std::vector<Bool3> contains_;
    std::vector<Bool3> begin_;
    std::vector<Bool3> end_;
    std::vector<std::vector<Bool3>> direction_;
};

class OrderedSignatureAnalysis : public ForwardAnalysisFinal<OrderedSignature>,
                                 InstructionDispatcher::Receiver {
  public:
    void print() override { finalState().print(); }

    OrderedSignatureAnalysis() : dispatcher_(InstructionDispatcher(*this)) {}

  protected:
    void ldfun_(CodeEditor::Iterator ins) override {
        BC bc = *ins;
        current().forceArgument(bc.immediateConst());
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

    void call_(CodeEditor::Iterator ins) override { current().setAsNotLeaf(); }

    void dispatch_(CodeEditor::Iterator ins) override {}

    void dispatch_stack_(CodeEditor::Iterator ins) override {}

    void call_stack_(CodeEditor::Iterator ins) override {}

    void stvar_(CodeEditor::Iterator ins) override {
        BC bc = *ins;
        current().storeArgument(bc.immediateConst());
    }

    Dispatcher& dispatcher() override { return dispatcher_; }

  protected:
    OrderedSignature* initialState() override {
        return new OrderedSignature(*code_);
    }

  private:
    InstructionDispatcher dispatcher_;
};
}
