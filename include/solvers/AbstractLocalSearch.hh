#if !defined(_ABSTRACT_LOCAL_SEARCH_HH_)
#define _ABSTRACT_LOCAL_SEARCH_HH_

#include <iostream>
#include <fstream>
#include <string>

#include "solvers/Solver.hh"
#include "helpers/StateManager.hh"
#include "helpers/OutputManager.hh"
#include "runners/Runner.hh"
#include "utils/Parameter.hh"
#include "utils/Interruptible.hh"

namespace EasyLocal {

  namespace Core {
    
    /** A Local Search Solver has an internal state, and defines the ways for
    dealing with a local search algorithm.
    @ingroup Solvers
    */
    template <class Input, class Output, class State, typename CFtype>
    class AbstractLocalSearch
      : public Parametrized, public Solver<Input, Output, CFtype>, public Interruptible<int>
    {
    public:
  
      typedef typename Solver<Input, Output, CFtype>::SolverResult SolverResult;
  
      /** These methods are the unique interface of Solvers */
      virtual SolverResult Solve() throw (ParameterNotSet, IncorrectParameterValue) final;
      virtual SolverResult Resolve(const Output& initial_solution) throw (ParameterNotSet, IncorrectParameterValue) final;
    protected:
      AbstractLocalSearch(const Input& in,
      StateManager<Input,State,CFtype>& e_sm,
      OutputManager<Input,Output,State,CFtype>& e_om,
      std::string name, std::string description);
  
      /** Implements Interruptible. */
      virtual std::function<int(void)> MakeFunction()
      {
        return [this](void) -> int {
          this->Go();
          return 1;
        };
      }
  
      virtual void FindInitialState();
      // This will be the actual solver strategy implementation
      virtual void Go() = 0;
      StateManager<Input,State,CFtype>& sm; /**< A pointer to the attached
        state manager. */
        OutputManager<Input,Output,State,CFtype>& om; /**< A pointer to the attached
          output manager. */
          std::shared_ptr<State> p_current_state, p_best_state;        /**< The internal states of the solver. */
  
      CFtype current_state_cost, best_state_cost;  /**< The cost of the internal states. */
      std::shared_ptr<Output> p_out; /**< The output object of the solver. */
      // parameters
      Parameter<unsigned int> init_trials;
      Parameter<bool> random_initial_state;
      Parameter<double> timeout;
  
    private:
      void InitializeSolve() throw (ParameterNotSet, IncorrectParameterValue);
      void TerminateSolve();
    };

    /*************************************************************************
    * Implementation
    *************************************************************************/

    /**
    @brief Constructs an abstract local search solver.
 
    @param in an input object
    @param e_sm a compatible state manager
    @param e_om a compatible output manager
    @param name a descriptive name for the solver
    */
    template <class Input, class Output, class State, typename CFtype>
    AbstractLocalSearch<Input,Output,State,CFtype>::AbstractLocalSearch(const Input& in,
    StateManager<Input,State,CFtype>& e_sm,
    OutputManager<Input,Output,State,CFtype>& e_om,
    std::string name,
    std::string description)
      : Parametrized(name, description),
    Solver<Input, Output, CFtype>(in, name),
    sm(e_sm),
    om(e_om),
    // Parameters
    init_trials("init_trials", "Number of states to be tried in the initialization phase", this->parameters),
    random_initial_state("random_state", "Random initial state", this->parameters),
    timeout("timeout", "Solver timeout (if not specified, no timeout)", this->parameters)
    {
      init_trials = 1;
      random_initial_state = true;
    }

    /**
    The initial state is generated by delegating this task to
    the state manager. The function invokes the SampleState function.
    */
    template <class Input, class Output, class State, typename CFtype>
    void AbstractLocalSearch<Input,Output,State,CFtype>::FindInitialState()
    {
      if (random_initial_state)
        current_state_cost = sm.SampleState(*p_current_state, init_trials);
      else
      {
        sm.GreedyState(*p_current_state);
        current_state_cost = sm.CostFunction(*p_current_state);
      }
      *p_best_state = *p_current_state;
      best_state_cost = current_state_cost;
    }

    template <class Input, class Output, class State, typename CFtype>
    void AbstractLocalSearch<Input,Output,State,CFtype>::InitializeSolve() throw (ParameterNotSet, IncorrectParameterValue)
    {
      p_best_state = std::make_shared<State>(this->in);
      p_current_state = std::make_shared<State>(this->in);
    }

    template <class Input, class Output, class State, typename CFtype>
    typename AbstractLocalSearch<Input,Output,State,CFtype>::SolverResult AbstractLocalSearch<Input,Output,State,CFtype>::Solve() throw (ParameterNotSet, IncorrectParameterValue)
    {
      auto start = std::chrono::high_resolution_clock::now();
      InitializeSolve();
      FindInitialState();
      if (timeout.IsSet())
      {
        SyncRun(std::chrono::milliseconds(static_cast<long long int>(timeout * 1000.0)));
      }
      else
        Go();
      p_out = std::make_shared<Output>(this->in);
      om.OutputState(*p_best_state, *p_out);
      TerminateSolve();
  
      double run_time = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(std::chrono::high_resolution_clock::now() - start).count();
  
      return std::make_tuple(*p_out, sm.Violations(*p_best_state), sm.Objective(*p_best_state), run_time);
    }

    template <class Input, class Output, class State, typename CFtype>
    typename AbstractLocalSearch<Input,Output,State,CFtype>::SolverResult AbstractLocalSearch<Input,Output,State,CFtype>::Resolve(const Output& initial_solution) throw (ParameterNotSet, IncorrectParameterValue)
    {
      auto start = std::chrono::high_resolution_clock::now();
  
      InitializeSolve();
      om.InputState(*p_current_state, initial_solution);
      *p_best_state = *p_current_state;
      best_state_cost = current_state_cost = sm.CostFunction(*p_current_state);
      if (timeout.IsSet())
        SyncRun(std::chrono::milliseconds(static_cast<long long int>(timeout * 1000.0)));
      else
        Go();
      p_out = std::make_shared<Output>(this->in);
      om.OutputState(*p_best_state, *p_out);
      TerminateSolve();
  
      double run_time = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(std::chrono::high_resolution_clock::now() - start).count();
  
      return std::make_tuple(*p_out, sm.Violations(*p_best_state), sm.Objective(*p_best_state), run_time);
    }

    template <class Input, class Output, class State, typename CFtype>
    void AbstractLocalSearch<Input,Output,State,CFtype>::TerminateSolve()
      { }
  }
}

#endif // _ABSTRACT_LOCAL_SEARCH_HH_
