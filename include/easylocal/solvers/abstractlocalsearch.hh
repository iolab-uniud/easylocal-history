#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <atomic>

#include "easylocal/solvers/solver.hh"
#include "easylocal/helpers/statemanager.hh"
#include "easylocal/helpers/outputmanager.hh"
#include "easylocal/runners/runner.hh"
#include "easylocal/utils/parameter.hh"
#include "easylocal/utils/interruptible.hh"


namespace EasyLocal {
  
  namespace Core {
    
    /** A Local Search Solver has an internal state, and defines the ways for
     dealing with a local search algorithm.
     @ingroup Solvers
     */
    template <class Input, class Output, class State, class CostStructure = DefaultCostStructure<int>>
    class AbstractLocalSearch
    : public Parametrized, public Solver<Input, Output, CostStructure>, public Interruptible<int>
    {
    public:
      /** These methods are the unique interface of Solvers */
      virtual SolverResult<Input, Output, CostStructure> Solve() throw (ParameterNotSet, IncorrectParameterValue) final;
      virtual SolverResult<Input, Output, CostStructure> Resolve(const Output& initial_solution) throw (ParameterNotSet, IncorrectParameterValue) final;
      
      AbstractLocalSearch(const Input& in,
                          StateManager<Input, State, CostStructure>& e_sm,
                          OutputManager<Input, Output, State>& e_om,
                          std::string name, std::shared_ptr<spdlog::logger> logger = nullptr);
      virtual std::shared_ptr<Output> GetCurrentSolution() const;
      
    protected:
      
      virtual std::shared_ptr<State> GetCurrentState() const = 0;
      
      virtual ~AbstractLocalSearch()
      {}
      
      /** Implements Interruptible. */
      virtual std::function<int(void)> MakeFunction()
      {
        return [this](void) -> int {
          this->ResetTimeout();
          this->Go();
          return 1;
        };
      }
      
      virtual void FindInitialState();
      // This will be the actual solver strategy implementation
      virtual void Go() = 0;
      StateManager<Input, State, CostStructure>& sm; /**< A pointer to the attached
                                               state manager. */
      OutputManager<Input, Output, State>& om; /**< A pointer to the attached
                                                        output manager. */
      std::shared_ptr<State> p_current_state, p_best_state;        /**< The internal states of the solver. */
      
      CostStructure current_state_cost, best_state_cost;  /**< The cost of the internal states. */
      std::shared_ptr<Output> p_out; /**< The output object of the solver. */
      // parameters
      
      void InitializeParameters();
      
      Parameter<unsigned int> init_trials;
      Parameter<bool> random_initial_state;
      Parameter<double> timeout;
      std::atomic<bool> is_running;
      
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
    template <class Input, class Output, class State, class CostStructure>
    AbstractLocalSearch<Input, Output, State, CostStructure>::AbstractLocalSearch(const Input& in,
                                                                                  StateManager<Input, State, CostStructure>& e_sm,
                                                                                  OutputManager<Input, Output, State>& e_om,
                                                                                  std::string name,
                                                                                  std::shared_ptr<spdlog::logger> logger)
    : Parametrized(name, typeid(this).name()),
    Solver<Input, Output, CostStructure>(in, name, logger),
    sm(e_sm),
    om(e_om),
    is_running(false)
    {}
    
    
    template <class Input, class Output, class State, class CostStructure>
    void AbstractLocalSearch<Input, Output, State, CostStructure>::InitializeParameters()
    {
      init_trials("init_trials", "Number of states to be tried in the initialization phase", this->parameters);
      random_initial_state("random_state", "Random initial state", this->parameters);
      timeout("timeout", "Solver timeout (if not specified, no timeout)", this->parameters);
      init_trials = 1;
      random_initial_state = true;
    }
    
    /**
     The initial state is generated by delegating this task to
     the state manager. The function invokes the SampleState function.
     */
    template <class Input, class Output, class State, class CostStructure>
    void AbstractLocalSearch<Input, Output, State, CostStructure>::FindInitialState()
    {
      if (random_initial_state)
        current_state_cost = sm.SampleState(*p_current_state, init_trials);
      else
      {
        sm.GreedyState(*p_current_state);
        current_state_cost = sm.CostFunctionComponents(*p_current_state);
      }
      *p_best_state = *p_current_state;
      best_state_cost = current_state_cost;
    }
    
    template <class Input, class Output, class State, class CostStructure>
    void AbstractLocalSearch<Input, Output, State, CostStructure>::InitializeSolve() throw (ParameterNotSet, IncorrectParameterValue)
    {
      p_best_state = std::make_shared<State>(this->in);
      p_current_state = std::make_shared<State>(this->in);
    }
    
    template <class Input, class Output, class State, class CostStructure>
    SolverResult<Input, Output, CostStructure> AbstractLocalSearch<Input, Output, State, CostStructure>::Solve() throw (ParameterNotSet, IncorrectParameterValue)
    {
      auto start = std::chrono::high_resolution_clock::now();
      is_running = true;
      InitializeSolve();
      FindInitialState();
      if (timeout.IsSet())
        SyncRun(std::chrono::milliseconds(static_cast<long long int>(timeout * 1000.0)));
      else
        Go();
      p_out = std::make_shared<Output>(this->in);
      om.OutputState(*p_best_state, *p_out);
      TerminateSolve();
        
      double run_time = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(std::chrono::high_resolution_clock::now() - start).count();
      is_running = false;
        
      return SolverResult<Input, Output, CostStructure>(*p_out, sm.CostFunctionComponents(*p_best_state), run_time);
    }
    
    template <class Input, class Output, class State, class CostStructure>
    SolverResult<Input, Output, CostStructure> AbstractLocalSearch<Input, Output, State, CostStructure>::Resolve(const Output& initial_solution) throw (ParameterNotSet, IncorrectParameterValue)
    {
      auto start = std::chrono::high_resolution_clock::now();
      is_running = true;
      
      InitializeSolve();
      om.InputState(*p_current_state, initial_solution);
      *p_best_state = *p_current_state;
      best_state_cost = current_state_cost = sm.CostFunctionComponents(*p_current_state);
      if (timeout.IsSet())
        SyncRun(std::chrono::milliseconds(static_cast<long long int>(timeout * 1000.0)));
      else
        Go();
      p_out = std::make_shared<Output>(this->in);
      om.OutputState(*p_best_state, *p_out);
      TerminateSolve();
      is_running = false;
          
      double run_time = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(std::chrono::high_resolution_clock::now() - start).count();
          
      return SolverResult<Input, Output, CostStructure>(*p_out, sm.CostFunctionComponents(*p_best_state), run_time);
    }
    
    template <class Input, class Output, class State, class CostStructure>
    void AbstractLocalSearch<Input, Output, State, CostStructure>::TerminateSolve()
    {}
    
    template <class Input, class Output, class State, class CostStructure>
    std::shared_ptr<Output> AbstractLocalSearch<Input, Output, State, CostStructure>::GetCurrentSolution() const
    {
      std::shared_ptr<State> current_state;
      if (!is_running)
        current_state = this->p_best_state;
      else
        current_state = GetCurrentState();
      std::shared_ptr<Output> out = std::make_shared<Output>(this->in);
      om.OutputState(*current_state, *out);
      return out;
    }
  }
}

