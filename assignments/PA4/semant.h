#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <iostream>  
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"
#include <vector>
#include <string>
#include <algorithm>

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

class ClassTable {
private:
  int semant_errors;
  ostream& error_stream;
  bool isDAG(int intId);
  bool redefinedAttributes(int &redefClassId,int &redefFeatureId);

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);
  std::vector<std::vector<int> > classGraph;
  int rootId;
  std::vector<std::vector<Symbol> > attrStruct;
  std::vector<std::string> classesList;
  std::vector<std::string> parentsList;
  //The three components of the environment
  SymbolTable<Symbol, Entry> objectEnv;
  SymbolTable<Symbol, SymbolTable<Symbol,std::vector<Symbol> > > methodEnv;
  std::vector<Symbol> classEnv;
  Class_ currentClass;
  SymbolTable<Symbol,SymbolTable<Symbol,Entry> > attrEnv;
  //Helper variable for setting up the method environment
  SymbolTable<Symbol,std::vector<Symbol> >* thisMethods;
  SymbolTable<Symbol,Entry>* thisAttr;
  //Type checking helper methods
  bool AconformsToB(Symbol a, Symbol b);
  Symbol leastCommonAncestor(Symbol a, Symbol b);
  std::vector<int> dfsPath(int x);
  int lastCommonElement(std::vector<int> &a, std::vector<int> &b);
  int hasSameAttr(int x, int y);
  bool isSameSymbol(Symbol a, Symbol b);
  int findClassNameInList(std::string className,std::vector<std::string> &list);
  Symbol getTypeFromHierarchy(Symbol currentClass, Symbol attrName);
};


#endif

