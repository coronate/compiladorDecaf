/* File: ast.cc
 * ------------
 *
 * Author: Deyuan Guo
 */

#include <stdio.h>  // printf
#include <string.h> // strdup
#include "ast.h"
#include "ast_decl.h"
#include "ast_type.h"
#include "errors.h"

// the global code generator class.
CodeGenerator *CG = new CodeGenerator();

Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
}

/* The Print method is used to print the parse tree nodes.
 * If this node has a location (most nodes do, but some do not), it
 * will first print the line number to help you match the parse tree
 * back to the source text. It then indents the proper number of levels
 * and prints the "print name" of the node. It then will invoke the
 * virtual function PrintChildren which is expected to print the
 * internals of the node (itself & children) as appropriate.
 */
void Node::Print(int indentLevel, const char *label) {
    const int numSpaces = 3;
    printf("\n");
    if (GetLocation())
        printf("%*d", numSpaces, GetLocation()->first_line);
    else
        printf("%*s", numSpaces, "");
    printf("%*s%s%s: ", indentLevel*numSpaces, "",
           label? label : "", GetPrintNameForNode());
    PrintChildren(indentLevel);
}

Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
}

void Identifier::PrintChildren(int indentLevel) {
    printf("%s", name);
    if (decl) printf(" ---------------- {%d}", decl->GetIndex());
}

void Identifier::CheckDecl() {
    Decl *d = symtab->Lookup(this);
    if (d == NULL) {
        ReportError::IdentifierNotDeclared(this, LookingForVariable);
    } else {
        this->SetDecl(d);
    }
}

void Identifier::Check(checkT c) {
    if (c == E_CheckDecl) {
        this->CheckDecl();
    }
}

bool Identifier::IsEquivalentTo(Identifier *other) {
    bool eq = false;
    if (!strcmp(name, other->GetIdName())) eq = true;
    return eq;
}

void Identifier::Emit() {
    if (decl)
        emit_loc = decl->GetEmitLoc();
}

void Identifier::AddPrefix(const char *prefix) {
    char *s = (char *)malloc(strlen(name) + strlen(prefix) + 1);
    sprintf(s, "%s%s", prefix, name);
    name = s;
}

