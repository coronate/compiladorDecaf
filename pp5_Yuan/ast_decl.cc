/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 *
 * Author: Deyuan Guo
 */

#include "ast_decl.h"
#include "ast_stmt.h"
#include "ast_type.h"
#include "list.h"
#include "errors.h"

Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this);
    idx = -1;
    expr_type = NULL;
}

VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
    class_member_ofst = -1;
}

void VarDecl::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
    if (class_member_ofst != -1)
        std::cout << " ~~[Ofst: " << class_member_ofst << "]";
    type->Print(indentLevel+1);
    id->Print(indentLevel+1);
    if (id->GetDecl()) printf(" ........ {def}");
}

void VarDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        idx = symtab->InsertSymbol(this);
        id->SetDecl(this);
    }
}

void VarDecl::CheckDecl() {
    type->Check(E_CheckDecl);
    id->Check(E_CheckDecl);

    expr_type = type->GetType();
}

void VarDecl::Check(checkT c) {
    switch (c) {
        case E_CheckDecl:
            this->CheckDecl(); break;
        case E_BuildST:
            this->BuildST(); break;
        default:
            type->Check(c);
            id->Check(c);
    }
}

void VarDecl::AssignOffset() {
    if (this->IsGlobal()) {
        emit_loc = new Location(gpRelative, CG->GetNextGlobalLoc(),
                id->GetIdName());
    }
}

void VarDecl::AssignMemberOffset(bool inClass, int offset) {
    class_member_ofst = offset;
    // set location for var members of class.
    emit_loc = new Location(fpRelative, offset, id->GetIdName(), CG->ThisPtr);
}

void VarDecl::Emit() {
    if (type == Type::doubleType) {
        ReportError::Formatted(this->GetLocation(),
                "Double type is not supported by compiler back end yet.");
        Assert(0);
    }

    if (!emit_loc) {
        // some auto variables.
        emit_loc = new Location(fpRelative, CG->GetNextLocalLoc(),
                id->GetIdName());
    }
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp,
        List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
    instance_size = 4;
    vtable_size = 0;
}

void ClassDecl::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
    id->Print(indentLevel+1);
    if (id->GetDecl()) printf(" ........ {def}");
    if (extends) extends->Print(indentLevel+1, "(extends) ");
    implements->PrintAll(indentLevel+1, "(implements) ");
    members->PrintAll(indentLevel+1);
}

void ClassDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        // if two local symbols have the same name, then report an error.
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        idx = symtab->InsertSymbol(this);
        id->SetDecl(this);
    }
    // record the owner of the current class scope.
    symtab->BuildScope(this->GetId()->GetIdName());
    if (extends) {
        // record the parent of the current class.
        symtab->SetScopeParent(extends->GetId()->GetIdName());
    }
    // record the implements of the current class.
    for (int i = 0; i < implements->NumElements(); i++) {
        symtab->SetInterface(implements->Nth(i)->GetId()->GetIdName());
    }
    members->CheckAll(E_BuildST);
    symtab->ExitScope();
}

void ClassDecl::CheckDecl() {
    id->Check(E_CheckDecl);
    // check extends.
    if (extends) {
        extends->Check(E_CheckDecl, LookingForClass);
    }
    // check interface.
    for (int i = 0; i < implements->NumElements(); i++) {
        implements->Nth(i)->Check(E_CheckDecl, LookingForInterface);
    }
    symtab->EnterScope();
    members->CheckAll(E_CheckDecl);
    symtab->ExitScope();

    expr_type = new NamedType(id);
    expr_type->SetSelfType();
}

