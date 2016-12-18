/** Enables the use of R internals for us so that we can manipulate R structures
 * in low level.
 */

#include <cassert>

#include "api.h"

#include "ir/Compiler.h"
#include "interpreter/interp_context.h"
#include "interpreter/interp.h"
#include "ir/BC.h"

#include "utils/FunctionHandle.h"

#include "optimizer/Printer.h"
#include "code/analysis.h"
#include "optimizer/cp.h"
#include "optimizer/Signature.h"
#include "optimizer/liveness.h"

#include "ir/Optimizer.h"

using namespace rir;

extern "C" void resetCompileExpressionOverride();
extern "C" void resetCmpFunOverride();
extern "C" void setCompileExpressionOverride(int, SEXP (*fun)(SEXP, SEXP));
extern "C" void setCmpFunOverride(int, SEXP (*fun)(SEXP));
typedef SEXP (*callback_eval)(SEXP, SEXP);
extern "C" void setEvalHook(callback_eval);
extern "C" void resetEvalHook();
static int rirJitEnabled = 0;


/** Testing - returns the result of signature analysis on given code.
 */
REXPORT SEXP rir_analysis_signature(SEXP what) {
    CodeEditor ce(what);
    SignatureAnalysis sa;
    sa.analyze(ce);
    return sa.finalState().exportToR();
}


REXPORT SEXP rir_da(SEXP what) {
    CodeEditor ce(what);
    Printer p;
    p.run(ce);


/*    ConstantPropagation cp;
    cp.analyze(ce);
    cp.print();
    cp.finalState().print(); */

    SignatureAnalysis sa;
    sa.analyze(ce);
    sa.print();


    return R_NilValue;
}



// actual rir api --------------------------------------------------------------

/** Returns TRUE if given SEXP is a valid rir compiled function. FALSE otherwise.
 */
REXPORT SEXP rir_isValidFunction(SEXP what) {
    return isValidClosureSEXP(what) == nullptr ? R_FalseValue : R_TrueValue;
}

/** Prints the information in given Function SEXP
 */
REXPORT SEXP rir_disassemble(SEXP what) {

    Rprintf("%p\n", what);
    ::Function * f = TYPEOF(what) == CLOSXP ? isValidClosureSEXP(what) : isValidFunctionSEXP(what);

    if (f == nullptr)
        Rf_error("Not a rir compiled code");

    CodeEditor(what).print();
    return R_NilValue;
}

REXPORT SEXP rir_compile(SEXP what) {

    // TODO make this nicer
    if (TYPEOF(what) == CLOSXP) {
        SEXP body = BODY(what);
        if (TYPEOF(body) == BCODESXP) {
            body = VECTOR_ELT(CDR(body), 0);
        }

        if (TYPEOF(body) == INTSXP)
            Rf_error("closure already compiled");

        SEXP result = allocSExp(CLOSXP);
        PROTECT(result);
        auto res = Compiler::compileClosure(body, FORMALS(what));
        SET_FORMALS(result, res.formals);
        SET_CLOENV(result, CLOENV(what));
        SET_BODY(result, rir_createWrapperAst(res.bc));
        Rf_copyMostAttrib(what, result);
        UNPROTECT(1);
        return result;
    } else {
        if (TYPEOF(what) == BCODESXP) {
            what = VECTOR_ELT(CDR(what), 0);
        }
        auto res = Compiler::compileExpression(what);
        return rir_createWrapperAst(res.bc);
    }
}

REXPORT SEXP rir_markOptimize(SEXP what) {
    assert(TYPEOF(what) == CLOSXP);
    SEXP b = BODY(what);
    isValidFunctionSEXP(b);
    Function* fun = (Function*)INTEGER(b);
    fun->markOpt = true;
    return R_NilValue;
}

REXPORT SEXP rir_jitDisable(SEXP expression) {
    if (rirJitEnabled == 2)
        Rf_error("enabled sticky, cannot disable");

    rirJitEnabled = 0;
    resetCompileExpressionOverride();
    resetCmpFunOverride();
    resetEvalHook();
    return R_NilValue;
}

static SEXP compileClosure_(SEXP cls) {
    return rir_compile(cls);
}

static SEXP compileExpression_(SEXP exp, SEXP env) {
    return rir_compile(exp);
}

REXPORT SEXP rir_jitEnable(SEXP expression) {
    if (TYPEOF(expression) != STRSXP || Rf_length(expression) < 1)
        Rf_error("string expected");

    const char* type CHAR(STRING_ELT(expression, 0));

    rirJitEnabled = 1;

    if (strcmp(type, "sticky") == 0 || strcmp(type, "force") == 0)
        rirJitEnabled = 2;

    setCompileExpressionOverride(INTSXP, &compileExpression_);
    setCmpFunOverride(INTSXP, &compileClosure_);

    if (strcmp(type, "force") == 0)
        setEvalHook(&rirEval);

    return R_NilValue;
}


/** Evaluates the given expression.
 */
REXPORT SEXP rir_eval(SEXP what, SEXP env) {
    ::Function * f = isValidFunctionObject(what);
    if (f == nullptr)
        f = isValidClosureSEXP(what);
    if (f == nullptr)
        Rf_error("Not rir compiled code");
    return evalRirCode(functionCode(f), globalContext(), env, 0);
}

// debugging & internal purposes API only --------------------------------------

/** Returns the constant pool object for inspection from R.
 */
REXPORT SEXP rir_cp() {
    return globalContext()->cp.list;
}

/** Returns the ast (source) pool object for inspection from R.
 */
REXPORT SEXP rir_src() {
    return globalContext()->src.list;
}

REXPORT SEXP rir_body(SEXP cls) {
    ::Function * f = isValidClosureSEXP(cls);
    if (f == nullptr)
        Rf_error("Not a valid rir compiled function");
    return functionSEXP(f);
}

REXPORT SEXP rir_executeWrapper(SEXP bytecode, SEXP env) {
    ::Function * f = reinterpret_cast<::Function *>(INTEGER(bytecode));
    return evalRirCode(functionCode(f), globalContext(), env, 0);
}

REXPORT SEXP rir_executePromiseWrapper(SEXP function, SEXP offset, SEXP env) {
    assert(TYPEOF(function) == INTSXP && "Invalid rir function");
    assert(TYPEOF(offset) == INTSXP && Rf_length(offset) == 1 && "Invalid offset");
    unsigned ofs = (unsigned)INTEGER(offset)[0];
    ::Code * c = codeAt((Function*)INTEGER(function), ofs);
    return evalRirCode(c, globalContext(), env, 0);
}



REXPORT SEXP rir_analyze_liveness(SEXP what) {

    Rprintf("--- Liveness analysis of %p ---\n", what);

    ::Function * f = TYPEOF(what) == CLOSXP ? isValidClosureSEXP(what) : isValidFunctionSEXP(what);
    if (f == nullptr)
        Rf_error("Not a rir compiled code");

    CodeEditor ce(what);
//    ce.print(false);

    LivenessAnalysis la;
    la.analyze(ce);

    Rprintf("Live at the beginning: ");
    //la.finalState().stack().top().print();
    la.finalState().getState().print();
    Rprintf("\n");

    return R_NilValue;
}

// startup ---------------------------------------------------------------------

/** Initializes the rir contexts, registers the gc and so on...

  VECSXP length == # of pointers?
         tl = 0

 */

bool startup() {
    initializeRuntime(rir_compile, Optimizer::reoptimizeFunction);
    return true;
}

bool startup_ok = startup();
