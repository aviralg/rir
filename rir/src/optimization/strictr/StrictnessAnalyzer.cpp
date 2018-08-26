#include "StrictnessState.h"
#include "code/analysis.h"
#include "interpreter/interp_context.h"

// TODO - What does R_Visible do in the interpreter code?

// IDEA:
/*
  The Analyzer is an abstract interpreter. It works on the abstract state, i.e. the
  StrictnessState object, which exposes the abstract stack and the abstract environment.
*/

namespace rir {

class StrictnessAnalyzer : public ForwardAnalysisFinal<StrictnessState>,
                           InstructionDispatcher::Receiver {
private:
    Context ctx;

public:
    StrictnessAnalyzer()
        : dispatcher_(InstructionDispatcher(*this))
    {
    }

protected:
    void push_(CodeEditor::Iterator instr) override
    {
        BC bc = *instr;
        SEXP res = bc.immediateConst();
        current().stack().push(res);
    }

    void close_(CodeEditor::Iterator instr) override
    {
        BC bc = *instr;
        SEXP res = current().stack().peek(0);
        SEXP body = current().stack().peek(1);
        SEXP formals = current().stack().peek(2);
        res = allocSexp(CLOSXP);

        assert(isValidFunctionSEXP(body));
        // Make sure to use the most optimized version of this function
        while (((Function*)INTEGER(body))->next)
            body = ((Function*)INTEGER(body))->next;
        assert(isValidFunctionSEXP(body));

        SET_FORMALS(res, formals);
        SET_BODY(res, body);
        SET_CLOENV(res, current().environment());
        Rf_setAttrib(res, Rf_install("srcref"), srcref);
        current().stack().pop(3);
        current().stack().push(res);
    }

    void stvar_(CodeEditor::Iterator instr) override
    {
        BC bc = *instr;
        SEXP id = bc.immediateConst();
        int wasChanged = FRAME_CHANGED(current().environment());
        SEXP val = current().stack().pop();
        current().environment().set(id, val);
    }

    void set_shared_(CodeEditor::Iterator instr) override
    {
        SEXP val = current().stack().top();
        if (NAMED(val) < 2) {
            SET_NAMED(val, 2);
        }
    }

    void ldfun_(CodeEditor::Iterator instr) override
    {
        BC bc = *instr;
        SEXP sym = bc.immediateConst();
        SEXP fun = current().environment().findFun(sym);

        if (fun == R_UnboundValue)
            assert(false && "Unbound var");
        else if (fun == R_MissingArg)
            assert(false && "Missing argument");

        switch (TYPEOF(fun)) {
            case CLOSXP:
                current.jit(fun);
                break;
            case SPECIALSXP:
            case BUILTINSXP:
                // special and builtin functions are ok
                break;
            default:
                error("attempt to apply non-function");
        }
        current().stack().push(fun);
    }

    void BC::printArgs(CallSite cs)
    {
        Rprintf("[");
        for (unsigned i = 0; i < cs.nargs(); ++i) {
            auto arg = cs.arg(i);
            if (arg == MISSING_ARG_IDX)
                Rprintf(" _");
            else if (arg == DOTS_ARG_IDX)
                Rprintf(" ...");
            else
                Rprintf(" %x", arg);
        }
        Rprintf("] ");
    }

    void call_(CodeEditor::Iterator instr) override
    {
        BC bc = *instr;
        CallSite cs = instr.callSite();
        res = ostack_at(ctx, 0);
        res = doCall(c, res, n, id, env, ctx);
        ostack_pop(ctx);
        ostack_push(ctx, res);
    }

    // Deal with the impact on the local environment when calling random code
    void doCall(CodeEditor::Iterator ins)
    {
        // If the call is proceeded by a guard, we can assume that:
        // 1. No binding gets deleted
        // 2. Arguments are not overwritten
        // 3. The environment is not leaked
        if ((ins + 1) != code_->end() && (*(ins + 1)).is(Opcode::guard_env_)) {
            for (auto& e : current().env())
                if (e.second.t != FValue::Type::Argument)
                    e.second.mergeWith(FValue::Value());
            return;
        }

        // Depending on the type of analysis, we apply different strategies.
        // Only Type::Conservative is sound!
        if (type == Type::NoDelete)
            current().mergeAllEnv(FValue::Any());
        else if (type == Type::NoDeleteNoPromiseStore)
            current().mergeAllEnv(FValue::Value());
        else if (type == Type::Conservative) {
            current().global().leaksEnvironment = true;
            current().mergeAllEnv(FValue::Maybe());
        } else if (type == Type::NoArgsOverride)
            current().mergeAllEnv(FValue::Argument());
    }
}
