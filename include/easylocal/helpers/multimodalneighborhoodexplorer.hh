#if !defined(_MULTIMODALNEIGHBORHOOD_EXPLORER_HH_)
#define _MULTIMODALNEIGHBORHOOD_EXPLORER_HH_

#include <stdexcept>
#include <functional>
#include <algorithm>

#include "easylocal/helpers/statemanager.hh"
#include "easylocal/helpers/prohibitionmanager.hh"
#include "easylocal/helpers/neighborhoodexplorer.hh"
#include "easylocal/utils/tuple.hh"

namespace EasyLocal {
  
  namespace Core {
    
    /** General rule: all moves are related. */
    template <class Move1, class Move2>
    bool IsRelated(const Move1& m1, const Move2& m2)
    {
      return true;
    }

    /** Variadic multi-modal neighborhood explorer. Generates a NeighborhoodExplorer whose move type is a tuple of ActiveMoves.*/
    template <class Input, class State, typename CFtype, class ... BaseNeighborhoodExplorers>
    class MultimodalNeighborhoodExplorer : public NeighborhoodExplorer<Input, State, std::tuple<ActiveMove<typename BaseNeighborhoodExplorers::MoveType> ...>, CFtype>
    {
    public:
  
      /** Tuple type representing the combination of BaseNeighborhoodExplorers' moves. */
      typedef std::tuple<ActiveMove<typename BaseNeighborhoodExplorers::MoveType> ...> MoveTypes;
  
      /** Tuple type representing references to BaseNeighborhoodExplorers' moves (because we need to set them). */
      typedef std::tuple<std::reference_wrapper<ActiveMove<typename BaseNeighborhoodExplorers::MoveType>> ...> MoveTypeRefs;
  
      /** Type of the NeighborhoodExplorer which has a tuple of moves as its move type. */
      typedef NeighborhoodExplorer<Input, State, MoveTypes, CFtype> SuperNeighborhoodExplorer;
  
      /** Tuple type representing references to BaseNeighborhoodExplorers. */
      typedef std::tuple<std::reference_wrapper<BaseNeighborhoodExplorers> ...> NeighborhoodExplorerTypes;
  
      /** Modality of the NeighborhoodExplorer */
      virtual unsigned int Modality() const { return sizeof...(BaseNeighborhoodExplorers); }
  
    protected:
  
      /** Constructor, takes a variable number of base NeighborhoodExplorers.  */
      MultimodalNeighborhoodExplorer(const Input& in, StateManager<Input,State,CFtype>& sm, std::string name, BaseNeighborhoodExplorers& ... nhes)
        : SuperNeighborhoodExplorer(in, sm, name), nhes(std::make_tuple(std::reference_wrapper<BaseNeighborhoodExplorers>(nhes) ...))
        { }
      
      /** Constructor, takes a variable number of base NeighborhoodExplorers.  */
      MultimodalNeighborhoodExplorer(const Input& in, StateManager<Input,State,CFtype>& sm, std::string name)
        : SuperNeighborhoodExplorer(in, sm, name), nhes(std::make_tuple(std::reference_wrapper<BaseNeighborhoodExplorers>(*std::make_shared<BaseNeighborhoodExplorers>(in, sm)) ...))
        { }
  
      /** Instantiated base NeighborhoodExplorers. */
      NeighborhoodExplorerTypes nhes;
    
      /** Struct to encapsulate a templated function call. Since the access to tuples must be done through compile-time recursion, at the moment of calling BaseNeighborhoodExplorers methods we don't know yet the type of the current NeighborhoodExplorer and move. This class is necessary so that we can defer the instantiation of the templated helper functions (DoMakeMove, DoRandomMove, ...) inside the calls to Compute* and Execute* (when they are known). */
      class Call
      {
      public:
    
        /** An enum representing the functions which can be called. */
        enum Function
        {
          INITIALIZE_INACTIVE,
          INITIALIZE_ACTIVE,
          MAKE_MOVE,
          FEASIBLE_MOVE,
          RANDOM_MOVE,
          FIRST_MOVE,
          TRY_NEXT_MOVE,
          IS_ACTIVE,
          DELTA_COST_FUNCTION,
          DELTA_VIOLATIONS
        };
    
        /** Constructor.
          @param f the Function to call. 
        */
        Call(Function f) : to_call(f) { }

        /** Method to generate a function with void return type. */
        template <class N>
        std::function<void(const N&, State&, ActiveMove<typename N::MoveType>&)> getVoid() const throw(std::logic_error)
        {
          std::function<void(const N&, State&, ActiveMove<typename N::MoveType>&)> f;
          switch (to_call)
          {
            case MAKE_MOVE:
            f = &DoMakeMove<N>;
            break;
            case INITIALIZE_INACTIVE:
            f = &InitializeInactive<N>;
            break;
            case INITIALIZE_ACTIVE:
            f = &InitializeActive<N>;
            break;
            case FIRST_MOVE:
            f = &DoFirstMove<N>;
            break;
            case RANDOM_MOVE:
            f = &DoRandomMove<N>;
            break;
            default:
            throw std::logic_error("Function not implemented");
          }
          return f;
        }

        /** Method to generate a function with CFtype return type. */
        template <class N>
        std::function<CFtype(const N&, State&, ActiveMove<typename N::MoveType>&)> getCFtype() const throw(std::logic_error)
        {
          std::function<CFtype(const N&, State&, ActiveMove<typename N::MoveType>&)> f;
          switch (to_call)
          {
            case DELTA_COST_FUNCTION:
            f = &DoDeltaCostFunction<N>;
            break;
            case DELTA_VIOLATIONS:
            f = &DoDeltaViolations<N>;
            break;
            default:
            throw std::logic_error("Function not implemented");
          }
          return f;
        }

        /** Method to generate a function with bool return type. */
        template <class N>
        std::function<bool(const N&, State&, ActiveMove<typename N::MoveType>&)> getBool() const throw(std::logic_error)
        {
          std::function<bool(const N&, State&, ActiveMove<typename N::MoveType>&)> f;
          switch (to_call)
          {
            case FEASIBLE_MOVE:
            f = &IsFeasibleMove<N>;
            break;
            case TRY_NEXT_MOVE:
            f = &TryNextMove<N>;
            break;
            case IS_ACTIVE:
            f = &IsActive<N>;
            break;
            default:
            throw std::logic_error("Function not implemented");
          }
          return f;
        }
    
        Function to_call;
      };
  
