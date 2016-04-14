

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

bool ClassTable::AconformsToB(Symbol a, Symbol b)
{
  //a is b or one of its descendants
  std::string astr(a->get_string(),a->get_len());
  std::string bstr(a->get_string(),b->get_len());
  int aId = findClassNameInList(astr,classesList);
  if (aId==-1)
  {
    cerr << "Error finding " << astr << " class to check conformance with " << bstr << "class." << endl;
    return false;
  }
  int bId = findClassNameInList(bstr,classesList);
  if (bId==-1)
  {
    cerr << "Error finding " << bstr << " class to check if " << astr << "class conforms to it." << endl;
    return false;
  }
  //Find aId starting from bId
  std::vector<int> stack = classGraph[bId];
  int t; //current node id
  std::vector<int> vtemp;
  while(stack.size()>0)
  {
    //pop int id from stack
    t = stack.back();
    stack.pop_back();
    if (t == aId) return true;
    //retrieve vector for int id
    vtemp = classGraph[t];
    //add vect to stack
    stack.insert( stack.end(), vtemp.begin(), vtemp.end());
  }
  //if aId has not yet been found => aId does not conform to bId
  return false;
}

std::vector<int> ClassTable::dfsPath(int x)
{
  std::vector<int> res;
  std::vector<int> stack;
  stack.push_back(rootId);
  int curNode;
  while(stack.size()>0)
  {
    curNode = stack.back();
    res.push_back(curNode);
    if (curNode==x) break;
    stack.pop_back();
    std::vector<int> v = classGraph[curNode];
    for(unsigned int i=0;i<v.size();i++) stack.push_back(v[i]);
  }
  return res;
}

int ClassTable::lastCommonElement(std::vector<int> &a, std::vector<int> &b)
{
  int minLen = (a.size()<=b.size()) ? a.size() : b.size();
  int last = -1;
  while((last+1<=minLen-1) && (a[last+1]==b[last+1])) last++;
  return last;
}

