//=======================================================================
// Copyright (c) 2013-2014 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef CPP_UTILS_TMP_HPP
#define CPP_UTILS_TMP_HPP

#include <tuple>
#include <utility>

#define HAS_MEM_FUNC(func, name)                                              \
template<typename T, typename Sign>                                           \
struct name {                                                                 \
    typedef char yes[1];                                                      \
    typedef char no [2];                                                      \
    template <typename U, U> struct type_check;                               \
    template <typename _1> static yes &chk(type_check<Sign, &_1::func> *);    \
    template <typename   > static no  &chk(...);                              \
    static bool constexpr const value = sizeof(chk<T>(0)) == sizeof(yes);     \
}

#define HAS_STATIC_FIELD(field, name)                                                       \
template <class T>                                                                          \
class name {                                                                                \
    template<typename U, typename =                                                         \
        typename std::enable_if<!std::is_member_pointer<decltype(&U::field)>::value>::type> \
    static std::true_type check(int);                                                       \
    template <typename>                                                                     \
    static std::false_type check(...);                                                      \
public:                                                                                     \
    static constexpr const bool value = decltype(check<T>(0))::value;                       \
};

namespace cpp {

template<std::size_t I, typename Tuple, typename Functor>
struct for_each_tuple_t_impl {
    static void for_each(Functor&& func){
        std::forward<Functor>(func).template operator()<typename std::tuple_element<I, Tuple>::type>();
        for_each_tuple_t_impl<I - 1, Tuple, Functor>::for_each(std::forward<Functor>(func));
    }
};

template<typename Tuple, typename Functor>
struct for_each_tuple_t_impl<0, Tuple, Functor> {
    static void for_each(Functor&& func){
        std::forward<Functor>(func).template operator()<typename std::tuple_element<0, Tuple>::type>();
    }
};

template<typename Tuple, typename Functor>
void for_each_tuple_t(Functor&& func){
    for_each_tuple_t_impl<std::tuple_size<Tuple>::value - 1, Tuple, Functor>::for_each(std::forward<Functor>(func));
}

template<bool b1>
struct not_u : std::true_type {};

template<>
struct not_u<true> : std::false_type {};

template<bool b1, bool b2, bool b3 = true, bool b4 = true, bool b5 = true, bool b6 = true>
struct and_u : std::false_type {};

template<>
struct and_u<true, true, true, true, true, true> : std::true_type {};

template<bool b1, bool b2, bool b3 = false, bool b4 = false, bool b5 = false, bool b6 = false, bool b7 = false, bool b8 = false, bool b9 = false>
struct or_u : std::true_type {};

template<>
struct or_u<false, false, false, false, false, false, false, false, false> : std::false_type {};

//enable_if utilities

template<bool B, class T = void>
using disable_if_t = typename std::enable_if<!B, T>::type;

namespace detail {

//Note: Unfortunately, CLang is bugged (Bug 11723), therefore, it is not
//possible to use universal enable_if/disable_if directly, it is necessary to
//use the dummy :( FU Clang!

enum class enabler_t { DUMMY };
constexpr const enabler_t dummy = enabler_t::DUMMY;

} //end of detail

template<bool B>
using enable_if_u = typename std::enable_if<B, detail::enabler_t>::type;

template<bool B>
using disable_if_u = typename std::enable_if<not_u<B>::value, detail::enabler_t>::type;

template<bool... B>
using enable_if_all_u = typename std::enable_if<and_u<B...>::value, detail::enabler_t>::type;

template<bool... B>
using disable_if_all_u = typename std::enable_if<not_u<and_u<B...>::value>::value, detail::enabler_t>::type;

template<bool... B>
using enable_if_one_u = typename std::enable_if<or_u<B...>::value, detail::enabler_t>::type;

template<bool... B>
using disable_if_one_u = typename std::enable_if<not_u<or_u<B...>::value>::value, detail::enabler_t>::type;

//Other TMP Utilities

template<template<typename...> class TT, typename T>
struct is_specialization_of : std::false_type {};

template<template<typename...> class TT, typename... Args>
struct is_specialization_of<TT, TT<Args...>> : std::true_type {};

//Variadic manipulations utilities

template<std::size_t N, typename... T>
struct nth_type {
    using type = typename std::tuple_element<N, std::tuple<T...>>::type;
};

template<typename... T>
struct first_type {
    using type = typename nth_type<0, T...>::type;
};

template<typename... T>
struct last_type {
    using type = typename nth_type<sizeof...(T)-1, T...>::type;
};

template<int I, typename T1, typename... T, enable_if_u<(I == 0)> = detail::dummy>
auto nth_value(T1&& t, T&&... /*args*/) -> decltype(std::forward<T1>(t)) {
    return std::forward<T1>(t);
}

template<int I, typename T1, typename... T, enable_if_u<(I > 0)> = detail::dummy>
auto nth_value(T1&& /*t*/, T&&... args)
        -> decltype(std::forward<typename nth_type<I, T1, T...>::type>(std::declval<typename nth_type<I, T1, T...>::type>())){
    return std::forward<typename nth_type<I, T1, T...>::type>(nth_value<I - 1>((std::forward<T>(args))...));
}

template<typename... T>
auto last_value(T&&... args){
    return std::forward<typename last_type<T...>::type>(nth_value<sizeof...(T) - 1>(args...));
}

template<typename... T>
auto first_value(T&&... args){
    return std::forward<typename first_type<T...>::type>(nth_value<0>(args...));
}

template<typename V, typename F, typename... S>
struct all_convertible_to  : std::integral_constant<bool, and_u<all_convertible_to<V, F>::value, all_convertible_to<V, S...>::value>::value> {};

template<typename V, typename F>
struct all_convertible_to<V, F> : std::integral_constant<bool, std::is_convertible<F, V>::value> {};

template<std::size_t I, std::size_t S, typename F, typename... T>
struct is_homogeneous_helper {
    template<std::size_t I1, std::size_t S1, typename Enable = void>
    struct helper_int : std::integral_constant<bool, and_u<std::is_same<F, typename nth_type<I1, T...>::type>::value, is_homogeneous_helper<I1+1, S1, F, T...>::value>::value> {};