      /** Tuple dispatcher. Similarly to Call, this struct defines a number of templated functions that accept a tuple of moves and a tuple of neighborhood explorers, together with additional parameters. The struct is template-specialized to handle compile-time recursion. */
      template <class TupleOfMoves, class TupleOfNHEs, std::size_t N>
      struct TupleDispatcher
      {
        /** Run a function on a specific element of the tuples. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static void ExecuteAt(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c, int level)
        {
          // If we're at the right level of the recursion
          if (level == 0)
          {
            // Get right types (be nice to gcc)
            typedef typename std::tuple_element<N, TupleOfNHEs>::type::type CurrentNHE;
            typedef typename std::tuple_element<N, TupleOfMoves>::type::type CurrentMove;
            CurrentNHE& this_nhe = std::get<N>(temp_nhes).get();
            CurrentMove& this_move = std::get<N>(temp_moves).get();
       
            // Instantiate the function with the right template parameters
            std::function<void(const CurrentNHE &, State&,  CurrentMove &)> f =  c.template getVoid<CurrentNHE>();
        
            // Call on the last element
            f(this_nhe, st, this_move);
          }
          // Otherwise go down with recursion
          else if (level > 0)
            TupleDispatcher<TupleOfMoves, TupleOfNHEs, N - 1>::ExecuteAt(st, temp_moves, temp_nhes, c, --level);
#if defined(DEBUG)
          else
            throw std::logic_error("In function ExecuteAt (recursive case) level is less than zero");
#endif
        }
    
        /** Run a function on all the elements of the Moves (and NeighborhoodExplorers) tuple. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static void ExecuteAll(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c)
        {
          // Get right types (be nice to gcc)
          typedef typename std::tuple_element<N, TupleOfNHEs>::type::type CurrentNHE;
          typedef typename std::tuple_element<N, TupleOfMoves>::type::type CurrentMove;
          CurrentNHE& this_nhe = std::get<N>(temp_nhes).get();
          CurrentMove& this_move = std::get<N>(temp_moves).get();
       
          // Instantiate the function with the right template parameters
          std::function<void(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getVoid<CurrentNHE>();
      
          // Call recursively on first N-1 elements of the tuple, then call on the last element of the tuple
          TupleDispatcher<TupleOfMoves, TupleOfNHEs, N - 1>::ExecuteAll(st, temp_moves, temp_nhes, c);
          f(this_nhe, st, this_move);
        }
    
        /** Run a function on all the elements of tuples, then return the result. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static CFtype ComputeAt(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c, int level)
        {
          // If we're at the right level of the recursion
          if (level == 0)
          {
            // Get right types (be nice to gcc)
            typedef typename std::tuple_element<N, TupleOfNHEs>::type::type CurrentNHE;
            typedef typename std::tuple_element<N, TupleOfMoves>::type::type CurrentMove;
            CurrentNHE& this_nhe = std::get<N>(temp_nhes).get();
            CurrentMove& this_move = std::get<N>(temp_moves).get();

            // Instantiate the function with the right template parameters
            std::function<CFtype(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getCFtype<CurrentNHE>();
        
            // Call on the last element, return result
            return f(this_nhe, st, this_move);
          }
          // Otherwise, go down with recursion
          else if (level > 0)
            return TupleDispatcher<TupleOfMoves, TupleOfNHEs, N - 1>::ComputeAt(st, temp_moves, temp_nhes, c, --level);
#if defined(DEBUG)
          else
            throw std::logic_error("In function ComputeAt (recursive case) level is less than zero");
#endif
          // Just to avoid warnings from smart compilers
          return static_cast<CFtype>(0);
        }
    
        /** Run a function at all the levels of tuples, then return the sum of the result. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static CFtype ComputeAll(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c)
        {
          // Get right types (be nice to gcc)
          typedef typename std::tuple_element<N, TupleOfNHEs>::type::type CurrentNHE;
          typedef typename std::tuple_element<N, TupleOfMoves>::type::type CurrentMove;
          CurrentNHE& this_nhe = std::get<N>(temp_nhes).get();
          CurrentMove& this_move = std::get<N>(temp_moves).get();
      
          // Instantiate the function with the right template parameters
          std::function<CFtype(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getCFtype<CurrentNHE>();
      
          // Return aggregate result
          return TupleDispatcher<TupleOfMoves, TupleOfNHEs, N - 1>::ComputeAll(st, temp_moves, temp_nhes, c) + f(this_nhe, st, this_move);
        }

        /** Check a predicate on each element of the tuples and returns the vector of the corresponding boolean results. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static std::vector<bool> Check(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c)
        {
          // Get right types (be nice to gcc)
          typedef typename std::tuple_element<N, TupleOfNHEs>::type::type CurrentNHE;
          typedef typename std::tuple_element<N, TupleOfMoves>::type::type CurrentMove;
          CurrentNHE& this_nhe = std::get<N>(temp_nhes).get();
          CurrentMove& this_move = std::get<N>(temp_moves).get();

          // Instantiate the function with the right template parameters
          std::function<bool(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getBool<CurrentNHE>();

          // Go down with recursion and merge results
          std::vector<bool> current, others;
          current.push_back(f(this_nhe, st, this_move));
          others = TupleDispatcher<TupleOfMoves, TupleOfNHEs, N - 1>::Check(st, temp_moves, temp_nhes, c);
          current.insert(current.begin(), others.begin(), others.end());
          return current;
        }

        /** Check a predicate on each element of the tuples and returns true if all of them satisfies the predicate. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static bool CheckAll(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c)
        {
          // Get right types (be nice to gcc)
          typedef typename std::tuple_element<N, TupleOfNHEs>::type::type CurrentNHE;
          typedef typename std::tuple_element<N, TupleOfMoves>::type::type CurrentMove;
          CurrentNHE& this_nhe = std::get<N>(temp_nhes).get();
          CurrentMove& this_move = std::get<N>(temp_moves).get();
      
          // Instantiate the function with the right template parameters
          std::function<bool(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getBool<CurrentNHE&>();
      
          // Lazy evaluation
          if (!f(this_nhe, st, this_move))
            return false;
      
          return TupleDispatcher<TupleOfMoves, TupleOfNHEs, N - 1>::CheckAll(st, temp_moves, temp_nhes, c);
        }
    
        /** Check a predicate on each element of the tuples and returns true if at least one of them satisfies the predicate. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static bool CheckAny(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c)
        {
          // Get right types (be nice to gcc)
          typedef typename std::tuple_element<N, TupleOfNHEs>::type::type CurrentNHE;
          typedef typename std::tuple_element<N, TupleOfMoves>::type::type CurrentMove;
          CurrentNHE& this_nhe = std::get<N>(temp_nhes).get();
          CurrentMove& this_move = std::get<N>(temp_moves).get();
      
          // Instantiate the function with the right template parameters
          std::function<bool(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getBool<CurrentNHE>();
      
          // Lazy evaluation
          if (f(this_nhe, st, this_move))
            return true;
      
          return TupleDispatcher<TupleOfMoves, TupleOfNHEs, N - 1>::CheckAll(st, temp_moves, temp_nhes, c);
        }

        /** Check a predicate on a specific element of the tuples and returns true if it satisfies the predicate. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level tuple level at which to execute the function (0 is the last element, N - 1 is the first)
        */
        static bool CheckAt(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c, int level)
        {
          // If we're at the right level of the recursion
          if (level == 0)
          {
            // Get right types (be nice to gcc)
            typedef typename std::tuple_element<N, TupleOfNHEs>::type::type CurrentNHE;
            typedef typename std::tuple_element<N, TupleOfMoves>::type::type CurrentMove;
            CurrentNHE& this_nhe = std::get<N>(temp_nhes).get();
            CurrentMove& this_move = std::get<N>(temp_moves).get();
       
            // Instantiate the function with the right template parameters
            std::function<bool(const CurrentNHE&, State&,  CurrentMove&)> f =  c.template getBool<CurrentNHE>();
        
            return f(this_nhe, st, this_move);
          }
          // Otherwise, go down with recursion
          else if (level > 0)
            return TupleDispatcher<TupleOfMoves, TupleOfNHEs, N - 1>::CheckAt(st, temp_moves, temp_nhes, c, --level);
#if defined(DEBUG)
          else
            throw std::logic_error("In function CheckAt (recursive case) level is less than zero");
#endif
          // Just to avoid warnings from smart compilers
          return false;
        }
      };
  