void ClassDecl::CheckInherit() {
    symtab->EnterScope();

    for (int i = 0; i < members->NumElements(); i++) {
        Decl *d = members->Nth(i);
        Assert(d != NULL); // already inserted into symbol table.

        if (d->IsVarDecl()) {
            // check class inheritance of variables.
            Decl *t = symtab->LookupParent(d->GetId());
            if (t != NULL) {
                // subclass cannot override inherited variables.
                ReportError::DeclConflict(d, t);
            }
            // check interface inheritance of variables.
            t = symtab->LookupInterface(d->GetId());
            if (t != NULL) {
                // variable names conflict with interface method names.
                ReportError::DeclConflict(d, t);
            }

        } else if (d->IsFnDecl()) {
            // check class inheritance of functions.
            Decl *t = symtab->LookupParent(d->GetId());
            if (t != NULL) {
                if (!t->IsFnDecl()) {
                    ReportError::DeclConflict(d, t);
                } else {
                    // compare the function signature.
                    FnDecl *fn1 = dynamic_cast<FnDecl*>(d);
                    FnDecl *fn2 = dynamic_cast<FnDecl*>(t);
                    if (fn1->GetType() && fn2->GetType() // correct return type
                            && !fn1->IsEquivalentTo(fn2)) {
                        // subclass cannot overload inherited functions.
                        ReportError::OverrideMismatch(d);
                    }
                }
            }
            // check interface inheritance of functions.
            t = symtab->LookupInterface(d->GetId());
            if (t != NULL) {
                // compare the function signature.
                FnDecl *fn1 = dynamic_cast<FnDecl*>(d);
                FnDecl *fn2 = dynamic_cast<FnDecl*>(t);
                if (fn1->GetType() && fn2->GetType() // correct return type
                        && !fn1->IsEquivalentTo(fn2)) {
                    // subclass cannot overload inherited functions.
                    ReportError::OverrideMismatch(d);
                }
            }
            // check scopes within the function and keep active scope correct.
            d->Check(E_CheckInherit);
        }
    }

    symtab->ExitScope();
}

void ClassDecl::Check(checkT c) {
    switch (c) {
        case E_BuildST:
            this->BuildST(); break;
        case E_CheckDecl:
            this->CheckDecl(); break;
        case E_CheckInherit:
            this->CheckInherit(); break;
        default:
            id->Check(c);
            if (extends) extends->Check(c);
            implements->CheckAll(c);
            symtab->EnterScope();
            members->CheckAll(c);
            symtab->ExitScope();
    }
}

bool ClassDecl::IsChildOf(Decl *other) {
    if (other->IsClassDecl()) {
        if (id->IsEquivalentTo(other->GetId())) {
            // self.
            return true;
        } else if (!extends) {
            return false;
        } else {
            // look up all the parent classes.
            Decl *d = extends->GetId()->GetDecl();
            return dynamic_cast<ClassDecl*>(d)->IsChildOf(other);
        }
    } else if (other->IsInterfaceDecl()) {
        for (int i = 0; i < implements->NumElements(); i++) {
            Identifier *iid = implements->Nth(i)->GetId();
            if (iid->IsEquivalentTo(other->GetId())) {
                return true;
            }
        }
        if (!extends) {
            return false;
        } else {
            // look up all the parent classes' interfaces.
            Decl *d = extends->GetId()->GetDecl();
            return dynamic_cast<ClassDecl*>(d)->IsChildOf(other);
        }
    } else {
        return false;
    }
}

void ClassDecl::AddMembersToList(List<VarDecl*> *vars, List<FnDecl*> *fns) {
    for (int i = members->NumElements() - 1; i >= 0; i--) {
        Decl *d = members->Nth(i);
        if (d->IsVarDecl()) {
            vars->InsertAt(dynamic_cast<VarDecl*>(d), 0);
        } else if (d->IsFnDecl()) {
            fns->InsertAt(dynamic_cast<FnDecl*>(d), 0);
        }
    }
}

