

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"

extern int semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

//Kahn's algorithm
bool ClassTable::isDAG(int intId)
{
  std::vector<int> noinc; //set of vertices with no incoming edges
  for(int i=0;i<intId;i++) noinc.push_back(i);
  std::vector<int> v;
  int nodeId;
  //Loop through the nodes
  for(int i=0;i<intId;i++)
  {
    v = classGraph[i];
    //Loop through this node's edges
    for(unsigned int j=0;j<v.size();j++)
    {
      nodeId = v[j];
      //nodeId has at least one incoming edge => remove it from noinc
      for(unsigned int k=0;k<noinc.size();k++)
      {
	if (noinc[k]==nodeId)
	{
	  noinc.erase(noinc.begin()+k);
	  break;
	}
      }
    }
  }
  if (noinc.size()>1) return false; //the DAG should have exactly one root node only
  //Clone classGraph
  std::vector<std::vector<int> > classGraphCopy;
  for(int i=0;i<intId;i++)
  {
    std::vector<int> vals;
    classGraphCopy.push_back(vals);
  }
  std::vector<int> originalVect;
  // fill in classGraphCopy
  for(int i=0;i<intId;i++)
  {
    originalVect = classGraph[i];
    std::vector<int> copyVect(originalVect);
    classGraphCopy[i]=copyVect;
  }
  std::vector<int> endpoints;
  std::vector<int> vect;
  int nextNode;
  bool found;
  while(noinc.size()>0)
  {
    nodeId = noinc[0];
    noinc.erase(noinc.begin());
    endpoints = classGraphCopy[nodeId];
    //Loop through all the next nodes
    for(unsigned int i=0;i<endpoints.size();i++)
    {
      nextNode = endpoints[i];
      found=false;
      //check if nextNode has no further incoming edges
      for(int j=0;(j<intId) && (j!=nodeId);j++)
      {
	vect = classGraphCopy[j];
	for(unsigned int k=0;k<vect.size();k++)
	{
	  if (vect[k]==nextNode)
	  {
	    found=true;
	    break;
	  }
	}
	if (!found) noinc.push_back(nextNode);
      }
    }
    //remove all next node edges
    endpoints.clear();
    classGraphCopy[nodeId]=endpoints;
  }
  //Check whether the cloned graph is now empty
  int nNodes=0;
  for(int i=0;i<intId;i++) nNodes += classGraphCopy[i].size();
  return (nNodes==0);
}

//No hash table support before C++11 resort to simple array and slower O(N) access
int ClassTable::findClassNameInList(std::string className,std::vector<std::string> &list)
{
  for(unsigned int i=0;i<list.size();i++) if (list[i].compare(className)==0) return i;
  return -1;
}

