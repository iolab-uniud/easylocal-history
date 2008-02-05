#ifndef TABULISTMANAGER_HH_
#define TABULISTMANAGER_HH_

#include <list>
#include "ProhibitionManager.hh"
#include "TabuListItem.hh"
#include "../utils/Random.hh"
#include "../utils/Types.hh"

// forward class tag declaration
template <class Move, typename CFtype = int> class ListItem;

/** The Tabu List Manager handles a list of @c Move elements according
    to the prohibition mechanisms of tabu search.
    Namely it maintains an item in the list for a number of iterations 
    that varies randomly in a given range.
    Each time a new @c Move is inserted in the list, the ones which their 
    iteration count has expired are removed.
    @ingroup Helpers
*/
template <class State, class Move, typename CFtype = int>
class TabuListManager
: public ProhibitionManager<State,Move,CFtype>
{
public:
	void Print(std::ostream& os = std::cout) const;
	void InsertMove(const State& st, const Move& mv, const CFtype& mv_cost, const CFtype& curr, const CFtype& best);
	bool ProhibitedMove(const State& st, const Move& mv, const CFtype& mv_cost) const;
	/** Sets the length of the tabu list to be comprised in the range
	 [min, max].
	 @param min the minimum tabu tenure
	 @param max the maximum tabu tenure */
	void SetLength(unsigned int min, unsigned int max);
	void ReadParameters(std::istream& is = std::cin,
											std::ostream& os = std::cout);
	void Clean();
	/** Returns the minimum number of iterations a move is considered tabu.
	 @return the minimum tabu tenure */
	unsigned int MinTenure() const
	{ return min_tenure; }
	/** Returns the maximum number of iterations a move is considered tabu.
	 @return the maximum tabu tenure */
	unsigned int MaxTenure() const
	{ return max_tenure; }
	/** Verifies whether a move is the inverse of another one. Namely it
	 tests whether mv1 is the inverse of mv2 (that will be an element of
	 the tabu list).
	 @note @bf To be implemented in the application.
	 @param mv1 the move to be tested
	 @param mv2 the move used for comparison  */
	virtual bool Inverse(const Move& mv1, const Move& mv2) const = 0;
	void UpdateIteration() { PurgeList(); iter++; }
	TabuListManager();
	/** Virtual destructor. */
	virtual ~TabuListManager();
protected:    
	virtual bool Aspiration(const State& st, const Move&, const CFtype& mv_cost) const;
	virtual void InsertIntoList(const State& st, const Move& mv);
	void PurgeList();
	/** Updates the function associated with the aspiration criterion.
	 For default it does nothing.
	 @param mv_cost the cost of the move
	 @param curr the cost of the current solution
	 @param best the cost of the best solution found so far */
	void UpdateAspirationFunction(const CFtype& curr_cost, const CFtype& best_cost)
	{ current_state_cost = curr_cost; best_state_cost = best_cost; }
	bool ListMember(const Move&) const;
	// parameters
	unsigned int min_tenure; /**< The minimum tenure of the tabu list. */
	unsigned int max_tenure;  /**< The maximum tenure of the tabu list. */
	unsigned long iter; /**< The current iteration. */
	std::list<TabuListItem<State,Move,CFtype> > tlist; /**< The list of tabu moves. */
	CFtype current_state_cost; /**< The cost of current state of the attached runner (for the aspiration criterion) */
	CFtype best_state_cost; /**< The cost of best state of the attached runner (for the aspiration criterion) */
};

/*************************************************************************
 * Implementation
 *************************************************************************/
/**
    Constructs a tabu list manager object which manages a list of 
    the given tenure (i.e., the number of steps a move is considered tabu).
    
    @param min the minimum tabu tenure
    @param max the maximum tabu tenure
*/
template <class State, class Move, typename CFtype>
TabuListManager<State, Move,CFtype>::TabuListManager()
: min_tenure(0), max_tenure(1), iter(0)
{}

template <class State, class Move, typename CFtype>
void TabuListManager<State, Move,CFtype>::SetLength(unsigned int min, unsigned int max)
{ 
	min_tenure = min; 
	max_tenure = max; 
}

template <class State, class Move, typename CFtype>
void TabuListManager<State, Move,CFtype>::ReadParameters(std::istream& is,
        std::ostream& os)
{
    os << "  TABU LIST PARAMETERS" << std::endl;
    os << "    Length of the tabu list (min,max): ";
    is >> min_tenure >> max_tenure;
}

