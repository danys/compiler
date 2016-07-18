#include <assert.h>
#include <stdio.h>
#include "emit.h"
#include "cool-tree.h"
#include "symtab.h"
#include <vector>
#include <string>
#include <stack>

enum Basicness     {Basic, NotBasic};
#define TRUE 1
#define FALSE 0

class CgenClassTable;
typedef CgenClassTable *CgenClassTableP;

class CgenNode;
class Location;
typedef CgenNode *CgenNodeP;

class CgenClassTable : public SymbolTable<Symbol,CgenNode> {
private:
   List<CgenNode> *nds;
   ostream& str;
   int stringclasstag;
   int intclasstag;
   int boolclasstag;
   int classtagindex; //current class tag index
   std::vector<std::string> classNames;

// The following methods emit code for
// constants and global declarations.

   void code_global_data();
   void code_global_text();
   void code_bools(int);
   void code_select_gc();
   void code_constants();
   void code_class_name_tab();
   void code_prototype_objects();
   void code_class_obj_table();
   void code_class_init_methods();
   void code_class_method_defs();
   void code_dispatch_tables();
   void code_program_methods();

// The following creates an inheritance graph from
// a list of classes.  The graph is implemented as
// a tree of `CgenNode', and class names are placed
// in the base class symbol table.

   void install_basic_classes();
   void install_class(CgenNodeP nd);
   void install_classes(Classes cs);
   void build_inheritance_tree();
   void set_relations(CgenNodeP nd);
   int findClassTag(Symbol sym);
   void setClassTag(CgenNode* node);
   void setClassAttributesAndMethods();
   CgenNode* getDefiningNode(CgenNode* startNode, Symbol funcName);

public:
   CgenClassTable(Classes, ostream& str);
   void code();
   CgenNodeP root();
   CgenNode* getClassByName(Symbol className);
   CgenNode* currentNode;
   Location* getLocationByName(Symbol varName);
};

class Location
{
 private:
  char* reg;
  int offset;
 public:
  Location(char* reg_, int offset_){reg = reg_; offset = offset_;}
  char* getRegister(){return reg;}
  int getOffset(){return offset;}
};

class CgenNode : public class__class {
private: 
   CgenNodeP parentnd;                        // Parent of class
   List<CgenNode> *children;                  // Children of class
   Basicness basic_status;                    // `Basic' if class is basic
                                              // `NotBasic' otherwise
   int classTag;

public:
   CgenNode(Class_ c,
            Basicness bstatus,
            CgenClassTableP class_table);

   void add_child(CgenNodeP child);
   List<CgenNode> *get_children() { return children; }
   void set_parentnd(CgenNodeP p);
   CgenNodeP get_parentnd() { return parentnd; }
   int basic() { return (basic_status == Basic); }
   void setClassTag(int tag){classTag=tag;}
   int getClassTag(){return classTag;}
   void setMethodsAndAttributes(CgenNode* fromObj, bool checkOverride);
   void setMethodsAndAttributesFromParent(CgenNode* fromObj);
   std::vector<Feature> attributes;
   std::vector<Feature> methods;
   SymbolTable<Symbol,Location>* locations; 

   CgenClassTable* classTable;
   int getFeatureOffsetByName(Symbol featureName, bool isAttribute);
   SymbolTable<Symbol,Location>* getLocations(){return locations;};
   void code_init_method(ostream &s);
   virtual void code(ostream &s, CgenClassTable* table);
};

class BoolConst 
{
 private: 
  int val;
 public:
  BoolConst(int);
  void code_def(ostream&, int boolclasstag);
  void code_ref(ostream&) const;
};
