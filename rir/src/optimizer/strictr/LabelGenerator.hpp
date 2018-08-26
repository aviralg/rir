#ifndef __LABEL_GENERATOR_HPP__
#define __LABEL_GENERATOR_HPP__

#include <string>

using namespace std;

typedef std::string label_t;

enum LabelType {
    FunctionLabel,
    PromiseLabel,
    LibraryLabel,
    UntrackedLabel
};

class LabelGenerator {

private:
    unsigned function_count_;
    unsigned promise_count_;
    unsigned library_count_;
    unsigned untracked_count_;

    label_t create_label_(const char* prefix, unsigned count);

public:
    LabelGenerator();

    label_t generate(LabelType label_type);
};

#endif /* __LABEL_GENERATOR_HPP__ */
