/*#-------------------------------------------------
#
#           Number randomizer template
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.0 - 2021/04/19
#
#   - Header-only
#
#-------------------------------------------------*/

#ifndef RANDOMIZER_H
#define RANDOMIZER_H

#include <random>

// randomizer, call it with random<type>(min_val, max_val) - e.g. : random<double>(0.3, 1.75)
template<class T>
using uniform_distribution =
typename std::conditional<
    std::is_floating_point<T>::value,
    std::uniform_real_distribution<T>,
    typename std::conditional<
        std::is_integral<T>::value,
        std::uniform_int_distribution<T>,
        void
    >::type
>::type;

template <class T>
T Randomize(T lower, T upper)
{
    static thread_local std::mt19937_64 mt(std::random_device{}());
    uniform_distribution<T> dist(lower,upper);

    return dist(mt);
}

#endif // RANDOMIZER_H