template <class State, class Move, typename CFtype>
TabuListManager<State, Move,CFtype>::~TabuListManager()
{}

/**
   Inserts the move in the tabu list and updates the aspiration function.

   @param mv the move to add
   @param mv_cost the move cost
   @param best the best state cost found so far
*/
template <class State, class Move, typename CFtype>
void TabuListManager<State, Move,CFtype>::InsertMove(const State& st, const Move& mv, const CFtype& mv_cost, const CFtype& curr, 
						     const CFtype& best)
{
    InsertIntoList(st, mv);
    UpdateAspirationFunction(curr,best);
}

/**
   Checks whether the given move is prohibited.

   @param mv the move to check
   @param mv_cost the move cost
   @param curr the current state cost
   @param best the best state cost found so far
   @return true if the move mv is prohibited, false otherwise
*/
template <class State, class Move, typename CFtype>
bool TabuListManager<State, Move,CFtype>::ProhibitedMove(const State& st, const Move& mv, const CFtype& mv_cost) const
{
  return !Aspiration(st, mv, mv_cost) && ListMember(mv);
}

/**
    Cleans the data: deletes all the elements of the tabu list.
*/
template <class State, class Move, typename CFtype>
void TabuListManager<State, Move,CFtype>::Clean()
{
    tlist.clear();
    iter = 0;
}

/**
    Checks whether the inverse of a given move belongs to the tabu list.
    
    @param mv the move to check
    @return true if the inverse of the move belongs to the tabu list, 
    false otherwise
*/
template <class State, class Move, typename CFtype>
bool TabuListManager<State, Move,CFtype>::ListMember(const Move& mv) const
{
    typename std::list<TabuListItem<State,Move,CFtype> >::const_iterator p = tlist.begin();
    while (p != tlist.end())
    {
      if (Inverse(mv,p->elem))
	return true;
      else
	p++;
    }
    return false;
}

/**
    Prints the current status of the tabu list on an output stream.
    
    @param os the output stream
    @param tl the tabu list manager to output
*/
template <class State, class Move, typename CFtype>
void TabuListManager<State, Move,CFtype>::Print(std::ostream& os) const
{
    os <<  "Tabu List Manager: " << this->GetName() << std::endl;
    os <<  "  Tenure: " << min_tenure << " - " << max_tenure << std::endl;
    typename std::list<TabuListItem<State,Move,CFtype> >::const_iterator p = tlist.begin();
    while (p != tlist.end())
    {
        os << "  " << p->elem << " (" << p->out_iter - iter << ")" << std::endl;
        p++;
    }
}


/**
   Checks whether the aspiration criterion is satisfied for a given move.
   By default, it verifies if the move cost applied to the current state
   gives a value lower than the best state cost found so far.

   @param mv the move
   @param mv_cost the move cost
   @param curr the cost of the current state
   @param best the cost of the best state found so far
   @return true if the aspiration criterion is satisfied, false otherwise
*/
template <class State, class Move, typename CFtype>
bool TabuListManager<State, Move,CFtype>::Aspiration(const State& st, const Move&, const CFtype& mv_cost) const
    { 
      return LessThan<CFtype>(current_state_cost + mv_cost, best_state_cost); 
    }

/**
   Inserts the move into the tabu list, and update the list removing
   the moves for which the tenure has elapsed.

   @param mv the move to add
*/
template <class State, class Move, typename CFtype>
void TabuListManager<State, Move,CFtype>::InsertIntoList(const State& st, const Move& mv)
{
    unsigned int tenure = (unsigned int)Random::Int(min_tenure, max_tenure);
    TabuListItem<State,Move,CFtype> li(mv, iter + tenure);
    tlist.push_front(li);

    UpdateIteration();
}

/**
   Inserts the move into the tabu list, and update the list removing
   the moves for which the tenure has elapsed.

   @param mv the move to add
*/
template <class State, class Move, typename CFtype>
void TabuListManager<State, Move,CFtype>::PurgeList()
{
    typename std::list<TabuListItem<State,Move,CFtype> >::iterator p = tlist.begin();
    while (p != tlist.end())
        if (p->out_iter <= iter) // era "p->out_iter == iter". Ora consideriamo anche minore
            // perche' nel caso di runner multimodale questa funzione
            // non viene invocata a tutte le iterazioni
            p = tlist.erase(p);
        else
            p++;
}

#endif /*TABULISTMANAGER_HH_*/
