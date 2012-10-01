#if !defined(_ABSTRACT_LOCAL_SEARCH_HH_)
#define _ABSTRACT_LOCAL_SEARCH_HH_

#include <solvers/Solver.hh>
#include <helpers/StateManager.hh>
#include <helpers/OutputManager.hh>
#include <runners/Runner.hh>
#include <iostream> 
#include <fstream> 
#include <string>
#include <utils/Parameter.hh>
#include <utils/Interruptible.hh>

/** A Local Search Solver has an internal state, and defines the ways for
    dealing with a local search algorithm.
    @ingroup Solvers
*/
template <class Input, class Output, class State, typename CFtype = int>
class AbstractLocalSearch
  : public Solver<Input, Output>, public Interruptible<int>
{
public:
  /** These methods are the unique interface of Solvers */
  virtual Output Solve() final;
  virtual Output Resolve(const Output& initial_solution) final;
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
  ParameterBox parameters;
  Parameter<unsigned int> init_trials;
  Parameter<bool> random_initial_state;
  Parameter<double> timeout;
  // TODO: set those values
  std::chrono::milliseconds accumulated_time;
private:
  virtual void InitializeSolve();
  virtual const Output& TerminateSolve();
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
  : Solver<Input, Output>(in, name), sm(e_sm),  om(e_om), 
parameters(name, description), init_trials("init_trials", "Number of states to be tried in the initialization phase (default = 1)", parameters), random_initial_state("random_state", "Random initial state (default = true)", parameters), timeout("timeout", "Solver timeout (if not specified, no timeout)", parameters)
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
void AbstractLocalSearch<Input,Output,State,CFtype>::InitializeSolve()
{
  p_best_state = std::make_shared<State>(this->in);
  p_current_state = std::make_shared<State>(this->in);
  p_out = std::make_shared<Output>(this->in);
}

template <class Input, class Output, class State, typename CFtype>
Output AbstractLocalSearch<Input,Output,State,CFtype>::Solve()
{
  InitializeSolve();
  FindInitialState();
  if (timeout.IsSet())
    SyncRun(std::chrono::milliseconds(static_cast<long long int>(timeout * 1000.0)));
  else
    Go();
  return TerminateSolve();
}

template <class Input, class Output, class State, typename CFtype>
Output AbstractLocalSearch<Input,Output,State,CFtype>::Resolve(const Output& initial_solution)
{
  InitializeSolve();
  om.InputState(*p_current_state, initial_solution);
  *p_best_state = *p_current_state;
  best_state_cost = current_state_cost = sm.CostFunction(*p_current_state);
  if (timeout.IsSet())
    SyncRun(std::chrono::milliseconds(static_cast<long long int>(timeout * 1000.0)));
  else
    Go();
  return TerminateSolve();
}

template <class Input, class Output, class State, typename CFtype>
const Output& AbstractLocalSearch<Input,Output,State,CFtype>::TerminateSolve()
{
  om.OutputState(*p_best_state, *p_out);
  p_best_state.reset();
  p_current_state.reset();
  return *p_out;
}


#endif // _ABSTRACT_LOCAL_SEARCH_HH_
