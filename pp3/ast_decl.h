/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "ast_type.h"
#include "hashtable.h"
#include "list.h"

class Identifier;
class StmtBlock;


class Decl : public Node 
{
 protected:
  Identifier *id;
  
 public:
  Decl(Identifier *name);
  Identifier *GetID() { return id; }
  friend std::ostream& operator<<(std::ostream &out, Decl *decl) { if (decl) return out << decl->id; else return out; }
  virtual const char *GetTypeName() { return NULL; }
  virtual Type *GetType() { return NULL; }
};

class VarDecl : public Decl 
{
 protected:
  Type *type;
    
 public:
  VarDecl(Identifier *name, Type *type);
  Type *GetType() { return type; }
  const char *GetTypeName() { if (type) return type->GetTypeName(); else return NULL; }
  bool HasSameTypeSig(VarDecl *vd);
  void ReviewStatements();
  void errorDeclReview();
};


class ClassDecl : public Decl 
{
 protected:
  List<Decl*> *members;
  NamedType *extends;
  List<NamedType*> *implements;
  Hashtable<Decl*> *tablaHash;

 public:
  ClassDecl(Identifier *name, NamedType *extends, 
	    List<NamedType*> *implements, List<Decl*> *members);
  NamedType *GetExtends() { return extends; }
  List<NamedType*> *GetImplements() { return implements; }
  void ReviewStatements();
  void errorDeclReview();
  bool IsCompatibleWith(Decl *decl);
  Hashtable<Decl*> *GetSymTable() { return tablaHash; }
};

class InterfaceDecl : public Decl 
{
 protected:
  List<Decl*> *members;
  Hashtable<Decl*> *tablaHash;

 public:
  InterfaceDecl(Identifier *name, List<Decl*> *members);
  void errorDeclReview();
  List<Decl*> *GetMembers() { return members; }
  Hashtable<Decl*> *GetSymTable() { return tablaHash; }

};

class FnDecl : public Decl 
{
 protected:
  List<VarDecl*> *formals;
  Type *returnType;
  StmtBlock *body;
  Hashtable<Decl*> *tablaHash;
    
 public:
  FnDecl(Identifier *name, Type *returnType, List<VarDecl*> *formals);
  void SetFunctionBody(StmtBlock *b);
  void ReviewStatements();
  void errorDeclReview();
  Type *GetType() { return returnType; }
  List<VarDecl*> *GetFormals() { return formals; }
  const char *GetTypeName() { if (returnType) return returnType->GetTypeName(); else return NULL; }
  bool HasSameTypeSig(FnDecl *fd);
  Hashtable<Decl*> *GetSymTable() { return tablaHash; }

};


#endif