    template<std::size_t I1, std::size_t S1>
    struct helper_int<I1, S1, std::enable_if_t<I1 == S1>> : std::integral_constant<bool, std::is_same<F, typename nth_type<I1, T...>::type>::value> {};

    static constexpr const auto value = helper_int<I, S>::value;
};

template<typename F, typename... T>
struct is_homogeneous : std::integral_constant<bool, is_homogeneous_helper<0, sizeof...(T)-1, F, T...>::value> {};

template<typename... T>
struct is_sub_homogeneous;

template<>
struct is_sub_homogeneous<> : std::integral_constant<bool, false> {};

template<typename T>
struct is_sub_homogeneous<T> : std::integral_constant<bool, false> {};

template<typename T1, typename T2>
struct is_sub_homogeneous<T1, T2> : std::integral_constant<bool, not_u<std::is_same<T1, T2>::value>::value> {};

template<typename T1, typename T2, typename T3, typename... T>
struct is_sub_homogeneous<T1, T2, T3, T...> : std::integral_constant<bool,
    and_u<
        std::is_same<T1, T2>::value,
        is_sub_homogeneous<T2, T3, T...>::value
    >::value> {};

//TODO Try to pass the index_sequence as a type, not a parameter

template<typename F, std::size_t I1, std::size_t... I, typename... T, enable_if_u<(sizeof...(I) == 0)> = detail::dummy>
void for_each_in_subset(F&& f, const std::index_sequence<I1, I...>& /*i*/, T&&... args){
    f(std::forward<typename nth_type<I1, T...>::type>(nth_value<I1>(args...)));
}

template<typename F, std::size_t I1, std::size_t... I, typename... T, enable_if_u<(sizeof...(I) > 0)> = detail::dummy>
void for_each_in_subset(F&& f, const std::index_sequence<I1, I...>& /*i*/, T&&... args){
    f(std::forward<typename nth_type<I1, T...>::type>(nth_value<I1>(args...)));
    for_each_in_subset(f, std::index_sequence<I...>(), std::forward<T>(args)...);
}

template<typename F, typename... T>
void for_each_in(F&& f, T&&... args){
    for_each_in_subset(f, std::make_index_sequence<sizeof...(T)>(), std::forward<T>(args)...);
}

} //end of namespace cpp

#endif //CPP_UTILS_TMP_HPP