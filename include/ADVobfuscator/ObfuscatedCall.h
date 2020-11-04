


















#ifndef ObfuscatedCall_h
#define ObfuscatedCall_h

#include "MetaFSM.h"
#include "MetaRandom.h"






namespace andrivet { namespace ADVobfuscator { namespace Machine1 {

    
    
    
    template<typename E, typename R = Void>
    struct Machine : public msm::front::state_machine_def<Machine<E, R>>
    {
        
        struct event1 {};
        struct event2 {};
        struct event3 {};
        struct event4 {};
        struct event5 {};

        
        struct State1 : public msm::front::state<>{};
        struct State2 : public msm::front::state<>{};
        struct State3 : public msm::front::state<>{};
        struct State4 : public msm::front::state<>{};
        struct State5 : public msm::front::state<>{};
        struct Final  : public msm::front::state<>{};

        
        struct CallTarget
        {
            template<typename EVT, typename FSM, typename SRC, typename TGT>
            void operator()(EVT const& evt, FSM& fsm, SRC&, TGT&)
            {
                fsm.result_ = evt.call();
            }
        };

        
        using initial_state = State1;

        
        struct transition_table : mpl::vector<
        
        
        Row < State1  , event5      , State2                                               >,
        Row < State1  , event1      , State3                                               >,
        
        Row < State2  , event2      , State4                                               >,
        
        Row < State3  , none        , State3                                               >,
        
        Row < State4  , event4      , State1                                               >,
        Row < State4  , event3      , State5                                               >,
        
        Row < State5  , E           , Final,    CallTarget                                 >
        
        > {};

        using StateMachine = msm::back::state_machine<Machine<E, R>>;

        template<typename F, typename... Args>
        struct Run
        {
            static inline void run(StateMachine& machine, F f, Args&&... args)
            {
                

                machine.start();

                
                Unroller<55 + MetaRandom<__COUNTER__, 44>::value>{}([&]()
                {
                    machine.process_event(event5{});
                    machine.process_event(event2{});
                    machine.process_event(event4{});
                });

                machine.process_event(event5{});
                machine.process_event(event2{});
                machine.process_event(event3{});
                
                machine.process_event(E{f, args...});
            }
        };

        
        R result_;
    };

}}}


#pragma warning(push)
#pragma warning(disable : 4068)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#define OBFUSCATED_CALL0(f) andrivet::ADVobfuscator::ObfuscatedCall<andrivet::ADVobfuscator::Machine1::Machine>(MakeObfuscatedAddress(f, andrivet::ADVobfuscator::MetaRandom<__COUNTER__, 400>::value + 278))
#define OBFUSCATED_CALL_RET0(R, f) andrivet::ADVobfuscator::ObfuscatedCallRet<andrivet::ADVobfuscator::Machine1::Machine, R>(MakeObfuscatedAddress(f, andrivet::ADVobfuscator::MetaRandom<__COUNTER__, 400>::value + 278))

#define OBFUSCATED_CALL(f, ...) andrivet::ADVobfuscator::ObfuscatedCall<andrivet::ADVobfuscator::Machine1::Machine>(MakeObfuscatedAddress(f, andrivet::ADVobfuscator::MetaRandom<__COUNTER__, 400>::value + 278), __VA_ARGS__)
#define OBFUSCATED_CALL_RET(R, f, ...) andrivet::ADVobfuscator::ObfuscatedCallRet<andrivet::ADVobfuscator::Machine1::Machine, R>(MakeObfuscatedAddress(f, andrivet::ADVobfuscator::MetaRandom<__COUNTER__, 400>::value + 278), __VA_ARGS__)

#pragma clang diagnostic pop
#pragma warning(pop)


#endif