//Check if attributes are redefined a classe's subclasses
bool ClassTable::redefinedAttributes(int nodeId, std::vector<attr_class*> attrList)
{
  //Check if this node's attributes redefine attributes of attrList
  class__class* curClass = classesVect[nodeId];
  Features features = curClass->get_features();
  for(int i=features->first();features->more(i);i=features->next(i))
  {
    Feature feature = features->nth(i);
    if (feature->getNodeType()==1) continue; //this is a method definition => of no interest
    attr_class* attrib = dynamic_cast<attr_class*>(feature);
    //check if this attribute is in attrList already
    for(unsigned int j=0;j<attrList.size();j++)
    {
      Symbol curName = attrList[j]->getName();
      if (curName->equal_string(attrib->getName()->get_string(),attrib->getName()->get_len())==0) return true;
    }
    //Add this node's attributes to the attrList
    attrList.push_back(attrib);
  }
  //DFS recursion
  std::vector<int> children = classGraph[nodeId];
  for(unsigned int i=0;i<children.size();i++) if (redefinedAttributes(children[i],attrList)) return true;
  return false;
}

ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {
  //Iterate through the classes to build the dependency graph
  int /*intId=0,*/classId,parentId,tempId,pvectId; //unique id to assign to every class
  //Initialize empty graph
  for(int i=0;i<classes->len();i++)
  {
    std::vector<int> v;
    classGraph.push_back(v);
  }
  classesVect.resize(classes->len());
  for(int i=classes->first();classes->more(i);i=classes->next(i))
  {
    //Extract the class name and parent class name from each class
    Class_ classPointer = classes->nth(i);
    class__class* classInfoPointer =  dynamic_cast<class__class*>(classPointer);
    Symbol classNameS = classInfoPointer->get_name();
    Symbol classParentS = classInfoPointer->get_parent();
    char* className = classNameS->get_string();
    int classNameLen = classNameS->get_len();
    char* classParent = classParentS->get_string();
    int classParentLen = classParentS->get_len();
    std::string classNameStr(className,classNameLen);
    std::string classParentStr(classParent,classParentLen);
    //Insert the class name and the parent's name into a hash table if they are not yet present
    //Insert the class name
    tempId = findClassNameInList(classNameStr,classesList);
    if (tempId==-1)//key not found => insert key-value pair of class name
    {
      classId = classesList.size();
      classesList.push_back(classNameStr);
      //intId++;
      classesVect.push_back(classInfoPointer);
      //check if this is the root node "Object"
      if (classNameS->equal_string("Object",6)==0) rootId = tempId;
    }
    else
    {
      pvectId = findClassNameInList(classNameStr,parentClasses);
      if (pvectId==-1)
      {
	//Classes may not be redefined
	semant_error(classInfoPointer);
	cerr << error_stream << endl;
	break;
      }
      else
      {
	//Parent class name encountered earlier
	classesVect[tempId]=classInfoPointer;
	//check if this is the root node "Object"
	if (classNameS->equal_string("Object",6)==0) rootId = tempId;
      }
    }
    //Insert the parent class name
    tempId = findClassNameInList(classParentStr,classesList);
    if (tempId==-1)//key not found => insert key-value pair of parent class name
    {
      parentId = classesList.size();
      classesList.push_back(classParentStr);
      parentClasses.push_back(classParentStr);
      //intId++;
    }
    else parentId = tempId;
    //Ids for the class and its parent are in classId and in parentId
    //Insert parentId->classId branch in directed graph
    std::vector<int> v = classGraph[parentId];
    //Check whether the branch exists already in the graph
    bool found=false;
    for(unsigned int j=0;j<v.size();j++)
    {
      if (v[j]==classId)
      {
	found=true;
	break;
      }
    }
    if (!found) //if not found insert the branch
    {
      v.push_back(classId);
      classGraph[parentId] = v;
    }
  }
  if (semant_errors==0)
  {
    //Check if the graph has cycles
    //Topological sort
    bool isOK = isDAG(classesList.size());
    if (!isOK)
    {
      semant_error(dynamic_cast<class__class*>(classes->nth(0)));
      cerr << error_stream << endl;
    }
    else
    {
      //Check that no attributes are redefined in subclasses
      std::vector<attr_class*> attrList;
      if (redefinedAttributes(rootId,attrList))
      {
	semant_error(dynamic_cast<class__class*>(classes->nth(0)));
	cerr << error_stream << endl;
      }
    }
  }
}

Classes install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);
    Classes basicClasses = single_Classes(Object_class);
    basicClasses = append_Classes(basicClasses,single_Classes(IO_class));
    basicClasses = append_Classes(basicClasses,single_Classes(Int_class));
    basicClasses = append_Classes(basicClasses,single_Classes(Bool_class));
    basicClasses = append_Classes(basicClasses,single_Classes(Str_class));
    return basicClasses;
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 

void let_class::collectTypes()
{
  objectEnv->addid(identifier,type_decl);
}

void branch_class::collectTypes()
{
  objectEnv->addid(name,type_decl);
}

void formal_class::collectTypes()
{
  objectEnv->addid(name,type_decl);
}

void attr_class::collectTypes()
{
  objectEnv->addid(name,type_decl);
  init.collectTypes();
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
  expr.collectTypes();
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

/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();
    Classes basicClasses = install_basic_classes();
    classes = append_Classes(basicClasses,classes);
    /* ClassTable constructor may do some semantic analysis */
    ClassTable *classtable = new ClassTable(classes);

    //DFS traversal through AST and gather all object names
    for(int i = classes->first(); classes->more(i); i = classes->next(i))
    {
      methodEnv->enterscope();
      objectEnv->enterscope();
      classes->nth(i)->collectTypes();
      methodEnv->exitscope();
      objectEnv->exitscope();
    }

    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
}