      /** Base case of the recursive TupleDispatcher. */
      template <class TupleOfMoves, class TupleOfNHEs>
      struct TupleDispatcher<TupleOfMoves, TupleOfNHEs, 0>
      {
        /** Run a function on a specific element of the tuples. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static void ExecuteAt(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c, int level)
        {
          // If we're at the right level of the recursion
          if (level == 0)
          {
            // Get right types (be nice to gcc)
            typedef typename std::tuple_element<0, TupleOfNHEs>::type::type CurrentNHE;
            typedef typename std::tuple_element<0, TupleOfMoves>::type::type CurrentMove;
            CurrentNHE& this_nhe = std::get<0>(temp_nhes).get();
            CurrentMove& this_move = std::get<0>(temp_moves).get();

            // Instantiate the function with the right template parameters
            std::function<void(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getVoid<CurrentNHE>();
            f(this_nhe, st, this_move);
          }
#if defined(DEBUG)
          else
            throw std::logic_error("In function ExecuteAt (base case) level is not zero");
#endif
        }

        /** Run a function on all the elements of the Moves (and NeighborhoodExplorers) tuple. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static void ExecuteAll(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c)
        {
          // Get right types (be nice to gcc)
          typedef typename std::tuple_element<0, TupleOfNHEs>::type::type CurrentNHE;
          typedef typename std::tuple_element<0, TupleOfMoves>::type::type CurrentMove;
          CurrentNHE& this_nhe = std::get<0>(temp_nhes).get();
          CurrentMove& this_move = std::get<0>(temp_moves).get();

          // Instantiate the function with the right template parameters
          std::function<void(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getVoid<CurrentNHE>();
          f(this_nhe, st, this_move);
        }
    
        /** Run a function on all the elements of tuples, then return the result. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static CFtype ComputeAt(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c, int level)
        {
          // If we're at the right level of the recursion
          if (level == 0)
          {
            // Get right types (be nice to gcc)
            typedef typename std::tuple_element<0, TupleOfNHEs>::type::type CurrentNHE;
            typedef typename std::tuple_element<0, TupleOfMoves>::type::type CurrentMove;
            CurrentNHE& this_nhe = std::get<0>(temp_nhes).get();
            CurrentMove& this_move = std::get<0>(temp_moves).get();

            // Instantiate the function with the right template parameters
            std::function<CFtype(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getCFtype<CurrentNHE>();
            return f(this_nhe, st, this_move);
          }
#if defined(DEBUG)
          else
            throw std::logic_error("In function ComputeAt (base case) level is not zero");
#endif
          // Just to avoid warnings from smart compilers
          return static_cast<CFtype>(0);
        }
    
        /** Run a function at all the levels of tuples, then return the sum of the result. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static CFtype ComputeAll(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c)
        {
          // Get right types (be nice to gcc)
          typedef typename std::tuple_element<0, TupleOfNHEs>::type::type CurrentNHE;
          typedef typename std::tuple_element<0, TupleOfMoves>::type::type CurrentMove;
          CurrentNHE& this_nhe = std::get<0>(temp_nhes).get();
          CurrentMove& this_move = std::get<0>(temp_moves).get();
      
          // Instantiate the function with the right template parameters
          std::function<CFtype(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getCFtype<CurrentNHE>();
          return f(this_nhe, st, this_move);
        }

        /** Check a predicate on each element of the tuples and returns the vector of the corresponding boolean results. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static std::vector<bool> Check(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c)
        {
          // Get right types (be nice to gcc)
          typedef typename std::tuple_element<0, TupleOfNHEs>::type::type CurrentNHE;
          typedef typename std::tuple_element<0, TupleOfMoves>::type::type CurrentMove;
          CurrentNHE& this_nhe = std::get<0>(temp_nhes).get();
          CurrentMove& this_move = std::get<0>(temp_moves).get();

          // Instantiate the function with the right template parameters
          std::function<bool(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getBool<CurrentNHE>();
          std::vector<bool> current;
          current.push_back(f(this_nhe, st, this_move));
          return current;
        }
    
        /** Check a predicate on each element of the tuples and returns true if all of them satisfies the predicate. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static bool CheckAll(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c)
        {
          // Get right types (be nice to gcc)
          typedef typename std::tuple_element<0, TupleOfNHEs>::type::type CurrentNHE;
          typedef typename std::tuple_element<0, TupleOfMoves>::type::type CurrentMove;
          CurrentNHE& this_nhe = std::get<0>(temp_nhes).get();
          CurrentMove& this_move = std::get<0>(temp_moves).get();
      
          // Instantiate the function with the right template parameters
          std::function<bool(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getBool<CurrentNHE>();
          return f(this_nhe, st, this_move);
        }

        /** Check a predicate on each element of the tuples and returns true if at least one of them satisfies the predicate. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static bool CheckAny(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c)
        {
          // Get right types (be nice to gcc)
          typedef typename std::tuple_element<0, TupleOfNHEs>::type::type CurrentNHE;
          typedef typename std::tuple_element<0, TupleOfMoves>::type::type CurrentMove;
          CurrentNHE& this_nhe = std::get<0>(temp_nhes).get();
          CurrentMove& this_move = std::get<0>(temp_moves).get();
      
          // Instantiate the function with the right template parameters
          std::function<bool(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getBool<CurrentNHE>();
          return f(this_nhe, st, this_move);
        }

        /** Check a predicate on a specific element of the tuples and returns true if it satisfies the predicate. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static bool CheckAt(State& st, TupleOfMoves& temp_moves, const TupleOfNHEs& temp_nhes, const Call& c, int level)
        {
          // If we're at the right level of the recursion
          if (level == 0)
          {
            // Get right types (be nice to gcc)
            typedef typename std::tuple_element<0, TupleOfNHEs>::type::type CurrentNHE;
            typedef typename std::tuple_element<0, TupleOfMoves>::type::type CurrentMove; 
            CurrentNHE& this_nhe = std::get<0>(temp_nhes).get();
            CurrentMove& this_move = std::get<0>(temp_moves).get();

            // Instantiate the function with the right template parameters
            std::function<bool(const CurrentNHE&, State&, CurrentMove&)> f =  c.template getBool<CurrentNHE>();
            return f(this_nhe, st, this_move);
          }
#if defined(DEBUG)
          else
            throw std::logic_error("In function CheckAt (base case) level is not zero");
#endif
          // Just to avoid warnings from smart compilers
          return false;
        }
      };
  
      /** @copydoc TupleDispatcher::ExecuteAt */
      template <class ...NHEs>
      static void ExecuteAt(State& st, std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>& moves,
      const std::tuple<std::reference_wrapper<NHEs>...>& nhes, const Call& c, int index)
      {
        TupleDispatcher<std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>, std::tuple<std::reference_wrapper<NHEs>...>, sizeof...(NHEs) - 1>::ExecuteAt(st, moves, nhes, c, (sizeof...(NHEs) - 1) - index);
      }
  
      /** @copydoc TupleDispatcher::ExecuteAll */
      template <class ...NHEs>
      static void ExecuteAll(State& st, std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>& moves,
      const std::tuple<std::reference_wrapper<NHEs>...>& nhes, const Call& c)
      {
        TupleDispatcher<std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>, std::tuple<std::reference_wrapper<NHEs>...>, sizeof...(NHEs) - 1>::ExecuteAll(st, moves, nhes, c);
      }
  
