#if !defined(_RUNNER_OBSERVER_HH_)
#define _RUNNER_OBSERVER_HH_

#include <chrono>

namespace EasyLocal {
  
  namespace Core {
    template <class Input, class State, class Move, typename CFtype>
    class MoveRunner;
  }
  
  using namespace Core;
  
  namespace Debug {
    
    typedef std::chrono::duration<double, std::ratio<1>> secs;
    
    
    template <class Input, class State, class Move, typename CFtype = int>
    class RunnerObserver
    {
    public:
      RunnerObserver(unsigned int verbosity_level, unsigned int plot_level, std::ostream& log_os = std::cout, std::ostream& plot_os = std::cout);
      virtual ~RunnerObserver() {}
      virtual void NotifyStartRunner(MoveRunner<Input, State, Move, CFtype>& r);
      virtual void NotifyNewBest(MoveRunner<Input, State, Move, CFtype>& r);
      virtual void NotifyMadeMove(MoveRunner<Input, State, Move, CFtype>& r);
      virtual void NotifyEndRunner(MoveRunner<Input, State, Move, CFtype>& r);
    protected:
      bool notify_new_best, notify_made_move, plot_improving_moves, plot_all_moves, notify_violations_increased;
      CFtype previous_violations, previous_cost;
      std::ostream &log, &plot;
    };
    
    template <class Input, class State, class Move, typename CFtype>
    RunnerObserver<Input, State, Move, CFtype>::RunnerObserver(unsigned int verbosity_level, unsigned int plot_level, std::ostream& log_os, std::ostream& plot_os)
    : log(log_os), plot(plot_os)
    {
      // notify
      if (verbosity_level >= 1)
        notify_new_best = true;
      else
        notify_new_best = false;
      if (verbosity_level >= 2)
        notify_violations_increased = true;
      else
        notify_violations_increased = false;
      if (verbosity_level >= 3)
        notify_made_move = true;
      else
        notify_made_move = false;
      
      // plot
      if (plot_level >= 1)
        plot_improving_moves = true;
      else
        plot_improving_moves = false;
      if (plot_level >= 2)
        plot_all_moves = true;
      else
        plot_all_moves = false;
    }
    
    template <class Input, class State, class Move, typename CFtype>
    void RunnerObserver<Input, State, Move, CFtype>::NotifyStartRunner(MoveRunner<Input, State, Move, CFtype>& r)
    {
      if (plot_improving_moves || plot_all_moves)
        plot << r.iteration << ' ' <<
        std::chrono::duration_cast<std::chrono::milliseconds>(r.end - r.begin).count() / 1000.0 << "s " <<
        r.current_state_cost <<
        std::endl;
      previous_violations = r.current_state_violations;
    }
    
    template <class Input, class State, class Move, typename CFtype>
    void RunnerObserver<Input, State, Move, CFtype>::NotifyNewBest(MoveRunner<Input, State, Move, CFtype>& r)
    {
      if (notify_new_best)
      {
        log << "--New best: " << r.current_state_cost
        << " (it: " << r.iteration << ", idle: " << r.iteration - r.iteration_of_best
        << "), Costs: (";
        for (unsigned int i = 0; i < r.sm.CostComponents(); i++)
        {
          log << r.sm.Cost(*r.p_current_state, i);
          if (i < r.sm.CostComponents() - 1)
            log << ", ";
        }
        log << ") " << r.StatusString() << std::endl;
      }
      if (plot_improving_moves && !plot_all_moves)
        plot << r.name << ' ' <<
        r.iteration << ' ' <<
        std::chrono::duration_cast<std::chrono::milliseconds>(r.end - r.begin).count() / 1000.0 << "s " <<
        r.current_state_cost <<
        std::endl;
    }
    
    template <class Input, class State, class Move, typename CFtype>
    void RunnerObserver<Input, State, Move, CFtype>::NotifyMadeMove(MoveRunner<Input, State, Move, CFtype>& r)
    {
      if (notify_made_move)
      {
        log << "Move: " << r.current_move << ", Move Cost: " << r.current_move_cost << " (current: "
        << r.current_state_cost << ", best: "
        << r.best_state_cost <<  ") it: " << r.iteration
        << " (idle: " << r.iteration - r.iteration_of_best << ")"
        << "), Costs: (";
        for (unsigned int i = 0; i < r.sm.CostComponents(); i++)
        {
          log << r.sm.Cost(*r.p_current_state, i);
          if (i < r.sm.CostComponents() - 1)
            log << ", ";
        }
        log << ") " << r.StatusString() << std::endl;
      }
      
      if (notify_violations_increased && r.current_state_violations > previous_violations)
      {
        log << "Violations increased (" << previous_violations << " -> " << r.current_state_violations << "), cost " << ((previous_cost >= r.current_state_cost) ? ((previous_cost > r.current_state_cost) ? "increased" : "is unchanged") : "decreased" ) << std::endl;
      }
      previous_violations = r.current_state_violations;
      previous_cost = r.current_state_cost;
      if (plot_all_moves)
        plot << r.name << ' ' <<
        r.iteration << ' '  <<
        std::chrono::duration_cast<std::chrono::milliseconds>(r.end - r.begin).count() / 1000.0 << "s " <<
        r.current_state_cost <<
        std::endl;
    }
    
    template <class Input, class State, class Move, typename CFtype>
    void RunnerObserver<Input, State, Move, CFtype>::NotifyEndRunner(MoveRunner<Input, State, Move, CFtype>& r)
    {
      if (plot_improving_moves || plot_all_moves)
        plot << r.iteration << ' ' <<
        std::chrono::duration_cast<std::chrono::milliseconds>(r.end - r.begin).count() / 1000.0 << "s " <<
        r.current_state_cost << std::endl;
    }
    
  }
}

#endif // _RUNNER_OBSERVER_HH_
