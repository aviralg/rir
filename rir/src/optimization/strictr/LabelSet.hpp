#ifndef __LABEL_SET_HPP__
#define __LABEL_SET_HPP__

#include "LabelGenerator.hpp"
#include <vector>

using namespace std;

class LabelSet {
private:
    typedef vector<label_t> container_t;
    typedef container_t::iterator iterator_t;
    container_t container_;
    iterator_t position_(label_t label);

public:
    void insert(label_t label);
    void remove(label_t label);
    bool contains(label_t label);
    size_t size();
    void clear();
    bool mergeWith(LabelSet const* other);
    bool mergeWith(LabelSet const& other);
};

#endif /* __LABEL_SET_HPP__ */
