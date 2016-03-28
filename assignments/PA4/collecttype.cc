
#include "copyright.h"

#include "cool.h"
#include "tree.h"
#include "cool-tree.h"
#include "utilities.h"

void program_class::collectTypes()
{
   //DFS traversal through AST and gather all object names
    for(int i = classes->first(); classes->more(i); i = classes->next(i))
    {
      methodEnv->enterscope();
      objectEnv->enterscope();
      classes->nth(i)->collectTypes();
      methodEnv->exitscope();
      objectEnv->exitscope();
    }
}

void class__class::collectTypes()
{
  classEnv.push_back(idtable.add_string(name));
  for(int i = features->first(); features->more(i); i = features->next(i))
  {
    methodEnv->enterscope();
    objectEnv->enterscope();
    features->nth(i)->collectTypes();
    objectEnv->exitscope();
    methodEnv->exitscope();
  }
}

void method_class::collectTypes()
{
  Formal formal;
  SymbolTable<Symbol,std::vector<Symbol> > table;
  vector<Symbol> v;
  for(int i=formals->first();formals->more(i);i=formals->next(i))
  {
    formal = formals->nth(i);
    Symbol type = formal->getType();
    v.push_back(type);
  }
  v.push_back(return_type);
  table.addid(name,v);
  methodEnv.addid(classEnv[classEnv.size()-1],table);
  for(int i = formals->first(); formals->more(i); i = formals->next(i))
     formals->nth(i)->collectTypes();
   expr->collectTypes();
}

void attr_class::collectTypes()
{
  objectEnv->addid(name,type_decl);
  init->collectTypes();
}

void formal_class::collectTypes()
{
  objectEnv->addid(name,type_decl);
}

void branch_class::collectTypes()
{
  objectEnv->addid(name,type_decl);
  expr->collectTypes();
}

void assign_class::collectTypes()
{
   expr->collectTypes();
}

void static_dispatch_class::collectTypes()
{
   expr->collectTypes();
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->collectTypes();
}

void dispatch_class::collectTypes()
{
   expr->collectTypes();
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->collectTypes();
}

void cond_class::collectTypes()
{
   pred->collectTypes();
   then_exp->collectTypes();
   else_exp->collectTypes();
}

void loop_class::collectTypes()
{
  pred->collectTypes();
  body->collectTypes();
}

void typcase_class::collectTypes()
{
   expr->collectTypes();
   for(int i = cases->first(); cases->more(i); i = cases->next(i))
     cases->nth(i)->collectTypes();
}

void block_class::collectTypes()
{
   for(int i = body->first(); body->more(i); i = body->next(i))
     body->nth(i)->collectTypes();
}

void let_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_let\n";
   dump_Symbol(stream, n+2, identifier);
   dump_Symbol(stream, n+2, type_decl);
   init->dump_with_types(stream, n+2);
   body->dump_with_types(stream, n+2);
   dump_type(stream,n);
}

void plus_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_plus\n";
   e1->dump_with_types(stream, n+2);
   e2->dump_with_types(stream, n+2);
   dump_type(stream,n);
}

void sub_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_sub\n";
   e1->dump_with_types(stream, n+2);
   e2->dump_with_types(stream, n+2);
   dump_type(stream,n);
}

void mul_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_mul\n";
   e1->dump_with_types(stream, n+2);
   e2->dump_with_types(stream, n+2);
   dump_type(stream,n);
}

void divide_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_divide\n";
   e1->dump_with_types(stream, n+2);
   e2->dump_with_types(stream, n+2);
   dump_type(stream,n);
}

void neg_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_neg\n";
   e1->dump_with_types(stream, n+2);
   dump_type(stream,n);
}

void lt_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_lt\n";
   e1->dump_with_types(stream, n+2);
   e2->dump_with_types(stream, n+2);
   dump_type(stream,n);
}


void eq_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_eq\n";
   e1->dump_with_types(stream, n+2);
   e2->dump_with_types(stream, n+2);
   dump_type(stream,n);
}

void leq_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_leq\n";
   e1->dump_with_types(stream, n+2);
   e2->dump_with_types(stream, n+2);
   dump_type(stream,n);
}

void comp_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_comp\n";
   e1->dump_with_types(stream, n+2);
   dump_type(stream,n);
}

void int_const_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_int\n";
   dump_Symbol(stream, n+2, token);
   dump_type(stream,n);
}

void bool_const_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_bool\n";
   dump_Boolean(stream, n+2, val);
   dump_type(stream,n);
}

void string_const_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_string\n";
   stream << pad(n+2) << "\"";
   print_escaped_string(stream,token->get_string());
   stream << "\"\n";
   dump_type(stream,n);
}

void new__class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_new\n";
   dump_Symbol(stream, n+2, type_name);
   dump_type(stream,n);
}

void isvoid_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_isvoid\n";
   e1->dump_with_types(stream, n+2);
   dump_type(stream,n);
}

void no_expr_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_no_expr\n";
   dump_type(stream,n);
}

void object_class::collectTypes()
{
   dump_line(stream,n,this);
   stream << pad(n) << "_object\n";
   dump_Symbol(stream, n+2, name);
   dump_type(stream,n);
}

