

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
  if (noinc.size()>1) {return false;} //the DAG should have exactly one root node only
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
    nodeId = noinc.back();
    noinc.pop_back();
    endpoints = classGraphCopy[nodeId];
    //Loop through all the next nodes
    for(unsigned int i=0;i<endpoints.size();i++)
    {
      nextNode = endpoints[i];
      found=false;
      //check if nextNode has no further incoming edges
      for(int j=0;j<intId;j++)
      {
	if (j==nodeId) continue;
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

//O(N) search in list for element className
int ClassTable::findClassNameInList(std::string className,std::vector<std::string> &list)
{
  for(unsigned int i=0;i<list.size();i++) if (list[i].compare(className)==0) return i;
  return -1;
}

int ClassTable::hasSameAttr(int x, int y)
{
  std::vector<Symbol> attrsX = attrStruct[x];
  std::vector<Symbol> attrsY = attrStruct[y];
  for(unsigned int i=0;i<attrsX.size();i++)
  {
    for(unsigned int j=0;j<attrsY.size();j++)
    {
      if (isSameSymbol(attrsX[i],attrsY[j])) return j;
    }
  }
  return -1;
}

void ClassTable::printGraph()
{
  for(int i=0;i<classGraph.size();i++)
  {
    cout << i << ": ";
    std::vector<int> v = classGraph[i];
    for(int j=0;j<v.size();j++)
    {
      cout << " " << v[j];
    }
    cout << endl;
  }
  for(int i=0;i<classesList.size();i++) cout << classesList[i] << endl;
  cout << "OK" << endl;
}

void ClassTable::printAttrs()
{
  for(int i=0;i<attrStruct.size();i++)
  {
    std::vector<Symbol> v = attrStruct[i];
    cout << i << ":";
    for(int j=0;j<v.size();j++)
    {
      cout << " " << v[j];
    }
    cout << endl;
  }
}

//Check if attributes are redefined in a classe's subclasses
bool ClassTable::redefinedAttributes(int &redefClassId, int &redefFeatureId)
{
  //printGraph();
  //printAttrs();
  std::vector<int> stack,stack2;
  stack.push_back(rootId);
  int curId,curId2,attrId;
  std::vector<int> vtemp,vtemp2;
  while(stack.size()>0)
  {
    curId = stack.back();
    //Embedded stack
    stack2.push_back(curId);
    while(stack2.size()>0)
    {
      curId2 = stack2.back();
      if ((curId!=curId2) && ((attrId = hasSameAttr(curId,curId2))!=-1))
      {
	redefClassId=curId2;
	redefFeatureId=attrId;
	return true;
      }
      stack2.pop_back();
      vtemp2 = classGraph[curId2];
      for(unsigned int k=0;k<vtemp2.size();k++)
      {
	stack2.push_back(vtemp2[k]);
      }
    }
    //Embedded stack
    stack.pop_back();
    vtemp = classGraph[curId];
    for(unsigned int k=0;k<vtemp.size();k++)
    {
      stack.push_back(vtemp[k]);
    }
  }
  return false;
}

bool ClassTable::AconformsToB(Symbol a, Symbol b)
{
  //a is b or one of its descendants
  std::string astr(a->get_string(),a->get_len());
  if (astr.compare("SELF_TYPE")==0)
  {
    a = currentClass->getName();
    astr.assign(a->get_string(),a->get_len());
  }
  std::string bstr(b->get_string(),b->get_len());
  if (bstr.compare("SELF_TYPE")==0)
  {
    b = currentClass->getName();
    bstr.assign(b->get_string(),b->get_len());
  }
  if (astr.compare(bstr)==0) return true;
  int aId = findClassNameInList(astr,classesList);
  if (aId==-1)  return false;
  int bId = findClassNameInList(bstr,classesList);
  if (bId==-1) return false;
  //Find aId starting from bId
  std::vector<int> stack;
  stack.push_back(bId);
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

//Outputs a vector that contains the class ids from the root of the inheritance graph down to x
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

bool ClassTable::isSameSymbol(Symbol a, Symbol b)
{
  char* nameA = a->get_string();
  int lenA = a->get_len();
  char* nameB = b->get_string();
  int lenB = b->get_len();
  std::string AStr(nameA,lenA);
  std::string BStr(nameB,lenB);
  return (AStr.compare(BStr)==0);
}

ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {
  //Iterate through the classes to build the dependency graph
  int classId,parentId,tempId,pvectId; //unique id to assign to every class
  //Initialize empty graph
  for(int i=0;i<classes->len();i++)
  {
    std::vector<int> v;
    classGraph.push_back(v);
  }
  for(int i=classes->first();classes->more(i);i=classes->next(i))
  {
    //Extract the class name and parent class name from each class
    Symbol classNameS = classes->nth(i)->getName();
    Symbol classParentS = classes->nth(i)->getParent();
    char* className = classNameS->get_string();
    int classNameLen = classNameS->get_len();
    char* classParent = classParentS->get_string();
    int classParentLen = classParentS->get_len();
    std::string classNameStr(className,classNameLen);
    std::string classParentStr(classParent,classParentLen);
    //Insert the class name and the parent's name into a list if they are not yet present
    //Insert the class name
    tempId = findClassNameInList(classNameStr,classesList);
    //Basic classes may not be redefined
    if (classNameStr.compare("SELF_TYPE")==0)
    {
      cerr << classes->nth(i)->get_filename() << ":" << classes->nth(i)->get_line_number()<< ": " << "Classes may not be redefined!" << endl;
      semant_error();
    }
    if (tempId==-1)//key not found => insert key-value pair of class name
    {
      classId = classesList.size();
      classesList.push_back(classNameStr);
      //check if this is the root node "Object"
      if (classNameStr.compare("Object")==0) rootId = classId;
      std::vector<Symbol> svect;
      for(int k=classes->nth(i)->get_features()->first();classes->nth(i)->get_features()->more(k);k=classes->nth(i)->get_features()->next(k))
      {
	if (classes->nth(i)->get_features()->nth(k)->getNodeType()==2)
	{
	  svect.push_back(classes->nth(i)->get_features()->nth(k)->getName());
	}
      }
      attrStruct.push_back(svect);
    }
    else
    {
      classId = tempId;
      //Classes may not be redefined
      if (findClassNameInList(classNameStr,parentsList)==-1)
      {
	if (i>classes->len()-6) //i is in the range of the basic classes
	{
	  for(int z=classes->first();classes->more(z);z=classes->next(z))
	  {
	    if (classes->nth(z)->getName()->equal_string((char*)classNameStr.c_str(),classNameStr.size())==1)
	      {
		cerr << classes->nth(z)->get_filename() << ":" << classes->nth(z)->get_line_number()<< ": " << "Classes may not be redefined!" << endl;
	semant_error();
	      }
	  }
	}
	else
	{
	  cerr << classes->nth(i)->get_filename() << ":" << classes->nth(i)->get_line_number()<< ": " << "Classes may not be redefined!" << endl;
	semant_error();
	}
      }
      else
      {
	std::vector<Symbol> svect;
	for(int k=classes->nth(i)->get_features()->first();classes->nth(i)->get_features()->more(k);k=classes->nth(i)->get_features()->next(k))
	{
	  if (classes->nth(i)->get_features()->nth(k)->getNodeType()==2)
	  {
	    svect.push_back(classes->nth(i)->get_features()->nth(k)->getName());
	  }
	}
	attrStruct[classId] = svect;
      }
    }
    if (classNameStr.compare("Object")!=0)
    {
      //Insert the parent class name
      tempId = findClassNameInList(classParentStr,classesList);
      if (tempId==-1)//key not found => insert key-value pair of parent class name
      {
	parentId = classesList.size();
	classesList.push_back(classParentStr);
	parentsList.push_back(classParentStr);
	std::vector<Symbol> svect;
	attrStruct.push_back(svect);
      }
      else parentId = tempId;
      if (classParentStr.compare("Object")==0) rootId = parentId;
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
  }
  if (semant_errors==0)
  {
    //Check if class main is defined
    bool mainDef=false;
    for(unsigned int i=0;i<classesList.size();i++) if (classesList[i].compare("Main")==0) mainDef=true;
    if (!mainDef)
    {
      cerr << "Class Main is not defined." << endl;
      semant_error();
    }
    //Check if the graph has cycles
    //Topological sort
    bool isOK = isDAG(classGraph.size());
    if (!isOK)
    {
      cerr << classes->nth(0)->get_filename() << ":" << classes->nth(0)->get_line_number()<< ": " << "Inheritance graph has cycles." << endl;
      semant_error();
    }
    else
    {
      //Check that no attributes are redefined in subclasses
      int redefClassId=0, redefFeatureId=0;
      if (redefinedAttributes(redefClassId, redefFeatureId))
      {
	//Search for class and attribute in tree
	for(int k=classes->first();classes->more(k);k=classes->next(k))
	{
	  if (classes->nth(k)->getName()->equal_string((char*)classesList[redefClassId].c_str(),classesList[redefClassId].size())==1)
	  {
	    redefClassId = k;
	    break;
	  }
	}
	//Found class look for attribute
	int cc=-1;
	for(int k=classes->nth(redefClassId)->get_features()->first();classes->nth(redefClassId)->get_features()->more(k);k=classes->nth(redefClassId)->get_features()->next(k))
	{
	  if (classes->nth(redefClassId)->get_features()->nth(k)->getNodeType()==2) cc++;
	  if (cc==redefFeatureId) {redefFeatureId=k;break;}
	}
	cerr << classes->nth(redefClassId)->get_filename() << ":" << classes->nth(redefClassId)->get_features()->nth(redefFeatureId)->get_line_number()<< ": " << "Attributes are redefined in subclass." << endl;
	semant_error();
      }
    }
  }
}

Symbol ClassTable::getTypeFromHierarchy(Symbol currentClass, Symbol attrName)
{
   //Search for attribute object in the class hierarchy
  std::string currentClassStr;
  currentClassStr.assign((char*)currentClass->get_string(),currentClass->get_len());
  std::vector<int> path = dfsPath(findClassNameInList(currentClassStr,classesList));
  SymbolTable<Symbol,Entry>* attrTable = NULL;
  Symbol type = NULL;
  bool found=false;
  for(int i=path.size()-1;i>=0;i--)
  {
    attrTable = attrEnv.lookup(idtable.add_string((char*)classesList[path[i]].c_str(),classesList[path[i]].size()));
    type = attrTable->lookup(attrName);
    if (type!=NULL)
    {
      found=true;
      break;
    }
  }
  return type;
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
  classtable->thisMethods->enterscope();
  classtable->thisAttr = new SymbolTable<Symbol, Entry>();
  classtable->thisAttr->enterscope();
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
  classtable->thisAttr->addid(name,type_decl);
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
    std::string fileN;
    for(int i = classes->first(); classes->more(i); i = classes->next(i))
    {
      Symbol namesym = classes->nth(i)->getName();
      if (classtable->isSameSymbol(namesym,Object)) continue;
      if (classtable->isSameSymbol(namesym,Str)) continue;
      if (classtable->isSameSymbol(namesym,Int)) continue;
      if (classtable->isSameSymbol(namesym,IO)) continue;
      if (classtable->isSameSymbol(namesym,Bool)) continue;
      classtable->currentClass = classes->nth(i);
      fileN.assign(classes->nth(i)->get_filename()->get_string(),classes->nth(i)->get_filename()->get_len());
      classtable->currentFileName = fileN;
      classes->nth(i)->inferTypes(classtable);
    }
}

void class__class::inferTypes(ClassTable* classtable)
{
  classtable->classEnv.push_back(name);
  classtable->objectEnv = *(classtable->attrEnv.lookup(name));
  for(int i = features->first(); features->more(i); i = features->next(i))
  {
    features->nth(i)->inferTypes(classtable);
  }
}

void method_class::inferTypes(ClassTable* classtable)
{
  //Check if the parameter list of the method contains variables names that assume multiple types
  Symbol name1, name2;
  for(int i = formals->first(); formals->more(i); i = formals->next(i))
  {
    name1 = formals->nth(i)->getName();
    for(int j = formals->first(); formals->more(j); j = formals->next(j))
    {
      if (i==j) continue;
      name2 = formals->nth(j)->getName();
      if (classtable->isSameSymbol(name1,name2))
      {
	cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Formal parameter" << name1  << " multiply defined!" << endl;
    classtable->semant_error();
	break;
      }
    }
  }
  //End of multiply defined check
  //Check if the method overrides a superclass method and if the signature matches
  std::string currentClassName((char*)classtable->currentClass->getName()->get_string(),classtable->currentClass->getName()->get_len());
  std::vector<int> path = classtable->dfsPath(classtable->findClassNameInList(currentClassName,classtable->classesList));
  SymbolTable<Symbol,std::vector<Symbol> >* methodTable;
  std::vector<Symbol>* argsVect;
  for(int i=(int)path.size()-2;i>=0;i--)
  {
    methodTable = classtable->methodEnv.lookup(idtable.add_string((char*)classtable->classesList[path[i]].c_str(),classtable->classesList[path[i]].size()));
    argsVect = methodTable->lookup(name);
    //First check if the method arguments have the same length
    if ((argsVect!=NULL) && (argsVect->size()-1!=formals->len()))
    {
      cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Incompatible number of formal parameters in redefined method " << name << "!" << endl;
    classtable->semant_error();
      break;
    }
    else if ((argsVect!=NULL) && (argsVect->size()-1==formals->len()))
    {
      //Check if the signature matches super class signature
      for(int j=0;j<argsVect->size()-1;j++)
      {
	if (!classtable->isSameSymbol(argsVect->at(j),formals->nth(j)->getType()))
	{
	   cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Types do not match in redefined method " << name << "!" << endl;
    classtable->semant_error();
	  break;
	}
      }
    }
  }
  //End of superclass signature match check
  classtable->objectEnv.enterscope();
  classtable->objectEnv.addid(self,classtable->classEnv.back());
  for(int i = formals->first(); formals->more(i); i = formals->next(i))
  {
    classtable->objectEnv.addid(formals->nth(i)->getName(),formals->nth(i)->getType());
    formals->nth(i)->inferTypes(classtable);
  } 
  expr->inferTypes(classtable);
  Symbol cmp;
  if (return_type->equal_string("SELF_TYPE",9)==1) cmp = classtable->classEnv.back();
  else cmp = return_type;
  Symbol t0prime = expr->get_type();
  if (!classtable->AconformsToB(t0prime,cmp))
  {
    cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Method body type does not conform to method return type" << endl;
    classtable->semant_error();
  }
  set_type(return_type);
  std::string typStr;
  typStr.assign((char*)return_type->get_string(),return_type->get_len());
  if (classtable->findClassNameInList(typStr,classtable->classesList)==-1)
  {
    cerr << classtable->currentClass->get_filename();
    cerr << ":" << get_line_number() << ": "  << "Undefined return type " << type_name << " !" << endl;
    classtable->semant_error();
  }
  classtable->objectEnv.exitscope();
}

void attr_class::inferTypes(ClassTable* classtable)
{
  if (name->equal_string("self",4)==1)
  {
    cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "An attribute cannot be named self!" << endl;
    classtable->semant_error();
  }
  Symbol sym = classtable->objectEnv.lookup(name);
  if (init->isNULL()) set_type(sym);
  else
  {
    classtable->objectEnv.enterscope();
    classtable->objectEnv.addid(self,classtable->classEnv.back());
    init->inferTypes(classtable);
    Symbol t1 = init->get_type();
    if (!classtable->AconformsToB(t1,sym))
    {
      cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Attribute initialization type does not conform to declared type" << endl;
      classtable->semant_error();
    }
    classtable->objectEnv.exitscope();
    set_type(sym);
  }
}

void formal_class::inferTypes(ClassTable* classtable)
{
  if (classtable->isSameSymbol(type_decl,SELF_TYPE))
  {
    cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Formal parameter " << name << " cannot have type SELF_TYPE." << endl;
    classtable->semant_error();
  }
  else if (classtable->isSameSymbol(name,self))
  {
    cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Formal parameters cannot be named 'self'." << endl;
    classtable->semant_error();
  }
  classtable->objectEnv.addid(name,type_decl);
}

void branch_class::inferTypes(ClassTable* classtable)
{
  classtable->objectEnv.enterscope();
  classtable->objectEnv.addid(name,type_decl);
  expr->inferTypes(classtable);
  classtable->objectEnv.exitscope();
  set_type(expr->get_type());
}

void assign_class::inferTypes(ClassTable* classtable)
{
  if (classtable->isSameSymbol(name,self))
  {
    cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Cannot assign to self." << endl;
    classtable->semant_error();
  }
   expr->inferTypes(classtable);
   //OK now expr e1 should have a type t'
   Symbol tprimeType = expr->get_type();
   //Check if tprimeType is <= type of name
   Symbol t = classtable->objectEnv.lookup(name);
   if (t==NULL)
   {
     Symbol currentClassSym = classtable->currentClass->getName();
     t = classtable->getTypeFromHierarchy(currentClassSym,name);
   }
   if (t==NULL)
   {
    cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Type declaration for object not found anywhere." << endl;
    classtable->semant_error();
    t = Object; //default to object type
    set_type(Object);
   }
   else
   {
     if (!classtable->AconformsToB(tprimeType, t))
     {
       cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Assign class error: Assigned type " << tprimeType << " does not conform to type " << t <<  " of variable " << name  << "." << endl;
       classtable->semant_error();
     }
     set_type(tprimeType);
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
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Static dispatch class does not conform to given class T" << endl;
     classtable->semant_error();
   }
   SymbolTable<Symbol,std::vector<Symbol> >* methodsList = classtable->methodEnv.lookup(type_name);
   if ((classtable->methodEnv.probe(type_name)==NULL) || (methodsList->lookup(name)==NULL))
   {
      cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Dispatch object or method not found!" << endl;
       classtable->semant_error();
       return;
   }
   std::vector<Symbol> methodArgs = *(methodsList->lookup(name));
   //Check that all actual types conform to the types of methodArgs
   for(unsigned int i=0;i<methodArgs.size()-1;i++)
   {
     if (!classtable->AconformsToB(actual->nth(i)->get_type(),methodArgs[i]))
     {
       classtable->semant_error();
       cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Actual types of args of dynamic dispatch method do not conform to declared types" << endl;
       break;
     }
   }
   //Set the return type
   if (methodArgs.back()->equal_string("SELF_TYPE",9)==1) set_type(className);
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
   if (className->equal_string("SELF_TYPE",9)==1) t0prime = classtable->classEnv.back();
   else t0prime = className;
   if (classtable->methodEnv.lookup(t0prime)==NULL)
   {
     cerr << classtable->currentFileName << ":";
     cerr << get_line_number() << ": "  << "Dispatch object not found!" << endl;
     classtable->semant_error();
     return;
   }
   std::string t0primestr(t0prime->get_string(),t0prime->get_len());
   int t0primeId = classtable->findClassNameInList(t0primestr,classtable->classesList);
   std::vector<int> path = classtable->dfsPath(t0primeId);
   bool foundMethod=false;
   SymbolTable<Symbol,std::vector<Symbol> >* methodsList = NULL;
   std::vector<Symbol> methodArgs;
   Symbol classType;
   std::string typeStr;
   for(int i=path.size()-1;i>=0;i--)
   {
     typeStr = classtable->classesList[path[i]];
     classType = idtable.add_string((char*)typeStr.c_str(),typeStr.size());
     methodsList = classtable->methodEnv.lookup(classType);
     if (methodsList->lookup(name)!=NULL)
     {
       methodArgs = *(methodsList->lookup(name));
       foundMethod = true;
       break;
     }
   }
   if (!foundMethod)
   {
     cerr << classtable->currentClass->get_filename();
     cerr << ":" << get_line_number() << ": ";
     cerr << "Dispatch method not defined on expression type!" << endl;
     classtable->semant_error();
     return;
   }
   //Check that all actual types conform to the types of methodArgs
   Symbol argType;
   for(unsigned int i=0;i<methodArgs.size()-1;i++)
   {
     if (actual->nth(i)->get_type()->equal_string("SELF_TYPE",9)==1) argType = classtable->classEnv.back();
     else argType = actual->nth(i)->get_type();
     if (!classtable->AconformsToB(argType,methodArgs[i]))
     {
       classtable->semant_error();
       cerr << classtable->currentClass->get_filename() << ":" << get_line_number() << ": " << "Actual types of args of dynamic dispatch method do not conform to declared types" << endl;
       break;
     }
   }
   //Set the return type
   if (methodArgs.back()->equal_string("SELF_TYPE",9)==1) set_type(className);
   else set_type(methodArgs.back());
}

void cond_class::inferTypes(ClassTable* classtable)
{
   pred->inferTypes(classtable);
   then_exp->inferTypes(classtable);
   else_exp->inferTypes(classtable);
   if (pred->get_type()->equal_string("Bool",4)!=1) cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "If predicate does not have type Bool!" << endl;
   set_type(classtable->leastCommonAncestor(then_exp->get_type(),else_exp->get_type()));
}

void loop_class::inferTypes(ClassTable* classtable)
{
  pred->inferTypes(classtable);
  body->inferTypes(classtable);
  if (pred->get_type()->equal_string("Bool",4)!=1)
  {
    cerr << classtable->currentClass->get_filename() << ":" << get_line_number() << ": " << "Loop predicate must have type Bool!" << endl;
    classtable->semant_error();
  }
  set_type(Object);
}

void typcase_class::inferTypes(ClassTable* classtable)
{
  //Check that no branch switch variable has the same type than another
  Symbol type1, type2;
  for(int i = cases->first(); cases->more(i); i = cases->next(i))
  {
    type1 = cases->nth(i)->getType();
    for(int j = cases->first(); cases->more(j); j = cases->next(j))
    {
      if (i==j) continue;
      type2 = cases->nth(j)->getType();
      if (classtable->isSameSymbol(type1,type2))
      {
	cerr << classtable->currentClass->get_filename() << ":" << get_line_number() << ": " << "Case branches must all have unique types among the cases!" << endl;
    classtable->semant_error();
      }
    }
  }
  //End of check
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
  Symbol t0prime;
  if (classtable->isSameSymbol(identifier,self))
  {
    cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Self cannot be bound in a let expression!" << endl;
      classtable->semant_error();
  }
  if (init->isNULL())
  {
    t0prime = type_decl;
    classtable->objectEnv.enterscope();
    classtable->objectEnv.addid(identifier,t0prime);
    body->inferTypes(classtable);
    classtable->objectEnv.exitscope();
    set_type(body->get_type());
  }
  else
  {
    t0prime = type_decl;
    init->inferTypes(classtable);
    Symbol t1 = init->get_type();
    if (!classtable->AconformsToB(t1,t0prime))
    {
      cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Let initialization type does not conform to declared type!" << endl;
      classtable->semant_error();
    }
    classtable->objectEnv.enterscope();
    classtable->objectEnv.addid(identifier,t0prime);
    body->inferTypes(classtable);
    classtable->objectEnv.exitscope();
    set_type(body->get_type());
  }
}

void plus_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   e2->inferTypes(classtable);
   Symbol t1 = e1->get_type();
   Symbol t2 = e2->get_type();
   if (t1->equal_string("Int",3)!=1)
   {
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   if (t2->equal_string("Int",3)!=1)
   {
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Plus operand must have type Int!" << endl;
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
   if (t1->equal_string("Int",3)!=1)
   {
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   if (t2->equal_string("Int",3)!=1)
   {
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Plus operand must have type Int!" << endl;
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
   if (t1->equal_string("Int",3)!=1)
   {
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   if (t2->equal_string("Int",3)!=1)
   {
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Plus operand must have type Int!" << endl;
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
   if (t1->equal_string("Int",3)!=1)
   {
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Plus operand must have type Int!" << endl;
     classtable->semant_error();
   }
   if (t2->equal_string("Int",3)!=1)
   {
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Plus operand must have type Int!" << endl;
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
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Neg class should have Int type" << endl;
   }
   set_type(Int);
}

void lt_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   e2->inferTypes(classtable);
   if ((e1->get_type()->equal_string("Int",3)!=1) && (e2->get_type()->equal_string("Int",3)!=1))
   {
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Leq comparison operators must have type Int!" << endl;
     classtable->semant_error();
   }
   set_type(Bool);
}


void eq_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   e2->inferTypes(classtable);
   Symbol eqtype = e1->get_type();
   if ((eqtype->equal_string("Int",3)==1) || (eqtype->equal_string("String",6)==1) || (eqtype->equal_string("Bool",4)==1))
   {
     if (!classtable->isSameSymbol(e2->get_type(),eqtype))
     {
       cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Eq class needs to have operands of type Int, String or Bool!" << endl;
       classtable->semant_error();
     }
   }
   set_type(Bool);
}

void leq_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   e2->inferTypes(classtable);
   if ((e1->get_type()->equal_string("Int",3)!=1) && (e2->get_type()->equal_string("Int",3)!=1))
   {
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": " << "Leq comparison operators must have type Int!" << endl;
     classtable->semant_error();
   }
   set_type(Bool);
}

void comp_class::inferTypes(ClassTable* classtable)
{
   e1->inferTypes(classtable);
   if (e1->get_type()->equal_string("Bool",4)!=1)
   {
     cerr << classtable->currentClass->get_filename() << ":" << get_line_number()<< ": "  << "Not operand must have type Bool!" << endl;
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
  set_type(Str);
}

void new__class::inferTypes(ClassTable* classtable)
{
  if (type_name->equal_string("SELF_TYPE",9)==1) set_type(SELF_TYPE);
  else
  {
    std::string typStr;
    typStr.assign((char*)type_name->get_string(),type_name->get_len());
    if (classtable->findClassNameInList(typStr,classtable->classesList)==-1)
    {
      cerr << classtable->currentClass->get_filename();
      cerr << ":" << get_line_number() << ": "  << "New used with undefined class " << type_name << " !" << endl;
      classtable->semant_error();
    }
    set_type(type_name);
  }
}

void isvoid_class::inferTypes(ClassTable* classtable)
{
  e1->inferTypes(classtable);
   set_type(Bool);
}

void no_expr_class::inferTypes(ClassTable* classtable)
{
  set_type(No_type);
}

void object_class::inferTypes(ClassTable* classtable)
{
  if (classtable->isSameSymbol(name,self))
  {
    set_type(SELF_TYPE);
    return;
  }
  Symbol nameType = classtable->objectEnv.lookup(name);
  if (nameType!=NULL)
  {
    set_type(nameType); //default to object type
    return;
  }
  //Search for attribute object in the class hierarchy
  Symbol currentClassSym = classtable->currentClass->getName();
  Symbol type = classtable->getTypeFromHierarchy(currentClassSym,name);
  if (type==NULL)
  {
    cerr << classtable->currentClass->get_filename();
    cerr << ":" << get_line_number() << ": ";
    cerr << "Dispatch method not defined on expression type!" << endl;
    classtable->semant_error();
    set_type(Object);
    return;
  }
  else set_type(type);
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
    Classes temp = classes;
    for(int i=basicClasses->first();basicClasses->more(i);i=basicClasses->next(i))
    {
      classes = append_Classes(classes,single_Classes(basicClasses->nth(i)));
    }
    /* ClassTable constructor may do some semantic analysis */
    ClassTable *classtable = new ClassTable(classes);
    collectTypes(classtable);
    inferTypes(classtable);
    classes = temp;
    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
}
