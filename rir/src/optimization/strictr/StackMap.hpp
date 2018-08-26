#ifndef __STACK_MAP_HPP__
#define __STACK_MAP_HPP__

#include "LabelGenerator.hpp"
#include "code/State.h"

typedef AbstractStack<LabelSet> stack_t;
typedef AbstractEnvironment<label_t, stack_t>;

#endif /* __STACK_MAP_HPP__ */