void ClassDecl::AssignOffset() {
    // deal with class inheritance.
    // add all parents' methods.
    var_members = new List<VarDecl*>;
    methods = new List<FnDecl*>;
    ClassDecl *c = this;
    while (c) {
        c->AddMembersToList(var_members, methods);
        NamedType *t = c->GetExtends();
        if (!t) break;
        c = dynamic_cast<ClassDecl*>(t->GetId()->GetDecl());
    }

    // delete override methods.
    for (int i = 0; i < methods->NumElements(); i++) {
        FnDecl *f1 = methods->Nth(i);
        for (int j = i + 1; j < methods->NumElements(); j++) {
            FnDecl *f2 = methods->Nth(j);
            if (!strcmp(f1->GetId()->GetIdName(), f2->GetId()->GetIdName())) {
                // replace the parent's method with child's method, then the
                // order of the methods can be compatible with both of them.
                methods->RemoveAt(i);
                methods->InsertAt(f2, i);
                methods->RemoveAt(j);
                j--;
            }
        }
    }

    PrintDebug("tac+", "Class Methods of %s:", id->GetIdName());
    for (int i = 0; i < methods->NumElements(); i++) {
        PrintDebug("tac+", "%s", methods->Nth(i)->GetId()->GetIdName());
    }
    PrintDebug("tac+", "Class Vars of %s:", id->GetIdName());
    for (int i = 0; i < var_members->NumElements(); i++) {
        PrintDebug("tac+", "%s", var_members->Nth(i)->GetId()->GetIdName());
    }

    // assign offset for fn members.
    instance_size = var_members->NumElements() * 4 + 4;
    vtable_size = methods->NumElements() * 4;

    int var_offset = instance_size;
    for (int i = members->NumElements() - 1; i >= 0; i--) {
        Decl *d = members->Nth(i);
        if (d->IsVarDecl()) {
            var_offset -= 4;
            d->AssignMemberOffset(true, var_offset);
        } else if (d->IsFnDecl()) {
            // find the right offset.
            for (int i = 0; i < methods->NumElements(); i++) {
                FnDecl *f1 = methods->Nth(i);
                if (!strcmp(f1->GetId()->GetIdName(), d->GetId()->GetIdName()))
                    d->AssignMemberOffset(true, i * 4);
            }
        }
    }
}

void ClassDecl::AddPrefixToMethods() {
    // add prefix to method names.
    for (int i = 0; i < members->NumElements(); i++) {
        members->Nth(i)->AddPrefixToMethods();
    }
}

void ClassDecl::Emit() {
    PrintDebug("tac+", "Begin Emitting TAC in ClassDecl.");

    members->EmitAll();

    // Emit VTable.
    List<const char*> *method_labels = new List<const char*>;
    for (int i = 0; i < methods->NumElements(); i++) {
        FnDecl* fn = methods->Nth(i);
        PrintDebug("tac+", "Insert %s into VTable.", fn->GetId()->GetIdName());
        method_labels->Append(fn->GetId()->GetIdName());
    }
}

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

void InterfaceDecl::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
    id->Print(indentLevel+1);
    if (id->GetDecl()) printf(" ........ {def}");
    members->PrintAll(indentLevel+1);
}

void InterfaceDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        idx = symtab->InsertSymbol(this);
        id->SetDecl(this);
    }
    symtab->BuildScope(this->GetId()->GetIdName());
    members->CheckAll(E_BuildST);
    symtab->ExitScope();
}

void InterfaceDecl::Check(checkT c) {
    switch (c) {
        case E_BuildST:
            this->BuildST(); break;
        case E_CheckDecl:
            expr_type = new NamedType(id);
            expr_type->SetSelfType();
            // fall through.
        default:
            id->Check(c);
            symtab->EnterScope();
            members->CheckAll(c);
            symtab->ExitScope();
    }
}

void InterfaceDecl::Emit() {
    ReportError::Formatted(this->GetLocation(),
            "Interface is not supported by compiler back end yet.");
    Assert(0);
}

FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
    vtable_ofst = -1;
}

void FnDecl::SetFunctionBody(Stmt *b) {
    (body=b)->SetParent(this);
}

void FnDecl::PrintChildren(int indentLevel) {
    if (expr_type) std::cout << " <" << expr_type << ">";
    if (emit_loc) emit_loc->Print();
    if (vtable_ofst != -1)
        std::cout << " ~~[VTable: " << vtable_ofst << "]";
    returnType->Print(indentLevel+1, "(return type) ");
    id->Print(indentLevel+1);
    if (id->GetDecl()) printf(" ........ {def}");
    formals->PrintAll(indentLevel+1, "(formals) ");
    if (body) body->Print(indentLevel+1, "(body) ");
}

