/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "codegen.h"

Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this);
    scope = new Scope;
}

VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

int VarDecl::GetMemBytes() {
    // TODO: Update to handle object sizes
    return CodeGenerator::VarSize;
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}

NamedType* ClassDecl::GetType() {
    return new NamedType(id);
}

void ClassDecl::BuildScope() {
    for (int i = 0, n = members->NumElements(); i < n; ++i)
        scope->AddDecl(members->Nth(i));

    for (int i = 0, n = members->NumElements(); i < n; ++i)
        members->Nth(i)->BuildScope();
}

Location* ClassDecl::Emit(CodeGenerator *cg) {
    int memOffset = CodeGenerator::OffsetToFirstField;
    int vtblOffset = CodeGenerator::OffsetToFirstMethod;

    if (extends != NULL) {
        Decl *d = Program::gScope->table->Lookup(extends->GetName());
        Assert(d != NULL);
        memOffset += d->GetMemBytes();
        vtblOffset += d->GetVTblBytes();
    }

    for (int i = 0, n = members->NumElements(); i < n; ++i) {
        VarDecl *d = dynamic_cast<VarDecl*>(members->Nth(i));
        if (d == NULL)
            continue;
        d->SetMemOffset(memOffset);
        memOffset += d->GetMemBytes();

    }

    for (int i = 0, n = members->NumElements(); i < n; ++i) {
        FnDecl *d = dynamic_cast<FnDecl*>(members->Nth(i));
        if (d == NULL)
            continue;
        d->SetVTblOffset(vtblOffset);
        vtblOffset += CodeGenerator::VarSize;
    }

    for (int i = 0, n = members->NumElements(); i < n; ++i) {
        std::string prefix;
        prefix += GetName();
        prefix += ".";

        Decl *d = members->Nth(i);
        d->AddLabelPrefix(prefix.c_str());
        d->Emit(cg);
    }

    cg->GenVTable(GetName(), GetMethodLabels());

    return NULL;
}

int ClassDecl::GetMemBytes() {
    int memBytes = 0;

    if (extends != NULL) {
        Decl *d = Program::gScope->table->Lookup(extends->GetName());
        Assert(d != NULL);
        memBytes += d->GetMemBytes();
    }

    for (int i = 0, n = members->NumElements(); i < n; ++i)
        memBytes += members->Nth(i)->GetMemBytes();

    return memBytes;
}

int ClassDecl::GetVTblBytes() {
    int vtblBytes = 0;

    if (extends != NULL) {
        Decl *d = Program::gScope->table->Lookup(extends->GetName());
        Assert(d != NULL);
        vtblBytes += d->GetVTblBytes();
    }

    for (int i = 0, n = members->NumElements(); i < n; ++i)
        vtblBytes += members->Nth(i)->GetVTblBytes();

    return vtblBytes;
}

List<const char*>* ClassDecl::GetMethodLabels() {
    /* TODO: Implement polymorphic behaviour. Currently, the base class's
     * method will be called even if the object is actually of a derived type.
     */

    List<const char*> *labels = new List<const char*>;

    if (extends != NULL) {
        Decl *d = Program::gScope->table->Lookup(extends->GetName());
        ClassDecl *c = dynamic_cast<ClassDecl*>(d);
        Assert(d != NULL);
        List<const char*> *extLabels = c->GetMethodLabels();
        for (int i = 0, n = extLabels->NumElements(); i < n; ++i)
            labels->Append(strdup(extLabels->Nth(i)));
    }

    for (int i = 0, n = members->NumElements(); i < n; ++i) {
        FnDecl *d = dynamic_cast<FnDecl*>(members->Nth(i));
        if (d == NULL)
            continue;
        labels->Append(d->GetLabel());
    }

    return labels;
}

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

void InterfaceDecl::BuildScope() {
    for (int i = 0, n = members->NumElements(); i < n; ++i)
        scope->AddDecl(members->Nth(i));

    for (int i = 0, n = members->NumElements(); i < n; ++i)
        members->Nth(i)->BuildScope();
}

FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
    label = new std::string(GetName());
}

void FnDecl::SetFunctionBody(Stmt *b) {
    (body=b)->SetParent(this);
}

const char* FnDecl::GetLabel() {
    return label->c_str();
}

bool FnDecl::HasReturnVal() {
    return returnType == Type::voidType ? 0 : 1;
}

void FnDecl::BuildScope() {
    for (int i = 0, n = formals->NumElements(); i < n; ++i)
        scope->AddDecl(formals->Nth(i));

    for (int i = 0, n = formals->NumElements(); i < n; ++i)
        formals->Nth(i)->BuildScope();

    if (body) body->BuildScope();
}

Location* FnDecl::Emit(CodeGenerator *cg) {
    int offset = CodeGenerator::OffsetToFirstParam;
    for (int i = 0, n = formals->NumElements(); i < n; ++i) {
        VarDecl *d = formals->Nth(i);
        Location *loc = new Location(fpRelative, offset, d->GetName());
        d->SetMemLoc(loc);
        offset += d->GetMemBytes();
    }

    if (body != NULL) {
        cg->GenLabel(GetLabel());
        cg->GenBeginFunc()->SetFrameSize(body->GetMemBytes());
        body->Emit(cg);
        cg->GenEndFunc();
    }

    return NULL;
}

int FnDecl::GetVTblBytes() {
    return CodeGenerator::VarSize;
}

void FnDecl::AddLabelPrefix(const char *p) {
    label->insert(0, p);
}
