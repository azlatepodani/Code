#include <type_traits>


namespace azp {


template <typename T>
using remove_cv_t = typename std::remove_cv<T>::type;

template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;

template <typename T>
struct remove_cvref {
    typedef remove_cv_t<remove_reference_t<T>> type;
};

template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <bool val, typename T>
using enable_if_t = typename std::enable_if<val, T>::type;

template <typename T>
using enable_if_int_t = enable_if_t<std::is_integral<remove_cvref_t<T>>::value, T>;


} // namespace azp

