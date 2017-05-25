/* File: ast_expr.h
 * ----------------
 * The Expr class and its subclasses are used to represent
 * expressions in the parse tree.  For each expression in the
 * language (add, call, New, etc.) there is a corresponding
 * node class for that construct. 
 *
 * pp3: You will need to extend the Expr classes to implement 
 * semantic analysis for rules pertaining to expressions.
 */


#ifndef _H_ast_expr
#define _H_ast_expr

#include "ast.h"
#include "ast_stmt.h"
#include "list.h"
#include "hashtable.h"
#include "errors.h"
#include "ast_type.h"

class NamedType; // for new
class Type; // for NewArray

enum OpT {ENot, ENeg, EMul, EDiv, EMod, EPlus, ELess, ELessE, EGreat, EGreatE, EEqual, ENEqual, EAnd, EOr, EAssign};

class Expr : public Stmt 
{
  public:
    Type* evalType;
    Location* memoryLocation;
    Expr(yyltype loc);
    Expr();
    virtual Hashtable<Node*>* getScope(Hashtable<Node*>* table);
};

/* This node type is used for those places where an expression is optional.
 * We could use a NULL pointer, but then it adds a lot of checking for
 * NULL. By using a valid, but no-op, node, we save that trouble */
class EmptyExpr : public Expr
{
  public:
  EmptyExpr();
};

class IntConstant : public Expr 
{
  public:
    int value;
    IntConstant(yyltype loc, int val);
    virtual void Emit(CodeGenerator*);
};

class DoubleConstant : public Expr 
{
  protected:
    double value;
    
  public:
    DoubleConstant(yyltype loc, double val);
};

class BoolConstant : public Expr 
{
  protected:
    bool value;
    
  public:
    BoolConstant(yyltype loc, bool val);
    virtual void Emit(CodeGenerator*);
};

class StringConstant : public Expr 
{ 
  protected:
    char *value;
    
  public:
    StringConstant(yyltype loc, const char *val);
    virtual void Emit(CodeGenerator*);
};

class NullConstant: public Expr 
{
  public: 
    NullConstant(yyltype loc); 
    virtual void Emit(CodeGenerator*);
};

class Operator : public Node 
{
  public:
    char tokenString[4];
    OpT opType;
    
    Operator(yyltype loc, const char *tok);
    friend std::ostream& operator<<(std::ostream& out, Operator *o) { return out << o->tokenString; }
 };
 
class CompoundExpr : public Expr
{
  public:
    Operator *op;
    Expr *left, *right; // left will be NULL if unary
    
    CompoundExpr(Expr *lhs, Operator *op, Expr *rhs); // for binary
    CompoundExpr(Operator *op, Expr *rhs);             // for unary
    
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

class ArithmeticExpr : public CompoundExpr 
{
  public:
    ArithmeticExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    ArithmeticExpr(Operator *op, Expr *rhs) : CompoundExpr(op,rhs) {}
    virtual void Check(Hashtable<Node*> *, int);
};

class RelationalExpr : public CompoundExpr 
{
  public:
    RelationalExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

class EqualityExpr : public CompoundExpr 
{
  public:
    EqualityExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    const char *GetPrintNameForNode() { return "EqualityExpr"; }
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

class LogicalExpr : public CompoundExpr 
{
  public:
    LogicalExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    LogicalExpr(Operator *op, Expr *rhs) : CompoundExpr(op,rhs) {}
    const char *GetPrintNameForNode() { return "LogicalExpr"; }
    virtual void Check(Hashtable<Node*> *, int);
};

class AssignExpr : public CompoundExpr 
{
  public:
    bool storeWithOffset;
    int offset;
    AssignExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) { storeWithOffset = false; }
    const char *GetPrintNameForNode() { return "AssignExpr"; }
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

class LValue : public Expr 
{
  public:
    LValue(yyltype loc) : Expr(loc) {}
};

class This : public Expr 
{
  public:
    This(yyltype loc) : Expr(loc) {}
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

class ArrayAccess : public LValue 
{
  protected:
    Expr *base, *subscript;
    
  public:
    ArrayAccess(yyltype loc, Expr *base, Expr *subscript);
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

/* Note that field access is used both for qualified names
 * base.field and just field without qualification. We don't
 * know for sure whether there is an implicit "this." in
 * front until later on, so we use one node type for either
 * and sort it out later. */
class FieldAccess : public LValue 
{
  protected:
    Expr *base;	// will be NULL if no explicit base
    Identifier *field;
    
  public:
    FieldAccess(Expr *base, Identifier *field); //ok to pass NULL base
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

/* Like field access, call is used both for qualified base.field()
 * and unqualified field().  We won't figure out until later
 * whether we need implicit "this." so we use one node type for either
 * and sort it out later. */
class Call : public Expr 
{
  protected:
    Expr *base;	// will be NULL if no explicit base
    Identifier *field;
    List<Expr*> *actuals;
    
  public:
    Call(yyltype loc, Expr *base, Identifier *field, List<Expr*> *args);
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

class NewExpr : public Expr
{
  protected:
    NamedType *cType;
    
  public:
    NewExpr(yyltype loc, NamedType *clsType);
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

class NewArrayExpr : public Expr
{
  protected:
    Expr *size;
    Type *elemType;
    
  public:
    NewArrayExpr(yyltype loc, Expr *sizeExpr, Type *elemType);
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

class ReadIntegerExpr : public Expr
{
  public:
    ReadIntegerExpr(yyltype loc) : Expr(loc) {}
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

class ReadLineExpr : public Expr
{
  public:
    ReadLineExpr(yyltype loc) : Expr (loc) {}
    virtual void Check(Hashtable<Node*> *, int);
    virtual void Emit(CodeGenerator*);
};

    
#endif