      /** @copydoc TupleDispatcher::ComputeAt */
      template <class ...NHEs>
      static CFtype ComputeAt(State& st, std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>& moves,
      const std::tuple<std::reference_wrapper<NHEs>...>& nhes, const Call& c, int index)
      {
        return TupleDispatcher<std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>, std::tuple<std::reference_wrapper<NHEs>...>, sizeof...(NHEs) - 1>::ComputeAt(st, moves, nhes, c, (sizeof...(NHEs) - 1) - index);
      }
  
      /** @copydoc TupleDispatcher::ComputeAll */
      template <class ...NHEs>
      static CFtype ComputeAll(State& st, std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>& moves,
      const std::tuple<std::reference_wrapper<NHEs>...>& nhes, const Call& c)
      {
        return TupleDispatcher<std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>, std::tuple<std::reference_wrapper<NHEs>...>, sizeof...(NHEs) - 1>::ComputeAll(st, moves, nhes, c);
      }
  
      /** @copydoc TupleDispatcher::Check */
      template <class ...NHEs>
      static std::vector<bool> Check(State& st, std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>& moves,
      const std::tuple<std::reference_wrapper<NHEs>...>& nhes, const Call& c)
      {
        return TupleDispatcher<std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>, std::tuple<std::reference_wrapper<NHEs>...>, sizeof...(NHEs) - 1>::Check(st, moves, nhes, c);
      }
  
      /** @copydoc TupleDispatcher::CheckAll */
      template <class ...NHEs>
      static bool CheckAll(State& st, std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>& moves,
      const std::tuple<std::reference_wrapper<NHEs>...>& nhes, const Call& c)
      {
        return TupleDispatcher<std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>, std::tuple<std::reference_wrapper<NHEs>...>, sizeof...(NHEs) - 1>::CheckAll(st, moves, nhes, c);
      }
  
      /** @copydoc TupleDispatcher::CheckAny */
      template <class ...NHEs>
      static bool CheckAny(State& st, std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>& moves,
      const std::tuple<std::reference_wrapper<NHEs>...>& nhes, const Call& c)
      {
        return TupleDispatcher<std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>, std::tuple<std::reference_wrapper<NHEs>...>, sizeof...(NHEs) - 1>::CheckAny(st, moves, nhes, c);
      }
  
      /** @copydoc TupleDispatcher::CheckAt */
      template <class ...NHEs>
      static CFtype CheckAt(State& st, std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>& moves,
      const std::tuple<std::reference_wrapper<NHEs>...>& nhes, const Call& c, int index)
      {
        return TupleDispatcher<std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>, std::tuple<std::reference_wrapper<NHEs>...>, sizeof...(NHEs) - 1>::CheckAt(st, moves, nhes, c, (sizeof...(NHEs) - 1) - index);
      }
  
      /** Checks if a move is active.
          @param n a reference to the move's NeighborhoodExplorer
          @param s a reference to the current State
          @param m a reference to the ActiveMove to check
      */
      template<class N>
      static bool IsActive(const N& n, State& s, ActiveMove<typename N::MoveType>& m)
      {
        return m.active;
      }

      /** Generates a random move in the neighborhood.
          @param n a reference to the NeighborhoodExplorer
          @param s a reference to the current State
          @param m a reference to the ActiveMove which must be set
      */
      template<class N>
      static void DoRandomMove(const N& n, State& s, ActiveMove<typename N::MoveType>& m)
      {
        n.RandomMove(s, m);
        m.active = true;
      }

      /** Get the first move of the neighborhood
          @param n a reference to the move's NeighborhoodExplorer
          @param s a reference to the current State
          @param m a reference to the ActiveMove which must be set
      */
      template<class N>
      static void DoFirstMove(const N& n, State& s, ActiveMove<typename N::MoveType>& m)
      {
        n.FirstMove(s, m);
        m.active = true;
      }

      /** Try to produce the next move of the neighborhood
          @param n a reference to the move's NeighborhoodExplorer
          @param s a reference to the current State
          @param m a reference to the current move, where the next move will be written
      */
      template<class N>
      static bool TryNextMove(const N& n, State& s, ActiveMove<typename N::MoveType>& m)
      {
        m.active = n.NextMove(s, m);
        return m.active;
      }
  
      /** Executes the move on the state, modifies it
          @param n a reference to the move's NeighborhoodExplorer
          @param s a reference to the current State
          @param m a reference to the current move
      */
      template<class N>
      static void DoMakeMove(const N& n, State& s, ActiveMove<typename N::MoveType>& m)
      {
        if (m.active)
          n.MakeMove(s,m);
      }

      /** Checks if a move is feasible.
          @param n a reference to the move's NeighborhoodExplorer
          @param s a reference to the current State
          @param m a reference to the ActiveMove to check
      */
      template<class N>
      static bool IsFeasibleMove(const N& n, State& s, ActiveMove<typename N::MoveType>& m)
      {
        if (m.active) 
          return n.FeasibleMove(s, m);
        else
          return true;
      }

      /** Sets the active flag of an ActiveMove to false
          @param n a reference to the move's NeighborhoodExplorer
          @param s a reference to the current State
          @param m a reference to the ActiveMove to check
      */
      template<class N>
      static void InitializeInactive(const N& n, State& s, ActiveMove<typename N::MoveType>& m)
      {
        m.active = false;
      }

      /** Sets the active flag of an ActiveMove to true
          @param n a reference to the move's NeighborhoodExplorer
          @param s a reference to the current State
          @param m a reference to the ActiveMove to check
      */
      template<class N>
      static void InitializeActive(const N& n, State& s, ActiveMove<typename N::MoveType>& m)
      {
        m.active = true;
      }
  
      /** Computes the cost of making a move on a state
          @param n a reference to the move's NeighborhoodExplorer
          @param s a reference to the current State
          @param m a reference to the ActiveMove to check
      */
      template<class N>
      static CFtype DoDeltaCostFunction(const N& n, State& s, ActiveMove<typename N::MoveType>& m)
      {
        return n.DeltaCostFunction(s, m);
      }
  
      /** Computes the hard cost of making a move on a state
          @param n a reference to the move's NeighborhoodExplorer
          @param s a reference to the current State
          @param m a reference to the ActiveMove to check
      */
      template<class N>
      static CFtype DoDeltaViolations(const N& n, State& s, ActiveMove<typename N::MoveType>& m)
      {
        return n.DeltaViolations(s, m);
      }
  
    };


    /** A multi-modal NeighborhoodExplorer that generates the union of multiple neighborhood explorers. */
    template <class Input, class State, typename CFtype, class ... BaseNeighborhoodExplorers>
    class SetUnionNeighborhoodExplorer : public MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...>
    {
    private:
  
      /** Type of the NeighborhoodExplorer which has a tuple of moves as its move type. */
      typedef MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...> SuperNeighborhoodExplorer;
  
      /** Tuple type representing references to BaseNeighborhoodExplorers' moves (because we need to set them). */
      typedef typename SuperNeighborhoodExplorer::MoveTypeRefs MoveTypeRefs;
  
      /** Alias to SuperNeighborhoodExplorer::Call. */
      typedef typename SuperNeighborhoodExplorer::Call Call;
  
    public:
  
      /** Inherit constructor from superclass. Not yet supported by all compilers. */
      // using MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...>::MultimodalNeighborhoodExplorer;

      SetUnionNeighborhoodExplorer(Input& in, StateManager<Input,State,CFtype>& sm, std::string name, BaseNeighborhoodExplorers& ... nhes)
        : MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...>(in, sm, name, nhes ...)
      {
        // If not otherwise specified, initialize the probabilities as 1 / Modality
        this->bias = std::vector<double>(this->Modality(), 1.0 / (double) this->Modality());
      }

