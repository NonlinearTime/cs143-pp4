/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp4: You will need to extend the Decl classes to implement
 * code generation for declarations.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "list.h"

class Type;
class NamedType;
class Identifier;
class Stmt;
class CodeGenerator;
class Location;

class Decl : public Node
{
  protected:
    Identifier *id;

  public:
    Decl(Identifier *name);
    friend ostream& operator<<(ostream& out, Decl *d) { return out << d->id; }

    const char* GetName() { return id->GetName(); }

    virtual void BuildScope() = 0;

    // TODO: Make into a pure virtual function
    virtual Location* Emit(CodeGenerator *cg) { return NULL; }
    virtual int GetMemBytes() { return 0; }
};

class VarDecl : public Decl
{
  protected:
    Type *type;
    Location *memLoc;

  public:
    VarDecl(Identifier *name, Type *type);

    Type* GetType() { return type; }

    void BuildScope() { /* Empty */ }

    int GetMemBytes();

    Location* GetMemLoc() { return memLoc; }
    void SetMemLoc(Location *m) { memLoc = m; }
};

class ClassDecl : public Decl
{
  protected:
    List<Decl*> *members;
    NamedType *extends;
    List<NamedType*> *implements;

  public:
    ClassDecl(Identifier *name, NamedType *extends,
              List<NamedType*> *implements, List<Decl*> *members);

    NamedType* GetType();
    NamedType* GetExtends() { return extends; }

    void BuildScope();
};

class InterfaceDecl : public Decl
{
  protected:
    List<Decl*> *members;

  public:
    InterfaceDecl(Identifier *name, List<Decl*> *members);

    void BuildScope();
};

class FnDecl : public Decl
{
  protected:
    List<VarDecl*> *formals;
    Type *returnType;
    Stmt *body;

  public:
    FnDecl(Identifier *name, Type *returnType, List<VarDecl*> *formals);
    void SetFunctionBody(Stmt *b);

    Type* GetType() { return returnType; }

    void BuildScope();
    Location* Emit(CodeGenerator *cg);
};

#endif
