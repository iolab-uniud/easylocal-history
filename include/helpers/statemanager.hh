#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <stdexcept>

#include "helpers/costcomponent.hh"
#include "helpers/coststructure.hh"
#include "utils/third-party/json.hpp"

namespace EasyLocal
{
  namespace Core
  {
    /**
     This constant multiplies the value of the Violations function in thehierarchical formulation of the Cost function (i.e., CostFunction(s) = HARD_WEIGHT * Violations(s) + Objective(s)).
     @todo The use of the global HARD_WEIGHT is a rough solution, waiting for an idea of a general mechanism for managing cost function weights.
     */    
#if !defined(HARD_WEIGHT_SET)
    const int HARD_WEIGHT = 1000;
#define HARD_WEIGHT_SET
#endif
    /**
     * @brief This component is responsible for all operations on the state which are
     independent of the neighborhood definition, such as generating a random state
     or computing the cost of a state.
     * 
     * @tparam _Input the class representing the problem input
     * @tparam _State the class representing the problem solution
     * @tparam DefaultCostStructure<int> the type of the cost structure to be used (by default an aggregated cost structure with distinct violations and objectives)
     * 
     * @ingroup Helpers
     */
    template <class _Input, class _State, class CostStructure = DefaultCostStructure<int>>
    class StateManager
    {
    public:
      typedef typename CostStructure::CFtype CFtype;
      typedef _Input Input;
      typedef _State State;      
      /**
       Generates a random state
       @param in the input object
       @return a new random state
       */
      virtual void RandomState(const Input& in, State& st) = 0;
            
      /**
       Looks for the best state out of a given sample of random states.
       @param st state to be written
       @param samples number of states sampled
       @return a cost structure with the cost of the state @c st
       */
      virtual CostStructure SampleState(const Input& in, State &st, unsigned int samples);

      
      /**
       Generate a greedy state with a random component controlled by the
       parameters alpha and k.
       
       @note During the construction phase of GRASP one chooses, from an adaptive
       RCL (Restricted Candidate List), a solution element (e.g. be a boolean
       variable in the case of SAT) to set in the initial solution. The RCL contains
       the best components to set based on a greedy policy. In order to increase
       the number of optimal initial solutions and to improve the overall GRASP
       procedure, the RCL should contain more than the greedy component; @c alpha
       and @c k are used to control how many other components are included in
       the RCL.
       In particular, if the greedy component has value @c p, by setting @c alpha
       we can include in the RCL also components which has a value greater than
       @c alpha * @c p.
       Pretty much in the same way, @c k can be used to restrict the RCL to the
       best @c k components according to the greedy policy. In principle @c alpha
       and @c k should be used alternatively.
       
       @param in the input object
       @param st the state to be written
       @param alpha percentage of the value of the greedy component above which
       non-greedy component can be added to the GRASP's RCL
       @param k length of the GRASP's RCL
       
       @remarks this method is somehow specific for GRASP. The meaning of @c alpha
       and @k makes sense only when related to this approach.
       
       */
      virtual void GreedyState(const Input& in, State &st, double alpha, unsigned int k);
      
      /**
       Generate a greedy state.
       @note To be implemented in the application. Default behaviour is RandomState
       (MayRedef).
       */
      virtual void GreedyState(const Input& in, State &st);
      
      
      /**
       Compute the cost function calling the cost components.
       @param in the input object
       @param st the state to be evaluated
       @param an optional vector of weights for the cost components.
       @return all the components of the cost function in the given state
       
       @remarks The normal definition computes a weighted sum of the violation
       function and the objective function.
       
       @note It is rarely needed to redefine this method.
       */
      virtual CostStructure CostFunctionComponents(const Input& in, const State &st, const std::vector<double> &weights = std::vector<double>(0)) const;
      
      /** Returns a json object with all cost components.
       @param in the input object
       @param st the state to be evaluated
       @param an optional vector of weights for the cost components.
       @return all the components of the cost function in the given state embedded in a json object
       */      
      json JSONCostFunctionComponents(const Input& in, const State &st, const std::vector<double> &weights = std::vector<double>(0)) const;

      
      /**
       Check whether the lower bound of the cost function components has been reached. The
       tentative definition verifies whether the state costs are equal to zero.
       @return true if the lower bound of the cost function has been reached
       */
      virtual bool LowerBoundReached(const Input& in, const CostStructure &costs) const;
      
      /**
       Check whether the cost of the current state has reached the lower bound.
       By default calls @c LowerBoundReached(CostFunctionComponents(st)).
       @return true if the state is optimal w.r.t. the lower bound cost function
       has been reached
       @param in the input object
       @param st the state to be evaluated
       */
      virtual bool OptimalStateReached(const Input& in, const State &st) const;
      
      /**
       Add a component to the cost component array.
       @param cc the cost component to be added
       */
      void AddCostComponent(CostComponent<Input, State, CFtype> &cc);
      
      size_t CostComponents() const
      {
        return cost_component.size();
      }
      
      const CostComponent<Input, State, CFtype> &GetCostComponent(size_t i) const
      {
        return *cost_component[i];
      }
      
      size_t CostComponentIndex(const CostComponent<Input, State, CFtype> &cc) const
      {
        return cost_component_index.at(cc.hash);
      }
      
