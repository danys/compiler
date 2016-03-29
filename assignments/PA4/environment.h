#include "symtab.h"
#include "stringtab.h"
#include <vector>

struct environment
{
  SymbolTable<Symbol, Symbol>* objectEnv;
  SymbolTable<Symbol, SymbolTable<Symbol,std::vector<Symbol> > >* methodEnv;
  std::vector<Symbol>* classEnv;
};