      /** Constructor, takes a variable number of base NeighborhoodExplorers.  */
      SetUnionNeighborhoodExplorer(const Input& in, StateManager<Input,State,CFtype>& sm, std::string name)
        : MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...>(in, sm, name)
      { 
        // If not otherwise specified, initialize the probabilities as 1 / Modality
        this->bias = std::vector<double>(this->Modality(), 1.0 / (double) this->Modality());
      }
  
      SetUnionNeighborhoodExplorer(Input& in, StateManager<Input,State,CFtype>& sm, std::string name, std::vector<double> bias, BaseNeighborhoodExplorers& ... nhes)
        : MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...>(in, sm, name, nhes ...), bias(bias)
          { }
      
      
      /** Constructor, takes a variable number of base NeighborhoodExplorers.  */
      SetUnionNeighborhoodExplorer(const Input& in, StateManager<Input,State,CFtype>& sm, std::string name, std::vector<double> bias)
        : MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...>(in, sm, name), bias(bias)
          { }
  


      /** @copydoc NeighborhoodExplorer::RandomMove */
      virtual void RandomMove(const State& st, typename SuperNeighborhoodExplorer::MoveType& moves) const throw(EmptyNeighborhood)
      {
        // Select random neighborhood explorer with bias (don't assume that they sum up to one)
        double total_bias = 0.0;
        double pick;
        unsigned int selected = 0;

        for (unsigned int i = 0; i < bias.size(); i++)
          total_bias += bias[i];
        pick = Random::Double(0.0, total_bias);

        // Subtract bias until we're on the right neighborhood explorer
        while(pick > this->bias[selected])
        {
          pick -= this->bias[selected];
          selected++;
        }
    
        Call initialize_inactive(Call::Function::INITIALIZE_INACTIVE);
        Call random_move(SuperNeighborhoodExplorer::Call::Function::RANDOM_MOVE);
    
        // Convert to references
    
        MoveTypeRefs r_moves = to_refs(moves);
    
        // Set all actions to inactive
        SuperNeighborhoodExplorer::ExecuteAll(const_cast<State&>(st), r_moves, this->nhes, initialize_inactive);
    
        // Call NeighborhoodExplorer::RandomMove on the selected NeighborhoodExplorer 
        SuperNeighborhoodExplorer::ExecuteAt(const_cast<State&>(st), r_moves, this->nhes, random_move, selected);
      }
  
      /** @copydoc NeighborhoodExplorer::FirstMove */
      virtual void FirstMove(const State& st, typename SuperNeighborhoodExplorer::MoveType& moves) const throw(EmptyNeighborhood)
      {
        // Select first NeighborhoodExplorer
        unsigned int selected = 0;
 
        Call initialize_inactive(Call::Function::INITIALIZE_INACTIVE);
        Call first_move(Call::Function::FIRST_MOVE);
    
        // Convert to references
        MoveTypeRefs r_moves = to_refs(moves);
    
        // Set all actions to inactive
        SuperNeighborhoodExplorer::ExecuteAll(const_cast<State&>(st), r_moves, this->nhes, initialize_inactive);
    
        // Try picking the first move, if this doesn't work, get to the next NeighborhoodExplorer (and so on)
        while (selected < this->Modality())
        {
          try
          {
            SuperNeighborhoodExplorer::ExecuteAt(const_cast<State&>(st), r_moves, this->nhes, first_move, selected);
            break;
          }
          catch (EmptyNeighborhood e)
            { }
          selected++;
        }
    
        // If even the last NeighborhoodExplorer has an EmptyNeighborhood, throw an EmptyNeighborhood for the SetUnionNeighborhoodExplorer
        if (selected == this->Modality())
          throw EmptyNeighborhood();
      }
  
      /** @copydoc NeighborhoodExplorer::NextMove */
      virtual bool NextMove(const State& st, typename SuperNeighborhoodExplorer::MoveType& moves) const
      {
        // Select the current active NeighborhoodExplorer
        unsigned int selected = this->CurrentActiveMove(st, moves);
    
        Call try_next_move(Call::Function::TRY_NEXT_MOVE);
    
        // Convert to references
        MoveTypeRefs r_moves = to_refs(moves);    
    
        // If the current NeighborhoodExplorer has a next move, return true
        if (SuperNeighborhoodExplorer::CheckAt(const_cast<State&>(st), r_moves, this->nhes, try_next_move, selected))
          return true;
    
        // Otherwise, get to the next one
        do
        {
          selected++;
          if (selected == this->Modality())
            return false;
          try
          {
            // Call FirstMove on the current NeighborhoodExplorer
            Call first_move(Call::Function::FIRST_MOVE);
            SuperNeighborhoodExplorer::ExecuteAt(const_cast<State&>(st), r_moves, this->nhes, first_move, selected);
            return true;
          }
          catch (EmptyNeighborhood e)
            { }
        } while (true);
    
        return false;
      }
  
      /** @copydoc NeighborhoodExplorer::DeltaCostFunction */
      virtual CFtype DeltaCostFunction(const State& st, const typename SuperNeighborhoodExplorer::MoveType& moves) const
      {
        // Select the current active NeighborhoodExplorer
        unsigned int selected = this->CurrentActiveMove(st, moves);
    
        // Compute delta cost
        Call delta_cost_function(Call::Function::DELTA_COST_FUNCTION);
    
        // Convert to references to non-const
        auto c_moves = const_cast<decltype(moves)>(moves);
        MoveTypeRefs r_moves = to_refs(c_moves);
        // Return cost
        return SuperNeighborhoodExplorer::ComputeAt(const_cast<State&>(st), r_moves, this->nhes, delta_cost_function, selected);
      }

      /** @copydoc NeighborhoodExplorer::DeltaViolations */
      virtual CFtype DeltaViolations(const State& st, const typename SuperNeighborhoodExplorer::MoveType& moves) const
      {
        // Select the current active NeighborhoodExplorer
        unsigned int selected = this->CurrentActiveMove(st, moves);
    
        // Compute delta cost
        Call delta_violations(Call::Function::DELTA_VIOLATIONS);
    
        // Convert to references to non-const
        auto c_moves = const_cast<decltype(moves)>(moves);
        MoveTypeRefs r_moves = to_refs(c_moves);
        // Return cost
        return SuperNeighborhoodExplorer::ComputeAt(const_cast<State&>(st), r_moves, this->nhes, delta_violations, selected);
      }

      /** @copydoc NeighborhoodExplorer::MakeMove */
      virtual void MakeMove(State& st, const typename SuperNeighborhoodExplorer::MoveType& moves) const
      {
        // Select current active move
        unsigned int selected = this->CurrentActiveMove(st, moves);
        Call make_move(Call::Function::MAKE_MOVE);
    
        // Convert to references to non-const
        auto c_moves = const_cast<decltype(moves)>(moves);
        MoveTypeRefs r_moves = to_refs(c_moves);
        // Execute move on state
        SuperNeighborhoodExplorer::ExecuteAt(st, r_moves, this->nhes, make_move, selected);
      }
  