void FnDecl::BuildST() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        ReportError::DeclConflict(this, d);
    } else {
        idx = symtab->InsertSymbol(this);
        id->SetDecl(this);
    }
    symtab->BuildScope();
    formals->CheckAll(E_BuildST);
    if (body) body->Check(E_BuildST); // function body must be a StmtBlock.
    symtab->ExitScope();
}

void FnDecl::CheckDecl() {
    returnType->Check(E_CheckDecl);
    id->Check(E_CheckDecl);
    symtab->EnterScope();
    formals->CheckAll(E_CheckDecl);
    if (body) body->Check(E_CheckDecl);
    symtab->ExitScope();

    // check the signature of the main function.
    if (!strcmp(id->GetIdName(), "main")) {
        if (returnType != Type::voidType) {
            ReportError::Formatted(this->GetLocation(),
                    "Return value of 'main' function is expected to be void.");
        }
        if (formals->NumElements() != 0) {
            ReportError::NumArgsMismatch(id, 0, formals->NumElements());
        }
    }

    expr_type = returnType->GetType();
}

void FnDecl::Check(checkT c) {
    switch (c) {
        case E_BuildST:
            this->BuildST(); break;
        case E_CheckDecl:
            this->CheckDecl(); break;
        default:
            returnType->Check(c);
            id->Check(c);
            symtab->EnterScope();
            formals->CheckAll(c);
            if (body) body->Check(c);
            symtab->ExitScope();
    }
}

bool FnDecl::IsEquivalentTo(Decl *other) {
    Assert(this->GetType() && other->GetType());

    if (!other->IsFnDecl()) {
        return false;
    }
    FnDecl *fn = dynamic_cast<FnDecl*>(other);
    if (!returnType->IsEquivalentTo(fn->GetType())) {
        return false;
    }
    if (formals->NumElements() != fn->GetFormals()->NumElements()) {
        return false;
    }
    for (int i = 0; i < formals->NumElements(); i++) {
        // must be VarDecls.
        Type *var_type1 =
            (dynamic_cast<VarDecl*>(formals->Nth(i)))->GetType();
        Type *var_type2 =
            (dynamic_cast<VarDecl*>(fn->GetFormals()->Nth(i)))->GetType();
        if (!var_type1->IsEquivalentTo(var_type2)) {
            return false;
        }
    }
    return true;
}

void FnDecl::AddPrefixToMethods() {
    // add prefix for all functions.
    // Add prefix to all the function name except the global main.
    Decl *d = dynamic_cast<Decl*>(this->GetParent());
    if (d && d->IsClassDecl()) {
        id->AddPrefix(".");
        id->AddPrefix(d->GetId()->GetIdName());
        id->AddPrefix("_");
    } else if (strcmp(id->GetIdName(), "main")) {
        id->AddPrefix("_");
    }
}

void FnDecl::AssignMemberOffset(bool inClass, int offset) {
    vtable_ofst = offset;
}

void FnDecl::Emit() {
    PrintDebug("tac+", "Begin Emitting TAC in FnDecl.");
    if (returnType == Type::doubleType) {
        ReportError::Formatted(this->GetLocation(),
                "Double type is not supported by compiler back end yet.");
        Assert(0);
    }

    Decl *d = dynamic_cast<Decl*>(this->GetParent());
    CG->GenLabel(id->GetIdName());

    // BeginFunc will reset the FP offset counter.
    BeginFunc *f = CG->GenBeginFunc();

    // Add 4 to the offset of the 1st param for class member.
    if (d && d->IsClassDecl()) {
        CG->GetNextParamLoc();
    }

    // Generate all the Locations for formals.
    for (int i = 0; i < formals->NumElements(); i++) {
        VarDecl *v = formals->Nth(i);
        if (v->GetType() == Type::doubleType) {
            ReportError::Formatted(this->GetLocation(),
                    "Double type is not supported by compiler back end yet.");
            Assert(0);
        }
        Location *l = new Location(fpRelative, CG->GetNextParamLoc(),
                v->GetId()->GetIdName());
        v->SetEmitLoc(l);
    }

    if (body) body->Emit();

    // Backpatch the frame size.
    f->SetFrameSize(CG->GetFrameSize());

    CG->GenEndFunc();
}

