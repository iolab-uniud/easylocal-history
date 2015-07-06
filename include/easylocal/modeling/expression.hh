#ifndef _EXPRESSION_HH
#define _EXPRESSION_HH

#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

#include "easylocal/utils/printable.hh"
#include "ast.hh"
#include "operators.hh"
#include <iterator>
#include "easylocal/utils/random.hh"

namespace EasyLocal
{
  /** Implementing efficient and correct "deltas" for local search applications
      can be a tricky and error-prone process. Modeling expressions attempt to
      solve this problem through a symbolic approach. They are based on the idea
      that by analyzing the *structure* of an expression and its current value, 
      it is possible to efficiently compute the variation in the value of the
      whole expression as the values of the terminal nodes, and in particular of 
      the variables, change.
      The basic (terminal) modeling expressions are variables (Var), variable 
      arrays (VarArray), constants (just plain values). Using these and a number
      operators to manipulate them, we are able to build more complex expressions 
      to be used as cost components. Among the operators are
   
        * ==, !=, <, <=, ...
        * +, -, *, /, %
        * alldifferent
        * element
        * max, min
        * ...
   
      Each expression points to a corresponding node in an abstract syntax tree
      (AST) which is mainly used to simplify the expressions. Each node has a
      simplification procedure which depends on what the operands are. Simplifi-
      cation is important, because the cost of computing the deltas depends on 
      the size of the expression.
   */
  namespace Modeling
  {
    /** Generic base class representing modeling expression of type T. */
    template <typename T>
    class Exp : public virtual Core::Printable
    {
      friend class ASTOp<T>;
    public:
      
      /** Null expression (used to implement move construction). */
      Exp() = default;
      
      /** Constant expression. */
      Exp(T value) : p_ai(std::make_shared<ASTConst<T>>(value)) { }
      
      /** Copy constructor. */
      Exp(const Exp<T>& other) : p_ai(other.p_ai) { }

      /** Move constructor (using swap and the default constructor). */
      Exp(Exp<T>&& other) : Exp<T>()
      {
        swap(*this, other);
      }
      
      /** Swap (used for implementing move constructor and assignment). */
      friend void swap(Exp<T>& first, Exp<T>& second) // nothrow
      {
        using std::swap;
        swap(first.p_ai, second.p_ai);
      }
      
      /** Assignment (FIXME: why not move?). */
      Exp<T>& operator=(Exp<T>&& other) // (1)
      {
        swap(*this, other); // (2)
        return *this;
      }
      
      /** Creates an expression based on a node of the AST (for operator overloading). */
      Exp(const std::shared_ptr<ASTItem<T>>& p_ai) : p_ai(p_ai) { }
      
      /* Default Print (implementing Printable, forwards to ASTItem). */
      virtual void Print(std::ostream& os) const
      {
        p_ai->Print(os);
      }
      
      /** Default simplify (forwards to ASTItem).  Collapse operands. */
      void simplify()
      {
        p_ai = p_ai->simplify();
      }
      
      /** Default normalize (forwards to ASTItem). Sorts operands. */
      void normalize()
      {
        p_ai->normalize(true);
      }
      
      /** Virtual destructor, for inheritance. */
      virtual ~Exp<T>() { }
      
      /** Compute has function: to avoid processing symbols more than once. */
      size_t hash() const
      {
        return p_ai->hash();
      }
      
      /** Adds sub-AST to an expression store (with flattening). */
      size_t compile(ExpressionStore<T>& exp_store) const
      {
        return p_ai->compile(exp_store);
      }
      
    protected:
      
      /** Pointer to the AST item. */
      std::shared_ptr<ASTItem<T>> p_ai;
      
    };
    
    /** Array of Vars. */
    template <typename T>
    class Array;
    
    /**
     A modeling variable to be used inside expressions.     
     */
    template <typename T>
    class Var : public Exp<T>
    {
      friend class Array<T>;
    public:
      /**
       Constructor.
       @param exp_store a reference to the ExpressionStore (compiled expression) where the variable will be registered
       @param name name of the variable (for printing purposes)
       */
      explicit Var(ExpressionStore<T>& exp_store, const std::string& name)
      {
        this->p_ai = std::make_shared<ASTVar<T>>(name);
        this->p_ai->compile(exp_store);
      }
      
      Var() = default;
      
      /**
       Copy constructor.
       @param v the variable to copy
       */
      Var(const Var& v) = default;

      /** Virtual destructor. */
      virtual ~Var() = default;
      
      /** Name of the expression. 
          @return a string representing the expression
       */
      const std::string& name() const
      {
        ASTVar<T>* p_av = dynamic_cast<ASTVar<T>*>(this->p_ai.get());
        return p_av->name;
      }
    };
    
    /** Generic method to tell if two variables are the same variable. FIXME: check that equality makes sense
        @param v1 first variable
        @param v2 second variable
        @return true if the variables are the same variable
     */
    template <typename T>
    bool same_var(const Var<T>& v1, const ASTVar<T>* v2)
    {
      return v1.p_ai.get() == v2;
    }
    
    /** An Exp<T> array. */
    template <typename T>
    class Array : public std::vector<std::shared_ptr<Exp<T>>>, public Exp<T>
    {
    public:
      
      Array(ExpressionStore<T>& exp_store, const std::string& name, size_t size) : Array(exp_store)
      {
        // Create 'size' additional variables with the correct name
        for (unsigned int i = 0; i < size; i++)
        {
          std::stringstream ss;
          ss << name << "[" << i << "]";
          *this << std::make_shared<ASTVar<T>>(ss.str());
        }
      }
      
      /** Constructor.
          @param exp_store a reference to the ExpressionStore (compiled expression) where the array will be registered
       */
      Array(ExpressionStore<T>& exp_store)
      {
        this->p_ai = std::make_shared<ASTArray<T>>();
      }
      
      /** Operator to add expressions to the array (à la Gecode).
          @param e expression to add
          @return a reference to the array, for chained insertions (a << 1 << 2 << 3)
       */
      Array<T>& operator<<(std::shared_ptr<Exp<T>> e)
      {
        this->push_back(e);
        return *this;
      }
      
      /** Operator to add values to the array (à la Gecode). 
          @param v value to add to the array
       */
      Array& operator<<(const T& v) {
        return (*this) << std::make_shared<ASTConst<T>>(v);
      }
      
      /** Virtual destructor. */
      virtual ~Array() = default;
      
      /**
       Copy Constructor.
       @param the array to copy
       */
      Array(const Array& v) = default;

      /** Default constructor. */
      Array() = default;

      /** Retrieves expression at given index.
          @param i index of the expression to retrieve
          @return a reference to the (existing) expression
       */
      Exp<T>& operator[](size_t i) {
        return this->at(i)->get();
      }
      
      /** Generates element expression. 
          @param i index of the element to retrieve
          @return a new Element expression
       */
      Exp<T> operator[](const Exp<T>& index)
      {
        Exp<T> t = Exp<T>(std::make_shared<Element<T>>(index, *this));
        t.simplify();
        return t;
      }
      
    };
  }
}


#endif