      /** @copydoc NeighborhoodExplorer::FeasibleMove */
      virtual bool FeasibleMove(const State& st, const typename SuperNeighborhoodExplorer::MoveType& moves) const
      {
        // Select current active move
        unsigned int selected = this->CurrentActiveMove(st, moves);
        Call feasible_move(Call::Function::FEASIBLE_MOVE);
    
        // Convert to references to non-const
        auto c_moves = const_cast<decltype(moves)>(moves);
        MoveTypeRefs r_moves = to_refs(c_moves);
        // Check if current move is feasible
        return SuperNeighborhoodExplorer::CheckAt(const_cast<State&>(st), r_moves, this->nhes, feasible_move, selected);
      }
  
    protected:
  
      std::vector<double> bias;

      /** Computes the index of the current active move.
          @param st current state
          @param moves tuple of moves
      */
      int CurrentActiveMove(const State& st, const typename SuperNeighborhoodExplorer::MoveType& moves) const
      {
        Call is_active(Call::Function::IS_ACTIVE);
        auto c_moves = const_cast<decltype(moves)>(moves);
        MoveTypeRefs r_moves = to_refs(c_moves);
        std::vector<bool> moves_status = SuperNeighborhoodExplorer::Check(const_cast<State&>(st), r_moves, this->nhes, is_active);

        return (int)std::distance(moves_status.begin(), std::find_if(moves_status.begin(), moves_status.end(), [](bool element) { return element; }));
      }
    };

    /** A multi-modal NeighborhoodExplorer that generates the cartesian product of multiple neighborhood explorers. */
    template <class Input, class State, typename CFtype, class ... BaseNeighborhoodExplorers>
    class CartesianProductNeighborhoodExplorer : public MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...>
    {
    private:
  
      /** Type of the NeighborhoodExplorer which has a tuple of moves as its move type. */
      typedef MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...> SuperNeighborhoodExplorer;
  
      /** Tuple type representing references to BaseNeighborhoodExplorers' moves (because we need to set them). */
      typedef typename SuperNeighborhoodExplorer::MoveTypeRefs MoveTypeRefs;
  
      /** Alias to SuperNeighborhoodExplorer::Call */
      typedef typename SuperNeighborhoodExplorer::Call Call;
  
      typedef CartesianProductNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers...> ThisNeighborhoodExplorer;
  
      /** Tuple dispatcher specialization, to handle comparisons between moves. */
      template <class TupleOfMoves, class TupleOfNHEs, std::size_t N>
      struct TupleDispatcher : public SuperNeighborhoodExplorer::template TupleDispatcher<TupleOfMoves, TupleOfNHEs, N>
      {
        /** Compare (n)th and (n+1)th moves according to a predicate. We don't have base case because we don't want to get down to zero.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static bool CompareMovesAt(State& st, TupleOfMoves& temp_moves, ThisNeighborhoodExplorer& cp_nhe, int level)
        {
          // If we're at the right level of the recursion
          if (level == 0)
          {
            // Get last element of the tuples
            auto this_move = std::get<N-1>(temp_moves).get().RawMove();
            auto next_move = std::get<N>(temp_moves).get().RawMove();
        
            // Call on the last element
            return IsRelated(this_move, next_move);
          }
          // Otherwise go down with recursion
          else if (level > 0)
            return TupleDispatcher<TupleOfMoves, TupleOfNHEs, N-1>::CompareMovesAt(st, temp_moves, cp_nhe, --level);
      
#if defined(DEBUG)
          else
            throw std::logic_error("In function CompareMovesAt (recursive case) level is less than zero");
#endif
          // Just to accomodate smart compilers
          return false;
        }
    
        /** Compare (n)th and (n+1)th moves according to a predicate. We don't have base case because we don't want to get down to zero.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static std::vector<bool> CompareMoves(State& st, TupleOfMoves& temp_moves, ThisNeighborhoodExplorer& cp_nhe)
        {
          std::vector<bool> current, others;
          others = TupleDispatcher<TupleOfMoves, TupleOfNHEs, N-1>::CompareMoves(st, temp_moves, cp_nhe);
      
          // Get last element of the tuples
          auto this_move = std::get<N-1>(temp_moves).get().RawMove();
          auto next_move = std::get<N>(temp_moves).get().RawMove();
      
          // Call on the last element
          current.push_back(IsRelated(this_move, next_move));
          current.insert(current.begin(), others.begin(), others.end());
      
          return current;
        }
      };
  
      /** Base case of the recursive TupleDispatcher. */
      template <class TupleOfMoves, class TupleOfNHEs>
      struct TupleDispatcher<TupleOfMoves, TupleOfNHEs, 0>
      {
        /** Run a function on a specific element of the tuples. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static bool CompareMovesAt(State& st, TupleOfMoves& temp_moves, const ThisNeighborhoodExplorer& cp_nhe, int level)
        {
          // If we're at the right level of the recursion
          if (level == 0)
          {
            return true;
          }
#if defined(DEBUG)
          else
            throw std::logic_error("In function CompareMovesAt (base case) level is not zero");
#endif
          // just to prevent warnings from smart compilers
          return false;
        }
    
        /** Run a function on a specific element of the tuples. Based on compile-time recursion.
            @param st reference to the current state
            @param temp_moves tuple of references to ActiveMoves
            @param temp_nhes tuple of references to associated NeighborhoodExplorers
            @param level level at which to execute the function
        */
        static std::vector<bool> CompareMoves(State& st, TupleOfMoves& temp_moves, const ThisNeighborhoodExplorer& cp_nhe)
        {
          return std::vector<bool>(0);
        }
      };

  
      /** Checks that a move is related to the previous one.
          @param st a reference to the current State
          @param moves a reference to the current tuple of moves
          @param nhes a reference to the current tuple of NeighborhoodExplorers
          @param c wrapper to a predicate to compare two moves
          @param index index of the first move 
      */
      template <class ...NHEs>
      static bool CompareMovesAt(State& st, std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>& moves, const ThisNeighborhoodExplorer& cp_nhe, int index)
      {
        return TupleDispatcher<std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>, std::tuple<std::reference_wrapper<NHEs>...>, sizeof...(NHEs)-1>::CompareMovesAt(st, moves, const_cast<ThisNeighborhoodExplorer&>(cp_nhe), (sizeof...(NHEs) - 1) - index);
      }
  
      /** Checks that a move is related to the previous one.
          @param st a reference to the current State
          @param moves a reference to the current tuple of moves
          @param nhes a reference to the current tuple of NeighborhoodExplorers
          @param c wrapper to a predicate to compare two moves
          @param level index of the first move
      */
      template <class ...NHEs>
      static std::vector<bool> CompareMoves(State& st, std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>& moves, const ThisNeighborhoodExplorer& cp_nhe)
      {
        return TupleDispatcher<std::tuple<std::reference_wrapper<ActiveMove<typename NHEs::MoveType>>...>, std::tuple<std::reference_wrapper<NHEs>...>, sizeof...(NHEs)-1>::CompareMoves(st, moves, const_cast<ThisNeighborhoodExplorer&>(cp_nhe));
      }
  
  
  
#if defined(DEBUG)
      /** Check that all the ActiveMoves, inside a tuple of moves, are active. */
      void VerifyAllActives(const State& st, const typename SuperNeighborhoodExplorer::MoveType& moves) const throw (std::logic_error)
      {
        Call is_active(SuperNeighborhoodExplorer::Call::Function::IS_ACTIVE);
        auto c_moves = const_cast<decltype(moves)>(moves);
        MoveTypeRefs r_moves = to_refs(c_moves);
        std::vector<bool> active = SuperNeighborhoodExplorer::Check(const_cast<State&>(st), r_moves, this->nhes, is_active);
        for (bool v : active)
        {
          if (!v)
            throw std::logic_error("Some of the moves were not active in a composite CartesianProduct neighborhood explorer");
        }
      }
  
