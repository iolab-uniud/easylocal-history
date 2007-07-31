#ifndef STATEMANAGER_HH_
#define STATEMANAGER_HH_

#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include "CostComponent.hh"
#include "../basics/EasyLocalException.hh"
#include "../utils/Types.hh"

/** This constant multiplies the value of the Violations function in the
hierarchical formulation of the Cost function (i.e., 
                                               CostFunction(s) = HARD_WEIGHT * Violations(s) + Objective(s)).
@todo The use of the global HARD_WEIGHT is a rough solution, 
waiting for an idea of a general mechanism for managing cost function 
weights.
*/
#ifndef HARD_WEIGHT_SET
const int HARD_WEIGHT = 1000;
#endif

/** The State Manager is responsible for all operations on the state
which are independent of the neighborhood definition, such as
generating a random state, and computing the cost of a state.
No @c Move template is supplied to it.  
@ingroup Helpers
*/
template <class Input, class State, typename CFtype = int>
class StateManager
: virtual public EasyLocalObject
{
public:
  void Print(ostream& os = cout) const;
  /** Generates a random state.
  @note @bf To be implemented in the application.
  @param st the state generated */
  virtual void RandomState(State &st) = 0;
  virtual CFtype SampleState(State &st, unsigned int samples);
  // State Evaluation functions
  virtual CFtype CostFunction(const State& st) const;
  virtual CFtype Violations(const State& st) const;
  virtual CFtype Objective(const State& st) const;
  // Partial State Evaluation functions (for each cost component)
  /* virtual CFtype Objective(const State& st, unsigned int i) const;
  virtual CFtype Violations(const State& st, unsigned int i) const; */
		/** Checks whether the lower bound of the cost function has been
    reached.
    @return true if the lower bound of the cost function has reached,
    false otherwise */
  virtual bool LowerBoundReached(const CFtype& fvalue) const;
		/** Checks whether the lower bound of the cost function has been
    reached.
    @return true if the lower bound of the cost function has reached,
    false otherwise */
  virtual bool OptimalStateReached(const State& st) const;
  virtual unsigned StateDistance(const State& st1, const State& st2) const = 0;
  virtual void PrintStateDistance(const State& st1, const State& st2, ostream& os = cout) const = 0;

  void AddCostComponent(CostComponent<Input,State,CFtype>& cc);
  
  // debug functions
  virtual void PrintState(const State& st,
                          ostream& os = cout) const;
  virtual void PrintStateCost(const State& st,
                              ostream& os = cout) const;
  virtual void PrintStateDetailedCost(const State& st,
                                      ostream& os = cout) const;
  // info functions
  virtual void GetDetailedCost(const State& st, CFtype& violations,
                               vector<CFtype>& single_violations_cost,
                               CFtype& objective,
                               vector<CFtype>& single_objective_cost) const;
  
  virtual void Check() const throw(EasyLocalException);
  
  CostComponent<Input, State,CFtype>& GetCostComponent(unsigned i) const { return cost_component[i]; }
  unsigned CostComponents() const { return cost_component.size(); }

protected:
  StateManager(const Input& in);
  vector<CostComponent<Input,State,CFtype>* > cost_component;
  const Input& in;
};

/*************************************************************************
* Implementation
*************************************************************************/

/**
Builds a state manager object linked to the provided input.
 
 @param in a reference to an input object
 */
template <class Input, class State, typename CFtype>
StateManager<Input,State,CFtype>::StateManager(const Input& i)
: in(i)
{}

template <class Input, class State, typename CFtype>
void StateManager<Input,State,CFtype>::Print(std::ostream& os) const
{
  os  << "State Manager: " + GetName() << std::endl;
  os  << "Violations:" << std::endl;
  for (unsigned int i = 0; i < cost_component.size(); i++)
    if (cost_component[i]->IsHard())
      cost_component[i]->Print(os);
  os  << "Objective:" << std::endl;
  for (unsigned int i = 0; i < cost_component.size(); i++)
    if (cost_component[i]->IsSoft())
      cost_component[i]->Print(os);
}

/**
Looks for the best state out of a given number of sampled
 states.
 
 @param st the best state found
 @param samples the number of sampled states
 */
template <class Input, class State, typename CFtype>
CFtype StateManager<Input,State,CFtype>::SampleState(State &st,
                                                     unsigned int samples)
{
  unsigned int s = 1;
  RandomState(st);
  CFtype cost = CostFunction(st);
  State best_state = st;
  CFtype best_cost = cost;
  while (s < samples)
  {
    RandomState(st);
    cost = CostFunction(st);
    if (cost < best_cost)
    {
      best_state = st;
      best_cost = cost;
    }
    s++;
  }
  st = best_state;
  return best_cost;
}

/**
Evaluates the cost function value in a given state.  
 The tentative definition computes a weighted sum of the violation 
 function and the objective function.
 
 @param st the state to be evaluated
 @return the value of the cost function in the given state */
template <class Input, class State, typename CFtype>
CFtype StateManager<Input,State,CFtype>::CostFunction(const State& st) const
{ 
  CFtype hard_cost = 0, soft_cost = 0;
  for (unsigned int i = 0; i < cost_component.size(); i++)
    if (cost_component[i]->IsHard())
      hard_cost += cost_component[i]->Cost(st);
    else
      soft_cost += cost_component[i]->Cost(st);

  return HARD_WEIGHT * hard_cost  + soft_cost; 
}

