
#ifndef interpreter_context_h
#define interpreter_context_h

#include "interp_data.h"

#include <stdio.h>

#include <stdint.h>
#include <assert.h>

/** Compiler API. Given a language object, compiles it and returns the INTSXP containing the Function and its Code objects.

  The idea is to call this if we want on demand compilation of closures.
 */
typedef SEXP (*CompilerCallback)(SEXP);


#ifdef __cplusplus
extern "C" {
#endif

#define POOL_CAPACITY 4096
#define STACK_CAPACITY 4096

/** Resizeable R list.

 Allocates large list and then tricks R into believing that the list is actually smaller.

 This works because R uses non-moving GC and is used for constant and source pools as well as the interpreter object stack.
 */
typedef struct {
    SEXP list;
    size_t capacity;
} ResizeableList;

/** Interpreter frame information.
 */
typedef struct {
    struct Code* code;
    SEXP env;
    OpcodeT* pc;
    size_t bp;
} Frame;

#define CONTEXT_INDEX_CP 0
#define CONTEXT_INDEX_SRC 1
#define CONTEXT_INDEX_OSTACK 2

/** Interpreter's context.

 Interpreter's context is a list (so that it will be marked by R's gc) that contains the SEXP pools and stack as well as other stacks that do not need to be gc'd.

 */

typedef struct {
    SEXP list;
    ResizeableList cp;
    ResizeableList src;
    ResizeableList ostack;
    CompilerCallback compiler;
} Context;

// Some symbols
extern SEXP R_Subset2Sym;
extern SEXP R_SubsetSym;
extern SEXP R_SubassignSym;
extern SEXP R_Subassign2Sym;
extern SEXP R_valueSym;
extern SEXP setterPlaceholderSym;
extern SEXP getterPlaceholderSym;
extern SEXP quoteSym;

// TODO we might actually need to do more for the lengths (i.e. true length vs length)

INLINE size_t ostack_length(Context* c);

INLINE size_t rl_length(ResizeableList * l) {
    return Rf_length(l->list);
}

INLINE void rl_setLength(ResizeableList * l, size_t length) {
    ((VECSEXP)l->list)->vecsxp.length = length;
    ((VECSEXP)l->list)->vecsxp.truelength = length;
}

INLINE void rl_grow(ResizeableList * l, SEXP parent, size_t index) {
    int oldsize = rl_length(l);
    SEXP n = Rf_allocVector(VECSXP, l->capacity * 2);
    memcpy(DATAPTR(n), DATAPTR(l->list), l->capacity * sizeof(SEXP));
    SET_VECTOR_ELT(parent, index, n);
    l->list = n;
    rl_setLength(l, oldsize);
    l->capacity *= 2;
}

INLINE void rl_append(ResizeableList * l, SEXP val, SEXP parent, size_t index) {
    size_t i = rl_length(l);
    if (i == l->capacity)
        rl_grow(l, parent, index);
    rl_setLength(l, i + 1);
    SET_VECTOR_ELT(l->list, i, val);
}

INLINE SEXP ostack_top(Context* c) {
    return VECTOR_ELT(c->ostack.list, rl_length(&c->ostack) - 1);
}

INLINE SEXP* ostack_at(Context* c, uint32_t i) {
    assert(i < rl_length(&c->ostack));
    return &VECTOR_ELT(c->ostack.list, rl_length(&c->ostack) - 1 - i);
}

INLINE bool ostack_empty(Context* c) {
    return rl_length(&c->ostack) == 0;
}

INLINE size_t ostack_length(Context * c) {
    return rl_length(&c->ostack);
}

INLINE void ostack_popn(Context* c, size_t p) {
    int i = rl_length(&c->ostack) - p;
    rl_setLength(&c->ostack, i);
}

INLINE SEXP ostack_pop(Context* c) {
    // VECTOR_ELT does not check bounds
    int i = rl_length(&c->ostack) - 1;
    rl_setLength(&c->ostack, i);
    return VECTOR_ELT(c->ostack.list, i);
}

INLINE void ostack_push(Context* c, SEXP val) {
    rl_append(&c->ostack, val, c->list, CONTEXT_INDEX_OSTACK);
}

INLINE void ostack_ensureSize(Context* c, unsigned minFree) {
    while (rl_length(&c->ostack) + minFree > c->ostack.capacity)
        rl_grow(& c->ostack, c->list, CONTEXT_INDEX_OSTACK);
}

Context* context_create(CompilerCallback);

INLINE size_t cp_pool_length(Context * c) {
    return rl_length(& c->cp);
}

INLINE size_t src_pool_length(Context * c) {
    return rl_length(& c->src);
}

INLINE size_t cp_pool_add(Context* c, SEXP v) {
    size_t result = rl_length(& c->cp);
    rl_append(& c->cp, v, c->list, CONTEXT_INDEX_CP);
    return result;
}


INLINE size_t src_pool_add(Context* c, SEXP v) {
    size_t result = rl_length( &c->src);
    rl_append(& c->src, v, c->list, CONTEXT_INDEX_SRC);
    return result;
}

INLINE SEXP cp_pool_at(Context* c, size_t index) {
    return VECTOR_ELT(c->cp.list, index);
}

INLINE SEXP src_pool_at(Context* c, size_t value) {
    return VECTOR_ELT(c->src.list, value);
}



#ifdef __cplusplus
}
#endif

#endif // interpreter_context_h