      void VerifyAllRelated(const State& st, const typename SuperNeighborhoodExplorer::MoveType& moves) const throw (std::logic_error)
      {
        auto c_moves = const_cast<decltype(moves)>(moves);
        MoveTypeRefs r_moves = to_refs(c_moves);
        std::vector<bool> are_related = CompareMoves<BaseNeighborhoodExplorers...>(const_cast<State&>(st), r_moves, *this);
        for (bool v : are_related)
        {
          if (!v)
            throw std::logic_error("Some of the moves were not related in a composite CartesianProduct neighborhood explorer");
        }
      }
#endif
  
    public:

      /** Tuple type representing references to BaseNeighborhoodExplorers. */
      typedef std::tuple<std::reference_wrapper<BaseNeighborhoodExplorers> ...> NeighborhoodExplorerTypes;
  
      /** Inherit constructor from superclass. Not yet supported by all compilers. */
      // using MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...>::MultimodalNeighborhoodExplorer;
  
      CartesianProductNeighborhoodExplorer(Input& in, StateManager<Input,State,CFtype>& sm, std::string name, BaseNeighborhoodExplorers& ... nhes)
        : MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...>(in, sm, name, nhes ...)
          { }
      
      /** Constructor, takes a variable number of base NeighborhoodExplorers.  */
      CartesianProductNeighborhoodExplorer(const Input& in, StateManager<Input,State,CFtype>& sm, std::string name)
        : MultimodalNeighborhoodExplorer<Input, State, CFtype, BaseNeighborhoodExplorers ...>(in, sm, name)
          { }
  
      /** @copydoc NeighborhoodExplorer::RandomMove */
      virtual void RandomMove(const State& st, typename SuperNeighborhoodExplorer::MoveType& moves) const throw(EmptyNeighborhood)
      {
        Call random_move(SuperNeighborhoodExplorer::Call::Function::RANDOM_MOVE);
        Call make_move(SuperNeighborhoodExplorer::Call::Function::MAKE_MOVE);

        // Convert to references
        MoveTypeRefs r_moves = to_refs(moves);
    
        // The chain of states generated during the execution of the multimodal move (including the first one, but excluding the last)
        std::vector<State> temp_states(this->Modality(), State(this->in));
    
        temp_states[0] = st;
        temp_states[1] = temp_states[0];
    
        // Generate first random move starting from the initial state
        SuperNeighborhoodExplorer::ExecuteAt(temp_states[0], r_moves, this->nhes, random_move, 0);
    
        // Make move (otherwise we wouldn't have second state for generating the second random)
        SuperNeighborhoodExplorer::ExecuteAt(temp_states[1], r_moves, this->nhes, make_move, 0);
    
        // Generate all the random moves of the tuple
        for (unsigned int i = 1; i < this->Modality(); i++)
        {
          // Duplicate current state (to process it later)
          temp_states[i+1] = temp_states[i];
      
          do {
            // Generate next random move starting from the current state
            SuperNeighborhoodExplorer::ExecuteAt(temp_states[i], r_moves, this->nhes, random_move, i);
          }
          while (!CompareMovesAt<BaseNeighborhoodExplorers...>(temp_states[i], r_moves, *this, i));
      
          // Make move (otherwise we don't have next state for generating the next random)
          SuperNeighborhoodExplorer::ExecuteAt(temp_states[i+1], r_moves, this->nhes, make_move, i);
      
        }
    
#if defined(DEBUG)
        VerifyAllActives(st, moves);
#endif

      }
  
      /** @copydoc NeighborhoodExplorer::FirstMove */
      virtual void FirstMove(const State& st, typename SuperNeighborhoodExplorer::MoveType& moves) const throw(EmptyNeighborhood)
      {
        Call first_move(Call::Function::FIRST_MOVE);
        Call make_move(Call::Function::MAKE_MOVE);
        Call try_next_move(Call::Function::TRY_NEXT_MOVE);
    
        // Convert to references
        MoveTypeRefs r_moves = to_refs(moves);
    
        // The chain of states generated during the execution of the multimodal move (including the first one, but excluding the last)
        std::vector<State> temp_states(this->Modality(), State(this->in));
    
        temp_states[0] = st;
        temp_states[1] = temp_states[0];
    
        // Generate first move starting from the initial state
        SuperNeighborhoodExplorer::ExecuteAt(temp_states[0], r_moves, this->nhes, first_move, 0);
    
        // If modality is 1 we're finished
        if (this->Modality() == 1)
          return;
    
        // Execute first move and save new state in next state
        SuperNeighborhoodExplorer::ExecuteAt(temp_states[1], r_moves, this->nhes, make_move, 0);
    
        int i = 1;
        while (i < static_cast<int>(this->Modality()))
        {      
          try
          {
            // Generate first move
            SuperNeighborhoodExplorer::ExecuteAt(temp_states[i], r_moves, this->nhes, first_move, i);
        
            while (!CompareMovesAt<BaseNeighborhoodExplorers...>(temp_states[i], r_moves, *this, i))
            {
              if (!SuperNeighborhoodExplorer::CheckAt(temp_states[i], r_moves, this->nhes, try_next_move, i))
                throw EmptyNeighborhood(); // just to enter backtracking
            }
        
            if (i == static_cast<int>(this->Modality()) - 1)
            {
              // The whole chain of moves has been dispatched
#if defined(DEBUG)
              VerifyAllActives(st, moves);
              VerifyAllRelated(st, moves);
#endif
              return;
            }
            // Duplicate current state
            temp_states[i+1] = temp_states[i];
        
            // Make move (otherwise we don't have next state for generating the next move)
            SuperNeighborhoodExplorer::ExecuteAt(temp_states[i+1], r_moves, this->nhes, make_move, i);
          }
          catch (EmptyNeighborhood e)
          {
            while (i > 0)
            {
              // Backtrack
              i--;
          
              // Reset state which was modified during visit
              temp_states[i+1] = temp_states[i];

              // Generate first move
              //SuperNeighborhoodExplorer::ExecuteAt(temp_states[i], r_moves, this->nhes, first_move, i);
          
              bool empty = false;
          
              do
              {
                // We can't throw an exception to backtrack since we're already backtracking
                if (!SuperNeighborhoodExplorer::CheckAt(temp_states[i], r_moves, this->nhes, try_next_move, i))
                  empty = true;
              }
              while (!CompareMovesAt<BaseNeighborhoodExplorers...>(temp_states[i], r_moves, *this, i));
          
              // a move to be performed has been found
              if (!empty)
              {
                // execute generated move
                SuperNeighborhoodExplorer::ExecuteAt(temp_states[i+1], r_moves, this->nhes, make_move, i);
                break;
              }
              else
                if (i == 0)
              {
                // No combination whatsoever could be extended as a "first move" on the first neighborhood
                throw EmptyNeighborhood();
              }
            }
          }
          i++;
        }
      }

