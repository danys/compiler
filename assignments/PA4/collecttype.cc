
#include "copyright.h"
#include <vector>
#include "cool.h"
#include "tree.h"
#include "cool-tree.h"
#include "utilities.h"

void program_class::collectTypes(environment* env)
{
   //DFS traversal through AST and gather all object names
    for(int i = classes->first(); classes->more(i); i = classes->next(i))
    {
      env->methodEnv->enterscope();
      env->objectEnv->enterscope();
      classes->nth(i)->collectTypes(env);
      env->methodEnv->exitscope();
      env->objectEnv->exitscope();
    }
}

void class__class::collectTypes(environment* env)
{
  env->classEnv->push_back(name);
  for(int i = features->first(); features->more(i); i = features->next(i))
  {
    env->methodEnv->enterscope();
    env->objectEnv->enterscope();
    features->nth(i)->collectTypes(env);
    env->objectEnv->exitscope();
    env->methodEnv->exitscope();
  }
}

void method_class::collectTypes(environment* env)
{
  Formal formal;
  SymbolTable<Symbol,std::vector<Symbol> >* table = 0;
  std::vector<Symbol>* v = 0;
  for(int i=formals->first();formals->more(i);i=formals->next(i))
  {
    formal = formals->nth(i);
    Symbol type = formal->getType();
    v->push_back(type);
  }
  v->push_back(return_type);
  table->addid(name,v);
  env->methodEnv->addid(env->classEnv->at(env->classEnv->size()-1),table);
  for(int i = formals->first(); formals->more(i); i = formals->next(i))
     formals->nth(i)->collectTypes(env);
   expr->collectTypes(env);
}

void attr_class::collectTypes(environment* env)
{
  Symbol* typeD = 0;
  *typeD = type_decl;
  env->objectEnv->addid(name,typeD);
  init->collectTypes(env);
}

void formal_class::collectTypes(environment* env)
{
  Symbol* typeD = 0;
  *typeD = type_decl;
  env->objectEnv->addid(name,typeD);
}

void branch_class::collectTypes(environment* env)
{
  Symbol* typeD = 0;
  *typeD = type_decl;
  env->objectEnv->addid(name,typeD);
  expr->collectTypes(env);
}

void assign_class::collectTypes(environment* env)
{
   expr->collectTypes(env);
}

void static_dispatch_class::collectTypes(environment* env)
{
   expr->collectTypes(env);
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->collectTypes(env);
}

void dispatch_class::collectTypes(environment* env)
{
   expr->collectTypes(env);
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->collectTypes(env);
}

void cond_class::collectTypes(environment* env)
{
   pred->collectTypes(env);
   then_exp->collectTypes(env);
   else_exp->collectTypes(env);
}

void loop_class::collectTypes(environment* env)
{
  pred->collectTypes(env);
  body->collectTypes(env);
}

void typcase_class::collectTypes(environment* env)
{
   expr->collectTypes(env);
   for(int i = cases->first(); cases->more(i); i = cases->next(i))
     cases->nth(i)->collectTypes(env);
}

void block_class::collectTypes(environment* env)
{
   for(int i = body->first(); body->more(i); i = body->next(i))
     body->nth(i)->collectTypes(env);
}

void let_class::collectTypes(environment* env)
{
  Symbol* typeD = 0;
  *typeD = type_decl;
  env->objectEnv->addid(identifier,typeD);
  init->collectTypes(env);
  body->collectTypes(env);
}

void plus_class::collectTypes(environment* env)
{
   e1->collectTypes(env);
   e2->collectTypes(env);
}

void sub_class::collectTypes(environment* env)
{
   e1->collectTypes(env);
   e2->collectTypes(env);
}

void mul_class::collectTypes(environment* env)
{
   e1->collectTypes(env);
   e2->collectTypes(env);
}

void divide_class::collectTypes(environment* env)
{
   e1->collectTypes(env);
   e2->collectTypes(env);
}

void neg_class::collectTypes(environment* env)
{
   e1->collectTypes(env);
}

void lt_class::collectTypes(environment* env)
{
   e1->collectTypes(env);
   e2->collectTypes(env);
}


void eq_class::collectTypes(environment* env)
{
   e1->collectTypes(env);
   e2->collectTypes(env);
}

void leq_class::collectTypes(environment* env)
{
   e1->collectTypes(env);
   e2->collectTypes(env);
}

void comp_class::collectTypes(environment* env)
{
   e1->collectTypes(env);
}

void int_const_class::collectTypes(environment* env)
{
   //do nothing
}

void bool_const_class::collectTypes(environment* env)
{
   //do nothing
}

void string_const_class::collectTypes(environment* env)
{
   //do nothing
}

void new__class::collectTypes(environment* env)
{
   //do nothing
}

void isvoid_class::collectTypes(environment* env)
{
   e1->collectTypes(env);
}

void no_expr_class::collectTypes(environment* env)
{
  //do nothing
}

void object_class::collectTypes(environment* env)
{
  //do nothing
}

