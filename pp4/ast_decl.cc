#include <stdio.h>
#include <string.h>

#include <typeinfo>

#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "errors.h"


Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
  Assert(n != NULL);
  (this->id=n)->SetParent(this); 
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
  Assert(n != NULL && t != NULL);
  (this->type=t)->SetParent(this);
}

bool VarDecl::HasSameTypeSig(VarDecl *vd) {
  if (this->type)
    return this->type->HasSameType(vd->GetType());
  else 
    return false;
}

void VarDecl::stmtReview() {
   
}
 
 
void VarDecl::errorDeclReview() {
  if (this->type)
    this->type->errorTypeReview();
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
   
  Assert(n != NULL && imp != NULL && m != NULL);     
  this->extends = ex;
  if (this->extends) this->extends->SetParent(this);
  (this->implements=imp)->SetParentAll(this);
  (this->members=m)->SetParentAll(this);
  this->sym_table = new Hashtable<Decl*>;
}

void ClassDecl::stmtReview() {
  if (this->members)
    {
      for (int i = 0; i < this->members->NumElements(); i++)
	this->members->Nth(i)->stmtReview();
    }
}

void ClassDecl::errorDeclReview() {
   
  this->sym_table->Enter(this->GetID()->GetName(), this);

   
  if (this->members)
    {
      for (int i = 0; i < this->members->NumElements(); i++)
        {
	  Decl *cur = this->members->Nth(i);
	  Decl *prev;
	  const char *name = cur->GetID()->GetName();
	  if (name)
	    {
	      if ((prev = this->sym_table->Lookup(name)) != NULL)
		ReportError::DeclConflict(cur, prev);
	      else
		this->sym_table->Enter(name, cur);
	    }
        }
    }

   
   
   
  NamedType *ex = this->extends;
  while (ex)
    {
      const char *name = ex->GetID()->GetName();
      if (name)
	{
	  Node *node = Program::sym_table->Lookup(name);
	  if (node == NULL)
	    {
	      ReportError::IdentifierNotDeclared(ex->GetID(), LookingForClass);
	      break;
	    }
	  else if (typeid(*node) == typeid(ClassDecl))
	    {
	      ClassDecl *base = dynamic_cast<ClassDecl*>(node);
	      List<Decl*> *base_members = base->members;
	      List<Decl*> *inherited = new List<Decl*>;
	       
	      if (base_members)
		{
		  for (int i = 0; i < base_members->NumElements(); i++)
		    {
		      Decl *cur = base_members->Nth(i);
		      Decl *prev;
		      const char *name = cur->GetID()->GetName();
		      if ((prev = this->sym_table->Lookup(name)) != NULL)
			{
			  if (typeid(*cur) == typeid(VarDecl) || typeid(*cur) != typeid(*prev))  
			    ReportError::DeclConflict(prev, cur);
			  else if (typeid(*cur) == typeid(FnDecl) && typeid(*cur) == typeid(*prev))  
			    {
			      FnDecl *fdcur = dynamic_cast<FnDecl*>(cur);
			      FnDecl *fdprev = dynamic_cast<FnDecl*>(prev);
			      if (!fdcur->HasSameTypeSig(fdprev))
				ReportError::OverrideMismatch(fdprev);
			    }
			}
		      else  
			 
			 
			{
			  inherited->Append(cur);
			}
		    }
		   for (int i = 0; i < inherited->NumElements(); i++)
		     {
		       Decl *decl = inherited->Nth(i);
		       this->sym_table->Enter(decl->GetID()->GetName(), decl);
		     }
		}
	      ex = base->GetExtends();
	    }
	}
    }

  if (this->implements)
    {
      for (int i = 0; i < this->implements->NumElements(); i++)
	{
          NamedType *implement = this->implements->Nth(i);
	  Identifier *id = implement->GetID();
	  if (id)
	    {
	      Node *node = Program::sym_table->Lookup(id->GetName());
	      if (node == NULL || (typeid(*node) != typeid(InterfaceDecl)))
		{
		  ReportError::IdentifierNotDeclared(id, LookingForInterface);
		}
	      else if (typeid(*node) == typeid(InterfaceDecl))
		{
		  InterfaceDecl *ifd = dynamic_cast<InterfaceDecl*>(node);
		  List<Decl*> *members = ifd->GetMembers();
		  for (int j = 0; j < members->NumElements(); j++)
		    {
		      FnDecl *cur = dynamic_cast<FnDecl*>(members->Nth(j));
		      Decl *prev;
		      const char *name = cur->GetID()->GetName();
		;
		      if ((prev = this->sym_table->Lookup(name)) != NULL)
			{
			  if (typeid(*prev) != typeid(FnDecl))
			    ReportError::DeclConflict(cur, prev);
			  else if (!cur->HasSameTypeSig(dynamic_cast<FnDecl*>(prev)))
			    ReportError::OverrideMismatch(prev);
			}
		      else
			ReportError::InterfaceNotImplemented(this, implement);
		    }
		}
	    }
	}
    }

   
   
  if (this->members)
    {
      for (int i = 0; i < this->members->NumElements(); i++)
	this->members->Nth(i)->errorDeclReview();
    }
}

 
 
 
bool ClassDecl::mostrarCompatibilidad(Decl *decl)
{
  NamedType *extends = this->GetExtends();
  List<NamedType*> *implements = this->GetImplements();

  if (typeid(*decl) == typeid(ClassDecl))
    {
      ClassDecl *cldecl = dynamic_cast<ClassDecl*>(decl);
       
      if (extends)
        {
          const char *name = extends->GetTypeName();
          if (!strcmp(cldecl->GetID()->GetName(), name))
	    return true;
          else
           {
             if (name)
               {
                 Decl *exdecl = Program::sym_table->Lookup(name);
                 if (exdecl && typeid(*exdecl) == typeid(ClassDecl))
                   return dynamic_cast<ClassDecl*>(exdecl)->mostrarCompatibilidad(decl);
               }
           }
        }
    }
   
  else if (typeid(*decl) == typeid(InterfaceDecl))
    {
      InterfaceDecl *itfdecl = dynamic_cast<InterfaceDecl*>(decl);

      if (implements)
	{
	  for (int i = 0; i < implements->NumElements(); i++)
	    {
	      NamedType *implement = implements->Nth(i);
	      if (implement && !strcmp(itfdecl->GetID()->GetName(), implement->GetTypeName()))
		return true;
		  
	    }
	}
       if (extends)
        {
          const char *name = extends->GetTypeName();
          if (name)
            {
              Decl *exdecl = Program::sym_table->Lookup(name);
              if (exdecl && typeid(*exdecl) == typeid(ClassDecl))
                return dynamic_cast<ClassDecl*>(exdecl)->mostrarCompatibilidad(decl);
            }
        }
    }

  return false;
}


InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
  Assert(n != NULL && m != NULL);
  (this->members=m)->SetParentAll(this);
  this->sym_table  = new Hashtable<Decl*>;
}


void InterfaceDecl::errorDeclReview() {
  if (this->members)
    {
      for (int i = 0; i < this->members->NumElements(); i++)
	{
	  Decl *cur = members->Nth(i);
	  Decl *prev;
	  const char *name = cur->GetID()->GetName();
	  if (name)
	    {
	      if ((prev = this->sym_table->Lookup(name)) != NULL)
		{
		  ReportError::DeclConflict(cur, prev);
		}
	      else
		{
		  sym_table->Enter(name, cur);
		}
	    }
	}
    }
}
	

FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
  Assert(n != NULL && r!= NULL && d != NULL);
  (this->returnType=r)->SetParent(this);
  (this->formals=d)->SetParentAll(this);
  this->body = NULL;
  this->sym_table  = new Hashtable<Decl*>;
}

 
 
bool FnDecl::HasSameTypeSig(FnDecl *fd) {
  if (!strcmp(this->id->GetName(), fd->GetID()->GetName()))
    if (this->returnType->HasSameType(fd->GetType()))
      {
	List<VarDecl*> *f1 = formals;
	List<VarDecl*> *f2 = fd->GetFormals();

	if (f1 && f2)
	  if (f1->NumElements() == f2->NumElements())
	    {
	      for (int i = 0; i < f1->NumElements(); i++)
		{
		  VarDecl *vd1 = f1->Nth(i);
		  VarDecl *vd2 = f2->Nth(i);
		  if (!vd1->HasSameTypeSig(vd2))
		    return false;
		}
	      return true;
	    }
      }

  return false;

}

void FnDecl::stmtReview() {
  if (this->body)
    this->body->stmtReview();
}

void FnDecl::errorDeclReview() {
  if (this->formals)
    {
      for (int i = 0; i < this->formals->NumElements(); i++)
	{
	  VarDecl *cur = this->formals->Nth(i);
	  Decl *prev;
	  const char *name = cur->GetID()->GetName();
	  if (name)
	    {
	      if ((prev = this->sym_table->Lookup(name)) != NULL)
		{
		  ReportError::DeclConflict(cur, prev);
		}
	      else
		{
		  sym_table->Enter(name, cur);
		  cur->errorDeclReview();
		}
	    }
	}
    }
  if (this->body)
    this->body->errorDeclReview();
}

void FnDecl::SetFunctionBody(StmtBlock *b) {
  (this->body=b)->SetParent(this);
}