Symbol ClassTable::leastCommonAncestor(Symbol a, Symbol b)
{
  std::string astr(a->get_string(),a->get_len());
  int aId = findClassNameInList(astr,classesList);
  std::string bstr(a->get_string(),b->get_len());
  if (aId==-1)
  {
    cerr << "Error finding " << astr << " class to find least common ancestor." << endl;
    return false;
  }
  int bId = findClassNameInList(bstr,classesList);
  if (bId==-1)
  {
    cerr << "Error finding " << bstr << " class to find least common ancestor." << endl;
    return false;
  }
  std::vector<int> aPath = dfsPath(aId);
  std::vector<int> bPath = dfsPath(bId);
  int ancestorId = lastCommonElement(aPath,bPath);
  return idtable.add_string(const_cast<char*>(classesList[ancestorId].c_str()),classesList[ancestorId].size());
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
      cerr << "Inheritance graph has cycles." << endl;
      semant_error();
    }
    else
    {
      //Check that no attributes are redefined in subclasses
      std::vector<attr_class*> attrList;
      if (redefinedAttributes(rootId,attrList))
      {
	cerr << "Attributes are redefined in subclass." << endl;
	semant_error();
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

//First DFS pass that collects declared types
void program_class::collectTypes(ClassTable* classtable)
{
    classtable->methodEnv.enterscope();
    classtable->attrEnv.enterscope();
    for(int i = classes->first(); classes->more(i); i = classes->next(i))
    {
      classes->nth(i)->collectTypes(classtable);
    }
}

void class__class::collectTypes(ClassTable* classtable)
{
  classtable->classEnv.push_back(name);
  classtable->thisMethods = new SymbolTable<Symbol, std::vector<Symbol> >();
  classtable->thisAttr = new SymbolTable<Symbol, Symbol>();
  for(int i = features->first(); features->more(i); i = features->next(i))
  {
    features->nth(i)->collectTypes(classtable);
  }
  classtable->methodEnv.addid(name,classtable->thisMethods);
  classtable->attrEnv.addid(name,classtable->thisAttr);
}

void method_class::collectTypes(ClassTable* classtable)
{
  for(int i = formals->first(); formals->more(i); i = formals->next(i))
     formals->nth(i)->collectTypes(classtable);
  expr->collectTypes(classtable);
  Formal formal;
  std::vector<Symbol>* v = new std::vector<Symbol>();
  for(int i=formals->first();formals->more(i);i=formals->next(i))
  {
    formal = formals->nth(i);
    Symbol type = formal->getType();
    v->push_back(type);
  }
  v->push_back(return_type);
  classtable->thisMethods->addid(name,v);
}

void attr_class::collectTypes(ClassTable* classtable)
{
  Symbol* typeD = 0;
  *typeD = type_decl;
  classtable->thisAttr->addid(name,typeD);
  init->collectTypes(classtable);
}

void formal_class::collectTypes(ClassTable* classtable)
{
  //nothing to do
}

void branch_class::collectTypes(ClassTable* classtable)
{
  expr->collectTypes(classtable);
}

void assign_class::collectTypes(ClassTable* classtable)
{
   expr->collectTypes(classtable);
}

void static_dispatch_class::collectTypes(ClassTable* classtable)
{
   expr->collectTypes(classtable);
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->collectTypes(classtable);
}

void dispatch_class::collectTypes(ClassTable* classtable)
{
   expr->collectTypes(classtable);
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->collectTypes(classtable);
}

void cond_class::collectTypes(ClassTable* classtable)
{
   pred->collectTypes(classtable);
   then_exp->collectTypes(classtable);
   else_exp->collectTypes(classtable);
}

void loop_class::collectTypes(ClassTable* classtable)
{
  pred->collectTypes(classtable);
  body->collectTypes(classtable);
}

void typcase_class::collectTypes(ClassTable* classtable)
{
   expr->collectTypes(classtable);
   for(int i = cases->first(); cases->more(i); i = cases->next(i))
     cases->nth(i)->collectTypes(classtable);
}

void block_class::collectTypes(ClassTable* classtable)
{
   for(int i = body->first(); body->more(i); i = body->next(i))
     body->nth(i)->collectTypes(classtable);
}

void let_class::collectTypes(ClassTable* classtable)
{
  init->collectTypes(classtable);
  body->collectTypes(classtable);
}

void plus_class::collectTypes(ClassTable* classtable)
{
   e1->collectTypes(classtable);
   e2->collectTypes(classtable);
}

void sub_class::collectTypes(ClassTable* classtable)
{
   e1->collectTypes(classtable);
   e2->collectTypes(classtable);
}

void mul_class::collectTypes(ClassTable* classtable)
{
   e1->collectTypes(classtable);
   e2->collectTypes(classtable);
}

void divide_class::collectTypes(ClassTable* classtable)
{
   e1->collectTypes(classtable);
   e2->collectTypes(classtable);
}

void neg_class::collectTypes(ClassTable* classtable)
{
   e1->collectTypes(classtable);
}

void lt_class::collectTypes(ClassTable* classtable)
{
   e1->collectTypes(classtable);
   e2->collectTypes(classtable);
}


void eq_class::collectTypes(ClassTable* classtable)
{
   e1->collectTypes(classtable);
   e2->collectTypes(classtable);
}

void leq_class::collectTypes(ClassTable* classtable)
{
   e1->collectTypes(classtable);
   e2->collectTypes(classtable);
}

void comp_class::collectTypes(ClassTable* classtable)
{
   e1->collectTypes(classtable);
}

void int_const_class::collectTypes(ClassTable* classtable)
{
   //do nothing
}

void bool_const_class::collectTypes(ClassTable* classtable)
{
   //do nothing
}

void string_const_class::collectTypes(ClassTable* classtable)
{
   //do nothing
}

void new__class::collectTypes(ClassTable* classtable)
{
   //do nothing
}

void isvoid_class::collectTypes(ClassTable* classtable)
{
   e1->collectTypes(classtable);
}

void no_expr_class::collectTypes(ClassTable* classtable)
{
  //do nothing
}

void object_class::collectTypes(ClassTable* classtable)
{
  //do nothing
}

//2nd pass: infer types
void program_class::inferTypes(ClassTable* classtable)
{
    classtable->classEnv.clear();
    for(int i = classes->first(); classes->more(i); i = classes->next(i))
    {
      classes->nth(i)->inferTypes(classtable);
    }
}

void class__class::inferTypes(ClassTable* classtable)
{
  classtable->classEnv.push_back(name);
  objectEnv = attrEnv.lookup(name);
  for(int i = features->first(); features->more(i); i = features->next(i))
  {
    features->nth(i)->inferTypes(classtable);
  }
}

void method_class::inferTypes(ClassTable* classtable)
{
  //TODO
  /* Symbol tType;
  if (type_decl.equal_string("SELF_TYPE",9)==0) tType = classEnv.back();
  else tType = type_decl;*/
}

void attr_class::inferTypes(ClassTable* classtable)
{
  Symbol* typeD = 0;
  *typeD = type_decl;
  classtable->objectEnv.addid(name,typeD);
  init->inferTypes(classtable);
}

void formal_class::inferTypes(ClassTable* classtable)
{
  Symbol* typeD = 0;
  *typeD = type_decl;
  classtable->objectEnv.addid(name,typeD);
}

void branch_class::inferTypes(ClassTable* classtable)
{
  expr->inferTypes(classtable);
  set_type(expr->get_type());
}

void assign_class::inferTypes(ClassTable* classtable)
{
   expr->inferTypes(classtable);
   //OK now expr e1 should have a type t'
   Symbol tprimeType = expr->get_type();
   //Check if tprimtTpye is <= type of name
   Symbol* t = classtable->objectEnv.lookup(name);
   if (t==NULL)
   {
    cerr << "Type declaration for object not found anywhere." << endl;
    classtable->semant_error();
    *t = Object; //default to object type
   }
   if (!classtable->AconformsToB(tprimeType, *t))
   {
     cerr << "Assign class error: Assigned type does not conform to type of variable." << endl;
    classtable->semant_error();
   }
}

void static_dispatch_class::inferTypes(ClassTable* classtable)
{
   expr->inferTypes(classtable);
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->inferTypes(classtable);
   //Get the class from expr as a Symbol
   Symbol className = expr->get_type();
   if (!classtable->AconformsToB(className,type_name))
   {
     cerr << "Static dispatch class does not conform to given class T" << endl;
     classtable->semant_error();
   }
   SymbolTable<Symbol,std::vector<Symbol> >* methodsList = classtable->methodEnv.lookup(type_name);
   std::vector<Symbol> methodArgs = *(methodsList->lookup(name));
   //Check that all actual types conform to the types of methodArgs
   for(unsigned int i=0;i<methodArgs.size()-1;i++)
   {
     if (!classtable->AconformsToB(actual->nth(i)->get_type(),methodArgs[i]))
     {
       classtable->semant_error();
       cerr << "Actual types of args of dynamic dispatch method do not conform to declared types" << endl;
       break;
     }
   }
   //Set the return type
   if (methodArgs.back()->equal_string("SELF_TYPE",9)==0) set_type(className);
   else set_type(methodArgs.back());
}

void dispatch_class::inferTypes(ClassTable* classtable)
{
   expr->inferTypes(classtable);
   for(int i = actual->first(); actual->more(i); i = actual->next(i))
     actual->nth(i)->inferTypes(classtable);
   //Get the class from expr as a Symbol
   Symbol className = expr->get_type();
   Symbol t0prime;
   if (className->equal_string("SELF_TYPE",9)==0) t0prime = classtable->classEnv.back();
   else t0prime = className;
   SymbolTable<Symbol,std::vector<Symbol> >* methodsList = classtable->methodEnv.lookup(t0prime);
   std::vector<Symbol> methodArgs = *(methodsList->lookup(name));
   //Check that all actual types conform to the types of methodArgs
   for(unsigned int i=0;i<methodArgs.size()-1;i++)
   {
     if (!classtable->AconformsToB(actual->nth(i)->get_type(),methodArgs[i]))
     {
       classtable->semant_error();
       cerr << "Actual types of args of dynamic dispatch method do not conform to declared types" << endl;
       break;
     }
   }
   //Set the return type
   if (methodArgs.back()->equal_string("SELF_TYPE",9)==0) set_type(className);
   else set_type(methodArgs.back());
}

void cond_class::inferTypes(ClassTable* classtable)
{
   pred->inferTypes(classtable);
   then_exp->inferTypes(classtable);
   else_exp->inferTypes(classtable);
   if (pred->get_type()->equal_string("Bool",4)!=0) cerr << "If predicate does not have type Bool!" << endl;
   set_type(classtable->leastCommonAncestor(then_exp->get_type(),else_exp->get_type()));
}

void loop_class::inferTypes(ClassTable* classtable)
{
  pred->inferTypes(classtable);
  body->inferTypes(classtable);
  if (pred->get_type()->equal_string("Bool",4)!=0)
  {
    cerr << "Loop predicate must have type Bool!" << endl;
    classtable->semant_error();
  }
  set_type(Object);
}

void typcase_class::inferTypes(ClassTable* classtable)
{
   expr->inferTypes(classtable);
   for(int i = cases->first(); cases->more(i); i = cases->next(i))
     cases->nth(i)->inferTypes(classtable);
   Symbol temp;
   for(int i = cases->first(); cases->more(i); i = cases->next(i))
   {
     if (i==0) temp = cases->nth(i)->get_type();
     else temp = classtable->leastCommonAncestor(temp,cases->nth(i)->get_type());
   }
   set_type(temp);
}

void block_class::inferTypes(ClassTable* classtable)
{
   for(int i = body->first(); body->more(i); i = body->next(i))
     body->nth(i)->inferTypes(classtable);
   Expression exp = body->nth(body->len()-1);
   set_type(exp->get_type());
}

void let_class::inferTypes(ClassTable* classtable)
{
  Symbol* typeD = 0;
  *typeD = type_decl;
  classtable->objectEnv.addid(identifier,typeD);
  init->inferTypes(classtable);
  body->inferTypes(classtable);
  Symbol t0prime;
  if (type_decl->equal_string("SELF_TYPE",9)==0) t0prime = classtable->classEnv.back();
  else t0prime = type_decl;
  if (init->isNULL()==false)
  {
    Symbol e1 = init->get_type();
    if (!classtable->AconformsToB(e1,t0prime))
    {
      cerr << "Let initialization does not conform to expected type!" << endl;
      classtable->semant_error();
    }
  }
  set_type(body->get_type());
}

void plus_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   e2->inferTypes(classtable);
   Symbol t1 = e1->get_type();
   Symbol t2 = e2->get_type();
   if (t1->equal_string("Int",3)!=0)
   {
     cerr << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   if (t2->equal_string("Int",3)!=0)
   {
     cerr << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   set_type(Int);
}

void sub_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   e2->inferTypes(classtable);
   Symbol t1 = e1->get_type();
   Symbol t2 = e2->get_type();
   if (t1->equal_string("Int",3)!=0)
   {
     cerr << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   if (t2->equal_string("Int",3)!=0)
   {
     cerr << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   set_type(Int);
}

void mul_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   e2->inferTypes(classtable);
   Symbol t1 = e1->get_type();
   Symbol t2 = e2->get_type();
   if (t1->equal_string("Int",3)!=0)
   {
     cerr << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   if (t2->equal_string("Int",3)!=0)
   {
     cerr << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   set_type(Int);
}

void divide_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   e2->inferTypes(classtable);
   Symbol t1 = e1->get_type();
   Symbol t2 = e2->get_type();
   if (t1->equal_string("Int",3)!=0)
   {
     cerr << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   if (t2->equal_string("Int",3)!=0)
   {
     cerr << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   set_type(Int);
}

void neg_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   if (!e1->get_type()->equal_string("Int",3))
   {
     classtable->semant_error();
     cerr << "Neg class should have Int type" << endl;
   }
   set_type(Int);
}

void lt_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   e2->inferTypes(classtable);
   if ((e1->get_type()->equal_string("Int",3)!=0) && (e2->get_type()->equal_string("Int",3)!=0))
   {
     cerr << "Leq comparison operators must have type Int!" << endl;
     classtable->semant_error();
   }
   set_type(Bool);
}


void eq_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   e2->inferTypes(classtable);
   Symbol eqtype = e1->get_type();
   if ((eqtype->equal_string("Int",3)!=0) && (eqtype->equal_string("String",6)!=0) && (eqtype->equal_string("Bool",4)!=0))
   {
     cerr << "Eq class needs to have operands of type Int, String or Bool!" << endl;
     classtable->semant_error();
   }
   else
   {
     if (e2->get_type()->equal_string(eqtype->get_string(),eqtype->get_len())!=0)
     {
       cerr << "Eq operands must have the same type!" << endl;
       classtable->semant_error();
     }
   }
   set_type(Bool);
}

void leq_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   e2->inferTypes(classtable);
   if ((e1->get_type()->equal_string("Int",3)!=0) && (e2->get_type()->equal_string("Int",3)!=0))
   {
     cerr << "Leq comparison operators must have type Int!" << endl;
     classtable->semant_error();
   }
   set_type(Bool);
}

void comp_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   if (e1->get_type()->equal_string("Bool",4)!=0)
   {
     cerr << "Not operand must have type Bool!" << endl;
     classtable->semant_error();
   }
   set_type(Bool);
}

void int_const_class::inferTypes(ClassTable* classtable)
{
  //Check that token is an int
  std::string tokenstr(token->get_string(),token->get_len());
  bool ok = true;
  for(unsigned int i=0;i<tokenstr.size();i++)
  {
    if (((int)tokenstr[i]<48) || ((int)tokenstr[i]>57))
    {
      ok = false;
      break;
    }
  }
  if (!ok)
  {
    cerr << "Identifier is no integer default to 0" << endl;
    setToken(idtable.add_string("0")); //set token to zero
  }
  set_type(Int);
}

void bool_const_class::inferTypes(ClassTable* classtable)
{
   set_type(Bool);
}

void string_const_class::inferTypes(ClassTable* classtable)
{
  //Check that token is a string
  //Differentiate between IO and String class here?
  set_type(Str);
}

void new__class::inferTypes(ClassTable* classtable)
{
  if (type_name->equal_string("SELF_TYPE",9)==0) set_type(classtable->classEnv.back());
  else set_type(type_name);
}

void isvoid_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   set_type(Bool);
}

void no_expr_class::inferTypes(ClassTable* classtable)
{
  //set type to null
  set_type(NULL);
}

void object_class::inferTypes(ClassTable* classtable)
{
  Symbol* t = classtable->objectEnv.lookup(name);
  if (t==NULL)
  {
    cerr << "Type declaration for object not found anywhere." << endl;
    classtable->semant_error();
    set_type(Object); //default to object type
  }
  else set_type(*t);
}
//End of 2nd phase

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
    collectTypes(classtable);
    inferTypes(classtable);
    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
}