      /** @copydoc NeighborhoodExplorer::NextMove */
      virtual bool NextMove(const State& st, typename SuperNeighborhoodExplorer::MoveType& moves) const
      {
        Call first_move(Call::Function::FIRST_MOVE);
        Call make_move(Call::Function::MAKE_MOVE);
        Call try_next_move(Call::Function::TRY_NEXT_MOVE);
    
        int i = 0;
    
        // Convert to references
        MoveTypeRefs r_moves = to_refs(moves);
    
        // The chain of states generated during the execution of the multimodal move (including the first one, but excluding the last)
        std::vector<State> temp_states(this->Modality(), State(this->in));
    
        temp_states[0] = st;
    
        // create and initialize all the remaining states in the chain
        for (unsigned int i = 1; i < this->Modality(); i++)
        {
          temp_states[i] = temp_states[i - 1];
          SuperNeighborhoodExplorer::ExecuteAt(temp_states[i], r_moves, this->nhes, make_move, i - 1);
        }
    
        // attempt to find a next move in the last component of the move tuple
        i = this->Modality() - 1;
        while (SuperNeighborhoodExplorer::CheckAt(temp_states[i], r_moves, this->nhes, try_next_move, i))
          if (CompareMovesAt<BaseNeighborhoodExplorers...>(temp_states[i], r_moves, *this, i))
            return true;

        bool backtracking = true;
        while (i < static_cast<int>(this->Modality()))
        {
          // backtracking to the first available component that has a next move
          while (backtracking && i > 0)
          {
            i--; // backtrack
            temp_states[i+1] = temp_states[i];
        
            while (SuperNeighborhoodExplorer::CheckAt(temp_states[i], r_moves, this->nhes, try_next_move, i))
            {
              if (CompareMovesAt<BaseNeighborhoodExplorers...>(temp_states[i], r_moves, *this, i))
              {
                SuperNeighborhoodExplorer::ExecuteAt(temp_states[i + 1], r_moves, this->nhes, make_move, i);
                i++;
                backtracking = false;
                break;
              }
            }
          }
      
          if (i == 0)
            return false;
          backtracking = false;
          // forward trying a set of first moves
          try
          {
        
            SuperNeighborhoodExplorer::ExecuteAt(temp_states[i], r_moves, this->nhes, first_move, i);
        
            while (!CompareMovesAt<BaseNeighborhoodExplorers...>(temp_states[i], r_moves, *this, i))
            {
              if (!SuperNeighborhoodExplorer::CheckAt(temp_states[i], r_moves, this->nhes, try_next_move, i))
                throw EmptyNeighborhood(); // just to enter backtracking
            }
        
            if (i == static_cast<int>(this->Modality()) - 1)
              return true;
        
            temp_states[i + 1] = temp_states[i];
        
            SuperNeighborhoodExplorer::ExecuteAt(temp_states[i + 1], r_moves, this->nhes, make_move, i);
            i++;
          }
          catch (EmptyNeighborhood e)
          {
            backtracking = true;
          }
        }
        return true;
      }
  
      /** @copydoc NeighborhoodExplorer::DeltaCostFunction */
      virtual CFtype DeltaCostFunction(const State& st, const typename SuperNeighborhoodExplorer::MoveType& moves) const
      {
#if defined(DEBUG)
        VerifyAllActives(st, moves);
        VerifyAllRelated(st, moves);
#endif

        Call do_delta_cost_function(Call::Function::DELTA_COST_FUNCTION);
        Call make_move(Call::Function::MAKE_MOVE);
    
        CFtype sum = static_cast<CFtype>(0);
        auto c_moves = const_cast<decltype(moves)>(moves);
        MoveTypeRefs r_moves = to_refs(c_moves);
        std::vector<State> temp_states(this->Modality(), State(this->in)); // the chain of states
        temp_states[0] = st; // the first state is a copy of to st
        // create and initialize all the remaining states in the chain
        sum = SuperNeighborhoodExplorer::ComputeAt(temp_states[0], r_moves, this->nhes, do_delta_cost_function, 0);
        for (unsigned int i = 1; i < this->Modality(); i++)
        {
          temp_states[i] = temp_states[i - 1];
          SuperNeighborhoodExplorer::ExecuteAt(temp_states[i], r_moves, this->nhes, make_move, i - 1);
          sum += SuperNeighborhoodExplorer::ComputeAt(temp_states[i], r_moves, this->nhes, do_delta_cost_function, i);
        }
    
        return sum;
      }

      /** @copydoc NeighborhoodExplorer::DeltaViolations */
      virtual CFtype DeltaViolations(const State& st, const typename SuperNeighborhoodExplorer::MoveType& moves) const
      {
#if defined(DEBUG)
        VerifyAllActives(st, moves);
        VerifyAllRelated(st, moves);
#endif

        Call do_delta_violations(Call::Function::DELTA_VIOLATIONS);
        Call make_move(Call::Function::MAKE_MOVE);
    
        CFtype sum = static_cast<CFtype>(0);
        auto c_moves = const_cast<decltype(moves)>(moves);
        MoveTypeRefs r_moves = to_refs(c_moves);
        std::vector<State> temp_states(this->Modality(), State(this->in)); // the chain of states
        temp_states[0] = st; // the first state is a copy of to st
        // create and initialize all the remaining states in the chain
        sum = SuperNeighborhoodExplorer::ComputeAt(temp_states[0], r_moves, this->nhes, do_delta_violations, 0);
        for (unsigned int i = 1; i < this->Modality(); i++)
        {
          temp_states[i] = temp_states[i - 1];
          SuperNeighborhoodExplorer::ExecuteAt(temp_states[i], r_moves, this->nhes, make_move, i - 1);
          sum += SuperNeighborhoodExplorer::ComputeAt(temp_states[i], r_moves, this->nhes, do_delta_violations, i);
        }
    
        return sum;
      }

      /** @copydoc NeighborhoodExplorer::MakeMove */
      virtual void MakeMove(State& st, const typename SuperNeighborhoodExplorer::MoveType& moves) const
      {
#if defined(DEBUG)
        VerifyAllActives(st, moves);
        VerifyAllRelated(st, moves);
#endif
        Call make_move(Call::Function::MAKE_MOVE);
        auto c_moves = const_cast<decltype(moves)>(moves);
        MoveTypeRefs r_moves = to_refs(c_moves);
        SuperNeighborhoodExplorer::ExecuteAll(st, r_moves, this->nhes, make_move);
      }
  
      /** @copydoc NeighborhoodExplorer::FeasibleMove */
      virtual bool FeasibleMove(const State& st, const typename SuperNeighborhoodExplorer::MoveType& moves) const
      {
#if defined(DEBUG)
        VerifyAllActives(st, moves);
#endif
        Call is_feasible_move(Call::Function::FEASIBLE_MOVE);
        Call make_move(Call::Function::MAKE_MOVE);
        auto c_moves = const_cast<decltype(moves)>(moves);
        MoveTypeRefs r_moves = to_refs(c_moves);
        std::vector<State> temp_states(this->Modality(), State(this->in)); // the chain of states
        temp_states[0] = st; // the first state is a copy of to st
        // create and initialize all the remaining states in the chain
        if (!SuperNeighborhoodExplorer::CheckAt(temp_states[0], r_moves, this->nhes, is_feasible_move, 0))
          return false;
        for (unsigned int i = 1; i < this->Modality(); i++)
        {
          temp_states[i] = temp_states[i - 1];
          SuperNeighborhoodExplorer::ExecuteAt(temp_states[i], r_moves, this->nhes, make_move, i - 1);
          if (!SuperNeighborhoodExplorer::CheckAt(temp_states[i], r_moves, this->nhes, is_feasible_move, i))
            return false;
        }
        return true;
      }
    };
  }
}                            

#endif // _MULTIMODALNEIGHBORHOOD_EXPLORER_HH_