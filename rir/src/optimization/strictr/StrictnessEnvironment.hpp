#ifndef __ABSTRACT_STRICTNESS_ENVIRONMENT_HPP__
#define __ABSTRACT_STRICTNESS_ENVIRONMENT_HPP__

/** An ugly SFINAE template that statically checks that values used in states
    have mergeWith method.

    This is not necessary but produces nice non-templatish messages if you forget
    to create the method.
*/
template <typename T>
class has_mergeWith {
    typedef char one;
    typedef long two;

    template <typename C>
    static one test(char (*)[sizeof(&C::mergeWith)]);
    template <typename C>
    static two test(...);

public:
    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};

template <typename KEY, typename AVALUE>
class AbstractStrictnessEnvironment : public State {

    static_assert(std::is_copy_constructible<AVALUE>::value,
        "Abstract values must be copy constructible");
    static_assert(has_mergeWith<AVALUE>::value,
        "Abstract values must have mergeWith method");

    // TODO change this to static methods
    /*static_assert(std::is_const<decltype(AVALUE::top)>::value,
                  "Must have const top");
    static_assert(std::is_const<decltype(AVALUE::top)>::value,
                  "Must have const bottom");
    */

public:
    typedef AVALUE Value;

    AbstractStrictnessEnvironment()
    {
    }

    AbstractStrictnessEnvironment(AbstractStrictnessEnvironment const& from)
        : env_(from.env_)
    {
    }

    AbstractStrictnessEnvironment* clone() const override;

    bool mergeWith(State const* other) override;
    bool mergeWith(AbstractStrictnessEnvironment const& other);
    bool empty() const;
    bool contains(KEY name) const;
    AVALUE const& find(KEY name) const;
    AVALUE const& operator[](KEY name) const;
    AVALUE& operator[](KEY name);
    void mergeAll(AVALUE v);
    typename std::map<KEY, AVALUE>::iterator begin() { return env_.begin(); }
    typename std::map<KEY, AVALUE>::iterator end() { return env_.end(); }

protected:
    std::map<KEY, AVALUE> env_;
};

void print() const;
#endif /* __ABSTRACT_STRICTNESS_ENVIRONMENT_HPP__ */
