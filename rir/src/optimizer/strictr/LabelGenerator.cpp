#include "LabelGenerator.hpp"

LabelGenerator::LabelGenerator()
{
    function_count_ = 0;
    promise_count_ = 0;
    library_count_ = 0;
    untracked_count_ = 0;
}

label_t
LabelGenerator::generate(LabelType label_type)
{
    switch (label_type) {
        case FunctionLabel:
            return create_label_("F#", ++function_count_);
        case PromiseLabel:
            return create_label_("P#", ++promise_count_);
        case LibraryLabel:
            return create_label_("L#", ++library_count_);
        case UntrackedLabel:
            return create_label_("U#", ++untracked_count_);
    }
}

label_t
LabelGenerator::create_label_(const char* prefix, unsigned count)
{
    return string(prefix) + to_string(count);
}