/**
Checks whether the lower bound of the cost function has been reached.
 The tentative definition verifies whether the state cost is
 equal to zero.
 */
template <class Input, class State, typename CFtype>
bool StateManager<Input,State,CFtype>::LowerBoundReached(const CFtype& fvalue) const
{ return IsZero(fvalue); }

/**
Checks whether the lower bound of the cost function has been reached.
 The tentative definition verifies whether the current state cost is
 equal to zero.
 */
template <class Input, class State, typename CFtype>
bool StateManager<Input,State,CFtype>::OptimalStateReached(const State& st) const
{ return LowerBoundReached(CostFunction(st)); }

/**
Outputs some informations about the state passed as parameter.
 
 @param st the state to be inspected
 */
template <class Input, class State, typename CFtype>
void StateManager<Input,State,CFtype>::PrintState(const State& st,
                                                  std::ostream& os) const
{
  os  << st << std::endl;
  os  << "Violations:\t" << Violations(st) << std::endl
    << "Objective: \t" << Objective(st) << std::endl;
  os  << "Total cost:  \t" << CostFunction(st) << std::endl;
}

/**
Outputs the state cost components of the state passed as parameter.
 
 @param st the state to be inspected
 */
template <class Input, class State, typename CFtype>
void StateManager<Input,State,CFtype>::PrintStateCost(const State& st,
                                                      std::ostream& os) const
{
  CFtype violations = Violations(st), objective = Objective(st), 
  cost_function = CostFunction(st);
  for (unsigned int i = 0; i < cost_component.size(); i++)
    os  << i << ". " << cost_component[i]->GetName() << " : " 
      << cost_component[i]->Cost(st) << (cost_component[i]->IsHard() ? '*' : ' ') << std::endl;
  os  << "Total Violations:\t" << violations << std::endl;
  os  << "Total Objective:\t" << objective << std::endl;
  os  << "Total Cost:  \t" << cost_function << std::endl;
}

/**
Outputs the state cost components of the state passed as parameter.
 
 @param st the state to be inspected
 */
template <class Input, class State, typename CFtype>
void StateManager<Input,State,CFtype>::PrintStateDetailedCost(const State& st,
                                                              std::ostream& os) const
{
  for (unsigned int i = 0; i < cost_component.size(); i++)
      cost_component[i]->PrintCost(st, os);
  PrintStateCost(st, os);
}

template <class Input, class State, typename CFtype>
void StateManager<Input,State,CFtype>::GetDetailedCost(const State& st,
                                                       CFtype& violations,
                                                       std::vector<CFtype>& single_violations_cost,
                                                       CFtype& objective,
                                                       std::vector<CFtype>& single_objective_cost) const
{
  violations = Violations(st);
  single_violations_cost.resize(0);
  for (unsigned int i = 0; i < cost_component.size(); i++)
    if (cost_component[i]->IsHard())
      single_violations_cost.push_back(cost_component[i]->Cost(st));
  objective = Objective(st);
  single_objective_cost.resize(0);
  for (unsigned int i = 0; i < cost_component.size(); i++)
    if (cost_component[i]->IsSoft())
      single_objective_cost.push_back(cost_component[i]->Cost(st));
}

/**
A component-based definition of the violation function: 
 it returns the sum of the components. If components are 
 not used, this definition must be overwritten
 
 @param st the state to be evaluated
 @return the value of the violations function in the given state
 */
template <class Input, class State, typename CFtype>
CFtype StateManager<Input,State,CFtype>::Violations(const State& st) const
{
  CFtype cost = 0;
  for (unsigned int i = 0; i < cost_component.size(); i++)
    if (cost_component[i]->IsHard())
      cost += cost_component[i]->Cost(st);
  return cost;
}

/**
A component-based definition of the objective function: 
 it returns the sum of the components. If components are 
 not used, this definition must be overwritten
 
 @param st the state to be evaluated
 @return the value of the violations function in the given state
 */
template <class Input, class State, typename CFtype>
CFtype StateManager<Input,State,CFtype>::Objective(const State& st) const
{
  CFtype cost = 0;
  for (unsigned int i = 0; i < cost_component.size(); i++)
    if (cost_component[i]->IsSoft())
      cost += cost_component[i]->Cost(st);

  return cost;
}


/* template <class Input, class State, typename CFtype>
CFtype StateManager<Input,State,CFtype>::Violations(const State& st, unsigned int i) const
{
  if (hard_costs.size() > i);
  return hard_costs[i]->Cost(st);
}

template <class Input, class State, typename CFtype>
CFtype StateManager<Input,State,CFtype>::Objective(const State& st, unsigned int i) const
{
  assert(soft_costs.size() > i);
  return soft_costs[i]->Cost(st);
} */


/**
Adds a component to the cost component array
 */
template <class Input, class State, typename CFtype>
void StateManager<Input,State,CFtype>::AddCostComponent(CostComponent<Input,State,CFtype>& cc)
{
  cost_component.push_back(&cc);
}

/**
Checks wether the object state is consistent with all the related
 objects.
 */
template <class Input, class State, typename CFtype>
void StateManager<Input,State,CFtype>::Check() const
throw(EasyLocalException)
{}

#endif /*STATEMANAGER_HH_*/