      /**
       Clear the cost component array.
       */
      void ClearCostStructure();

      
      /**
       Compute the distance of two states (e.g. the Hamming distance).
       Currently used only by the @ref GeneralizedLocalSearchObserver.
       @return the distance, assumed always @c unsigned int
       */
      unsigned int StateDistance(const Input& in, const State &st1, const State &st2) const;
    
      
      /**
       Check whether the state is consistent. In particular, should check whether
       the redundant data structures are consistent with the main ones. Used only
       for debugging purposes.
       */
      virtual bool CheckConsistency(const Input& in, const State &st) const = 0;
      
      /** Name of the state manager */
      const std::string name;
      
      virtual void DisplayDetailedState(const Input& in, const State& st, std::ostream& os) const
      {
        os << st;
      }
      
      virtual json ToJSON(const Input& in, const State& st) const = 0;
      
      virtual void FromJSON(const Input& in, State& st, const json& output) const = 0;
                  
    protected:
      
      /** Build a StateManager object.
       @param name the name of the object
       */
      
      StateManager(std::string name);
      
      /** Destructor. */
      virtual ~StateManager() {}
      
      /**
       The set of the cost components. Hard and soft ones are all in this @c vector.
       */
      // TODO: transform into a shared_ptr
      std::vector<CostComponent<Input, State, CFtype> *> cost_component;
      /**
       The reverse map from cost component to its index.
       */
      std::map<size_t, size_t> cost_component_index;
    };
    
    /* **************************************************************************
     * Implementation
     * **************************************************************************/
    
    
    template <class Input, class State, class CostStructure>
    StateManager<Input, State, CostStructure>::StateManager(std::string name)
    : name(name)
    {}
    
    template <class Input, class State, class CostStructure>
    CostStructure StateManager<Input, State, CostStructure>::SampleState(const Input& in,
                                                                         State &st,
                                                                         unsigned int samples)
    {
      unsigned int s = 1;
      RandomState(in, st);
      CostStructure cost = CostFunctionComponents(in, st);
      State best_state(in);
      best_state = st;
      CostStructure best_cost = cost;
      while (s < samples)
      {
        RandomState(in, st);
        cost = CostFunctionComponents(in, st);
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
    
    template <class Input, class State, class CostStructure>
    void StateManager<Input, State, CostStructure>::GreedyState(const Input& in, State &st, double alpha,
                                                                unsigned int k)
    {
      GreedyState(in, st);
    }
    
    template <class Input, class State, class CostStructure>
    void StateManager<Input, State, CostStructure>::GreedyState(const Input& in, State &st)
    {
      throw std::runtime_error("For using this feature GreedyState must be implemented in the concrete class!");
    }
    
    template <class Input, class State, class CostStructure>
    CostStructure StateManager<Input, State, CostStructure>::CostFunctionComponents(const Input& in, const State &st, const std::vector<double> &weights) const
    {
      CFtype hard_cost = 0, soft_cost = 0;
      double weighted_cost = 0.0;
      std::vector<CFtype> cost_function(CostComponents(), (CFtype)0);
      for (size_t i = 0; i < cost_component.size(); i++)
      {
        CFtype current_cost = cost_function[i] = cost_component[i]->Cost(in, st);
        if (cost_component[i]->IsHard())
        {
          hard_cost += current_cost;
          if (!weights.empty())
            weighted_cost += HARD_WEIGHT * weights[i] * current_cost;
        }
        else
        {
          soft_cost += current_cost;
          if (!weights.empty())
            weighted_cost += weights[i] * current_cost;
        }
      }
      
      if (!weights.empty())
        return CostStructure(HARD_WEIGHT * hard_cost + soft_cost, weighted_cost, hard_cost, soft_cost, cost_function);
      else
        return CostStructure(HARD_WEIGHT * hard_cost + soft_cost, hard_cost, soft_cost, cost_function);
    }
    
    template <class Input, class State, class CostStructure>
    json StateManager<Input, State, CostStructure>::JSONCostFunctionComponents(const Input& in, const State &st, const std::vector<double> &weights) const
    {
      json res;
      res["components"] = {};
      CostStructure cost = CostFunctionComponents(in, st, weights);
      for (size_t i = 0; i < cost_component.size(); i++)
      {
        res["components"][cost_component[i]->name]["cost"] = cost[i];
        res["components"][cost_component[i]->name]["hard"] = cost_component[i]->IsHard();
        res["components"][cost_component[i]->name]["weight"] = cost_component[i]->Weight();
      }
      res["total"] = cost.total;
      res["violations"] = cost.violations;
      res["objective"] = cost.objective;
      return res;
    }
    
    template <class Input, class State, class CostStructure>
    bool StateManager<Input, State, CostStructure>::LowerBoundReached(const Input& in, const CostStructure &costs) const
    {
      return costs == 0;
    }
    
    template <class Input, class State, class CostStructure>
    bool StateManager<Input, State, CostStructure>::OptimalStateReached(const Input& in, const State &st) const
    {
      return LowerBoundReached(in, CostFunctionComponents(in, st));
    }
    
    template <class Input, class State, class CostStructure>
    void StateManager<Input, State, CostStructure>::AddCostComponent(CostComponent<Input, State, CFtype> &cc)
    {
      size_t index = cost_component.size();
      cost_component.push_back(&cc);
      cost_component_index[cc.hash] = index;
    }
    
    template <class Input, class State, class CostStructure>
    unsigned int StateManager<Input, State, CostStructure>::StateDistance(const Input& in,
                                                                          const State &st1,
                                                                          const State &st2) const
    {
      throw std::runtime_error("In order to use this feature StateDistance must be implemented in the concrete class!");
      return 0;
    }
  } // namespace Core
} // namespace EasyLocal