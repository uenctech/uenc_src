


















#ifndef MetaFSM_h
#define MetaFSM_h

#include <iostream>
#include <tuple>
#include <type_traits>

#pragma warning(push)
#pragma warning(disable: 4127 4100)
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>

#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
#pragma warning(pop)

#include "Indexes.h"
#include "Unroller.h"



namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace boost::msm::front;

namespace andrivet { namespace ADVobfuscator {

    
    struct Void {};

    
    
    template<typename R, typename F, typename... Args>
    struct event
    {
        
        constexpr event(F f, Args&... args): f_{f}, data_{args...} {}

        
        R call() const
        {
            
            using I = typename Make_Indexes<sizeof...(Args)>::type;
            return call_(I{});
        }

    private:
        
        template<typename U = R, int... I>
        typename std::enable_if<!std::is_same<U, Void>::value, U>::type
        call_(Indexes<I...>) const { return f_.original()(std::get<I>(data_)...); }

        
        template<typename U = R, int... I>
        typename std::enable_if<std::is_same<U, Void>::value, Void>::type
        call_(Indexes<I...>) const { f_.original()(std::get<I>(data_)...); return Void{}; }

    private:
        F f_;
        std::tuple<Args&...> data_;
    };

    
    
    
    
    
    template<template<typename, typename> class FSM, typename R, typename F, typename... Args>
    inline R ObfuscatedCallRet(F f, Args&&... args)
    {
        using E = event<R, F, Args&...>;
        using M = msm::back::state_machine<FSM<E, R>>;
        using Run = typename FSM<E, R>::template Run<F, Args...>;

        M machine;
        Run::run(machine, f, std::forward<Args>(args)...);
        return machine.result_;
    }

    
    
    
    
    template<template<typename, typename = Void> class FSM, typename F, typename... Args>
    inline void ObfuscatedCall(F f, Args&&... args)
    {
        using E = event<Void, F, Args&...>;
        using M = msm::back::state_machine<FSM<E, Void>>;
        using Run = typename FSM<E, Void>::template Run<F, Args...>;

        M machine;
        Run::run(machine, f, std::forward<Args>(args)...);
    }

    

    
    
    
    
    
    
    template<template<typename, typename, typename> class FSM, typename R, typename P, typename F, typename... Args>
    inline R ObfuscatedCallRetP(F f, Args&&... args)
    {
        using E = event<R, F, Args&...>;
        using M = msm::back::state_machine<FSM<E, P, R>>;
        using Run = typename FSM<E, P, R>::template Run<F, Args...>;

        M machine;
        Run::run(machine, f, std::forward<Args>(args)...);
        return machine.result_;
    }

    
    
    
    
    
    template<template<typename, typename, typename = Void> class FSM, typename P, typename F, typename... Args>
    inline void ObfuscatedCallP(F f, Args&&... args)
    {
        using E = event<Void, F, Args&...>;
        using M = msm::back::state_machine<FSM<E, P, Void>>;
        using Run = typename FSM<E, P, Void>::template Run<F, Args...>;

        M machine;
        Run::run(machine, f, std::forward<Args>(args)...);
    }

    
    template<typename F>
    struct ObfuscatedAddress
    {
        
        using func_ptr_t = void(*)();
        
        using func_ptr_integral = std::conditional<sizeof(func_ptr_t) <= sizeof(long), long, long long>::type;

        func_ptr_integral f_;
        int offset_;

        constexpr ObfuscatedAddress(F f, int offset): f_{reinterpret_cast<func_ptr_integral>(f) + offset}, offset_{offset} {}
        constexpr F original() const { return reinterpret_cast<F>(f_ - offset_); }
    };

    
    template<typename F>
    constexpr ObfuscatedAddress<F> MakeObfuscatedAddress(F f, int offset) { return ObfuscatedAddress<F>(f, offset); }

}}

#endif
