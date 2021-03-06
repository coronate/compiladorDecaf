/* File: ast.h
 * -----------
 * This file defines the abstract base class Node and the concrete
 * Identifier and Error node subclasses that are used through the tree as
 * leaf nodes. A parse tree is a hierarchical collection of ast nodes (or,
 * more correctly, of instances of concrete subclassses such as VarDecl,
 * ForStmt, and AssignExpr).
 *
 * Location: Each node maintains its lexical location (line and columns in
 * file), that location can be NULL for those nodes that don't care/use
 * locations. The location is typcially set by the node constructor.  The
 * location is used to provide the context when reporting semantic errors.
 *
 * Parent: Each node has a pointer to its parent. For a Program node, the
 * parent is NULL, for all other nodes it is the pointer to the node one level
 * up in the parse tree.  The parent is not set in the constructor (during a
 * bottom-up parse we don't know the parent at the time of construction) but
 * instead we wait until assigning the children into the parent node and then
 * set up links in both directions. The parent link is typically not used
 * during parsing, but is more important in later phases.
 *
 * Printing: The only interesting behavior of the node classes for pp2 is the
 * bility to print the tree using an in-order walk.  Each node class is
 * responsible for printing itself/children by overriding the virtual
 * PrintChildren() and GetPrintNameForNode() methods. All the classes we
 * provide already implement these methods, so your job is to construct the
 * nodes and wire them up during parsing. Once that's done, printing is a snap!
 *
 * Semantic analysis: For pp3 you are adding "Check" behavior to the ast
 * node classes. Your semantic analyzer should do an inorder walk on the
 * parse tree, and when visiting each node, verify the particular
 * semantic rules that apply to that construct.
 *
 * Code generation: For pp5 you are adding "Emit" behavior to the ast
 * node classes. Your code generator should do an postorder walk on the
 * parse tree, and when visiting each node, emitting the necessary
 * instructions for that construct.
 *
 * Author: Deyuan Guo
 */

#ifndef _H_ast
#define _H_ast

#include <iostream>
#include <stdlib.h>   // for NULL
#include "location.h"
#include "errors.h"
#include "codegen.h"

// the global code generator class.
extern CodeGenerator *CG;

class Node
{
  protected:
    yyltype *location;
    Node *parent;
    Type *expr_type; // link to the type of each node (not for stmt)
    Location *emit_loc;

  public:
    // constructor.
    Node(yyltype loc);
    Node();
    // utilities.
    yyltype *GetLocation()   { return location; }
    void SetParent(Node *p)  { parent = p; }
    Node *GetParent()        { return parent; }
    // print stuff.
    virtual const char *GetPrintNameForNode() = 0;
    // Print() is deliberately _not_ virtual
    // subclasses should override PrintChildren() instead
    void Print(int indentLevel, const char *label = NULL);
    virtual void PrintChildren(int indentLevel) {}
    // semantic check stuff.
    virtual void Check(checkT c) {}
    virtual Type * GetType() { return expr_type; }
    virtual bool IsLoopStmt() { return false; }
    virtual bool IsSwitchStmt() { return false; }
    virtual bool IsCaseStmt() { return false; }
    // code generation stuff.
    virtual void Emit() {}
    virtual Location * GetEmitLoc() { return emit_loc; }
};

class Identifier : public Node
{
  protected:
    char *name;
    Decl *decl;

  public:
    // constructor.
    Identifier(yyltype loc, const char *name);
    // print stuff.
    const char *GetPrintNameForNode()   { return "Identifier"; }
    void PrintChildren(int indentLevel);
    friend std::ostream& operator<<(std::ostream& out, Identifier *id)
        { return out << id->name; }
    // semantic check stuff.
    void Check(checkT c);
    bool IsEquivalentTo(Identifier *other);
    char *GetIdName() { return name; }
    void SetDecl(Decl *d) { decl = d; }
    Decl * GetDecl() { return decl; }
    // code generation stuff.
    void Emit();
    void AddPrefix(const char *prefix);
    Location * GetEmitLocDeref() { return GetEmitLoc(); }

  protected:
    void CheckDecl();
};

// This node class is designed to represent a portion of the tree that
// encountered syntax errors during parsing. The partial completed tree
// is discarded along with the states being popped, and an instance of
// the Error class can stand in as the placeholder in the parse tree
// when your parser can continue after an error.
class Error : public Node
{
  public:
    // constructor.
    Error() : Node() {}
    // print stuff.
    const char *GetPrintNameForNode()   { return "Error"; }
};

#endif

