#include "CodeEditor.h"

#include "BC.h"
#include "CodeStream.h"

#include <iomanip>
#include <iostream>

namespace rjit {
namespace rir {

CodeEditor::CodeEditor(CodeHandle& code) : original(code) {
    std::unordered_map<BC_t*, Label> bcLabels;

    {
        BC_t* pc = (BC_t*)code.data();
        BC_t* end = (BC_t*)((uintptr_t)pc + code.code->codeSize);
        while (pc != end) {
            BC bc = BC::advance(&pc);
            if (bc.isJmp()) {
                BC_t* target = (BC_t*)((uintptr_t)pc + bc.immediate.offset);
                if (!bcLabels.count(target))
                    bcLabels[target] = nextLabel++;
            }
        }
    }

    {
        BytecodeList* pos = &front;

        BC_t* pc = (BC_t*)code.data();
        BC_t* end = (BC_t*)((uintptr_t)pc + code.code->codeSize);

        while (pc != end) {
            pos->next = new BytecodeList();
            auto prev = pos;
            pos = pos->next;
            pos->prev = prev;

            if (bcLabels.count(pc)) {
                Label label = bcLabels[pc];
                pos->bc = BC::label(label);

                pos->next = new BytecodeList();
                auto prev = pos;
                pos = pos->next;
                pos->prev = prev;
            }

            // TODO: this needs to be redone
            // SEXP ast = code->getAst(pc);
            // if (ast)
            //     astMap[pos] = ast;

            BC bc = BC::advance(&pc);
            if (bc.isJmp()) {
                BC_t* target = (BC_t*)((uintptr_t)pc + bc.immediate.offset);
                Label label = bcLabels[target];
                bc.immediate.offset = label;
            }

            pos->bc = bc;
        }

        last.prev = pos;
        pos->next = &last;
    }
}

CodeEditor::~CodeEditor() {
    BytecodeList* pos = front.next;

    while (pos != &last) {
        BytecodeList* old = pos;
        pos = pos->next;
        delete old;
    }
}

void CodeEditor::print() {
    std::cout << "-----------------------------\n";
    for (Cursor cur = getCursor(); !cur.atEnd(); ++cur) {
        (*cur).print();
    }
}

void CodeEditor::normalizeReturn() {
    if (empty())
        return;

    Cursor lastI(this, last.prev);
    if ((*lastI).bc == BC_t::ret_)
        lastI.remove();

    for (Cursor pos = getCursor(); !pos.atEnd(); ++pos) {
        // TODO replace ret with jumps to the end
        assert((*pos).bc != BC_t::ret_);
    }
}
}
}
