


















#ifndef ObfuscatedCallWithPredicate_h
#define ObfuscatedCallWithPredicate_h

#include "MetaFSM.h"
#include "MetaRandom.h"




namespace andrivet { namespace ADVobfuscator { namespace Machine2 {

    
    
    
    
    template<typename E, typename P, typename R = Void>
    struct Machine : public msm::front::state_machine_def<Machine<E, R>>
    {
        
        struct event1 {};
        struct event2 {};

        
        struct State1 : public msm::front::state<>{};
        struct State2 : public msm::front::state<>{};
        struct State3 : public msm::front::state<>{};
        struct State4 : public msm::front::state<>{};
        struct State5 : public msm::front::state<>{};
        struct State6 : public msm::front::state<>{};
        struct Final  : public msm::front::state<>{};

        
        struct CallTarget
        {
            template<typename EVT, typename FSM, typename SRC, typename TGT>
            void operator()(EVT const& evt, FSM& fsm, SRC&, TGT&)
            {
                fsm.result_ = evt.call();
            }
        };

        struct CallPredicate
        {
            template<typename EVT, typename FSM, typename SRC, typename TGT>
            void operator()(EVT const&, FSM& fsm, SRC&, TGT&)
            {
                fsm.predicateCounter_ += P{}();
            }
        };

        struct Increment
        {
            template<typename EVT, typename FSM, typename SRC, typename TGT>
            void operator()(EVT const&, FSM& fsm, SRC&, TGT&)
            {
                ++fsm.predicateCounter_;
            }
        };

        
        struct Predicate
        {
            template<typename EVT, typename FSM, typename SRC, typename TGT>
            bool operator()(EVT const&, FSM& fsm, SRC&, TGT&)
            {
                return (fsm.predicateCounter_ - fsm.predicateCounterInit_) % 2 == 0;
            }
        };

        struct NotPredicate
        {
            template<typename EVT, typename FSM, typename SRC, typename TGT>
            bool operator()(EVT const& evt, FSM& fsm, SRC& src, TGT& tgt)
            {
                return !Predicate{}(evt, fsm, src, tgt);
            }
        };

        
        using initial_state = State1;

        
        struct transition_table : mpl::vector<
        
        
        Row < State1  , event1      , State2                                               >,
        Row < State1  , E           , State5                                               >,
        
        Row < State2  , event1      , State3  , CallPredicate                              >,
        Row < State2  , event2      , State1  , none                , Predicate            >,
        Row < State2  , event2      , State4  , none                , NotPredicate         >,
        
        Row < State3  , event1      , State2  , Increment                                  >,
        
        Row < State4  , E           , State5  , CallTarget                                 >,
        
        Row < State5  , event2      , State6                                               >,
        
        Row < State6  , event1      , Final                                                >
        
        > {};

        using StateMachine = msm::back::state_machine<Machine<E, P, R>>;

        template<typename F, typename... Args>
        struct Run
        {
            static inline void run(StateMachine& machine, F f, Args&&... args)
            {
                machine.start();

                machine.process_event(event1{});

                
                
                
                Unroller<19 + 2 * MetaRandom<__COUNTER__, 40>::value>{}([&]()
                {
                    machine.process_event(event1{});
                    machine.process_event(event1{});
                });

                machine.process_event(event2{});

                
                machine.process_event(E{f, args...});

                machine.process_event(event2{});
                machine.process_event(event1{});
            }
        };


        
        R result_;

        
        static const int predicateCounterInit_ = 100 + MetaRandom<__COUNTER__, 999>::value;
        int predicateCounter_ = predicateCounterInit_;
    };

}}}



#pragma warning(push)
#pragma warning(disable : 4068)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#define OBFUSCATED_CALL_P0(P, f) andrivet::ADVobfuscator::ObfuscatedCallP<andrivet::ADVobfuscator::Machine2::Machine, P>(MakeObfuscatedAddress(f, andrivet::ADVobfuscator::MetaRandom<__COUNTER__, 400>::value + 278))
#define OBFUSCATED_CALL_RET_P0(R, P, f) andrivet::ADVobfuscator::ObfuscatedCallRetP<andrivet::ADVobfuscator::Machine2::Machine, P, R>(MakeObfuscatedAddress(f, andrivet::ADVobfuscator::MetaRandom<__COUNTER__, 400>::value + 278))

#define OBFUSCATED_CALL_P(P, f, ...) andrivet::ADVobfuscator::ObfuscatedCallP<andrivet::ADVobfuscator::Machine2::Machine, P>(MakeObfuscatedAddress(f, andrivet::ADVobfuscator::MetaRandom<__COUNTER__, 400>::value + 278), ##__VA_ARGS__)
#define OBFUSCATED_CALL_RET_P(R, P, f, ...) andrivet::ADVobfuscator::ObfuscatedCallRetP<andrivet::ADVobfuscator::Machine2::Machine, P, R>(MakeObfuscatedAddress(f, andrivet::ADVobfuscator::MetaRandom<__COUNTER__, 400>::value + 278), ##__VA_ARGS__)

#pragma clang diagnostic pop
#pragma warning(pop)


#endif
