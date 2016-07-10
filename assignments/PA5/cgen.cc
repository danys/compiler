
//**************************************************************
//
// Code generator SKELETON
//
// Read the comments carefully. Make sure to
//    initialize the base class tags in
//       `CgenClassTable::CgenClassTable'
//
//    Add the label for the dispatch tables to
//       `IntEntry::code_def'
//       `StringEntry::code_def'
//       `BoolConst::code_def'
//
//    Add code to emit everyting else that is needed
//       in `CgenClassTable::code'
//
//
// The files as provided will produce code to begin the code
// segments, declare globals, and emit constants.  You must
// fill in the rest.
//
//**************************************************************

#include "cgen.h"
#include "cgen_gc.h"

extern void emit_string_constant(ostream& str, char *s);
extern int cgen_debug;

//
// Three symbols from the semantic analyzer (semant.cc) are used.
// If e : No_type, then no code is generated for e.
// Special code is generated for new SELF_TYPE.
// The name "self" also generates code different from other references.
//
//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
Symbol 
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

static char *gc_init_names[] =
  { "_NoGC_Init", "_GenGC_Init", "_ScnGC_Init" };
static char *gc_collect_names[] =
  { "_NoGC_Collect", "_GenGC_Collect", "_ScnGC_Collect" };


//  BoolConst is a class that implements code generation for operations
//  on the two booleans, which are given global names here.
BoolConst falsebool(FALSE);
BoolConst truebool(TRUE);

static int curLabel = 0;

//*********************************************************
//
// Define method for code generation
//
// This is the method called by the compiler driver
// `cgtest.cc'. cgen takes an `ostream' to which the assembly will be
// emmitted, and it passes this and the class list of the
// code generator tree to the constructor for `CgenClassTable'.
// That constructor performs all of the work of the code
// generator.
//
//*********************************************************

void program_class::cgen(ostream &os) 
{
  // spim wants comments to start with '#'
  os << "# start of generated code\n";

  initialize_constants();
  CgenClassTable *codegen_classtable = new CgenClassTable(classes,os);
  cout << "Walking AST and generating stack machine code" << endl;
  for(int i=classes->first();classes->more(i);i=classes->next(i))
  {
    classes->nth(i)->code(os,codegen_classtable);
  }
  os << "\n# end of generated code\n";
}

//////////////////////////////////////////////////////////////////////////////
//
//  emit_* procedures
//
//  emit_X  writes code for operation "X" to the output stream.
//  There is an emit_X for each opcode X, as well as emit_ functions
//  for generating names according to the naming conventions (see emit.h)
//  and calls to support functions defined in the trap handler.
//
//  Register names and addresses are passed as strings.  See `emit.h'
//  for symbolic names you can use to refer to the strings.
//
//////////////////////////////////////////////////////////////////////////////

static void emit_load(char *dest_reg, int offset, char *source_reg, ostream& s)
{
  s << LW << dest_reg << " " << offset * WORD_SIZE << "(" << source_reg << ")" 
    << endl;
}

static void emit_store(char *source_reg, int offset, char *dest_reg, ostream& s)
{
  s << SW << source_reg << " " << offset * WORD_SIZE << "(" << dest_reg << ")"
      << endl;
}

static void emit_load_imm(char *dest_reg, int val, ostream& s)
{ s << LI << dest_reg << " " << val << endl; }

static void emit_load_address(char *dest_reg, char *address, ostream& s)
{ s << LA << dest_reg << " " << address << endl; }

static void emit_partial_load_address(char *dest_reg, ostream& s)
{ s << LA << dest_reg << " "; }

static void emit_load_bool(char *dest, const BoolConst& b, ostream& s)
{
  emit_partial_load_address(dest,s);
  b.code_ref(s);
  s << endl;
}

static void emit_load_string(char *dest, StringEntry *str, ostream& s)
{
  emit_partial_load_address(dest,s);
  str->code_ref(s);
  s << endl;
}

static void emit_load_int(char *dest, IntEntry *i, ostream& s)
{
  emit_partial_load_address(dest,s);
  i->code_ref(s);
  s << endl;
}

static void emit_move(char *dest_reg, char *source_reg, ostream& s)
{ s << MOVE << dest_reg << " " << source_reg << endl; }

static void emit_neg(char *dest, char *src1, ostream& s)
{ s << NEG << dest << " " << src1 << endl; }

static void emit_add(char *dest, char *src1, char *src2, ostream& s)
{ s << ADD << dest << " " << src1 << " " << src2 << endl; }

static void emit_addu(char *dest, char *src1, char *src2, ostream& s)
{ s << ADDU << dest << " " << src1 << " " << src2 << endl; }

static void emit_addiu(char *dest, char *src1, int imm, ostream& s)
{ s << ADDIU << dest << " " << src1 << " " << imm << endl; }

static void emit_div(char *dest, char *src1, char *src2, ostream& s)
{ s << DIV << dest << " " << src1 << " " << src2 << endl; }

static void emit_mul(char *dest, char *src1, char *src2, ostream& s)
{ s << MUL << dest << " " << src1 << " " << src2 << endl; }

static void emit_sub(char *dest, char *src1, char *src2, ostream& s)
{ s << SUB << dest << " " << src1 << " " << src2 << endl; }

static void emit_sll(char *dest, char *src1, int num, ostream& s)
{ s << SLL << dest << " " << src1 << " " << num << endl; }

static void emit_jalr(char *dest, ostream& s)
{ s << JALR << "\t" << dest << endl; }

static void emit_jal(char *address,ostream &s)
{ s << JAL << address << endl; }

static void emit_return(ostream& s)
{ s << RET << endl; }

static void emit_gc_assign(ostream& s)
{ s << JAL << "_GenGC_Assign" << endl; }

static void emit_disptable_ref(Symbol sym, ostream& s)
{  s << sym << DISPTAB_SUFFIX; }

static void emit_init_ref(Symbol sym, ostream& s)
{ s << sym << CLASSINIT_SUFFIX; }

static void emit_label_ref(int l, ostream &s)
{ s << "label" << l; }

static void emit_protobj_ref(Symbol sym, ostream& s)
{ s << sym << PROTOBJ_SUFFIX; }

static void emit_method_ref(Symbol classname, Symbol methodname, ostream& s)
{ s << classname << METHOD_SEP << methodname; }

static void emit_label_def(int l, ostream &s)
{
  emit_label_ref(l,s);
  s << ":" << endl;
}

static void emit_beqz(char *source, int label, ostream &s)
{
  s << BEQZ << source << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_beq(char *src1, char *src2, int label, ostream &s)
{
  s << BEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bne(char *src1, char *src2, int label, ostream &s)
{
  s << BNE << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bleq(char *src1, char *src2, int label, ostream &s)
{
  s << BLEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_blt(char *src1, char *src2, int label, ostream &s)
{
  s << BLT << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_blti(char *src1, int imm, int label, ostream &s)
{
  s << BLT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bgti(char *src1, int imm, int label, ostream &s)
{
  s << BGT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_branch(int l, ostream& s)
{
  s << BRANCH;
  emit_label_ref(l,s);
  s << endl;
}

//
// Push a register on the stack. The stack grows towards smaller addresses.
//
static void emit_push(char *reg, ostream& str)
{
  emit_store(reg,0,SP,str);
  emit_addiu(SP,SP,-4,str);
}

static void emit_instantiate(ostream &s)
{
  s << JAL;
  emit_method_ref(Object,copy,s);
  s << endl;
}

//
// Fetch the integer value in an Int object.
// Emits code to fetch the integer value of the Integer object pointed
// to by register source into the register dest
//
static void emit_fetch_int(char *dest, char *source, ostream& s)
{ emit_load(dest, DEFAULT_OBJFIELDS, source, s); }

//
// Emits code to store the integer value contained in register source
// into the Integer object pointed to by dest.
//
static void emit_store_int(char *source, char *dest, ostream& s)
{ emit_store(source, DEFAULT_OBJFIELDS, dest, s); }


static void emit_test_collector(ostream &s)
{
  emit_push(ACC, s);
  emit_move(ACC, SP, s); // stack end
  emit_move(A1, ZERO, s); // allocate nothing
  s << JAL << gc_collect_names[cgen_Memmgr] << endl;
  emit_addiu(SP,SP,4,s);
  emit_load(ACC,0,SP,s);
}

static void emit_gc_check(char *source, ostream &s)
{
  if (source != (char*)A1) emit_move(A1, source, s);
  s << JAL << "_gc_check" << endl;
}


///////////////////////////////////////////////////////////////////////////////
//
// coding strings, ints, and booleans
//
// Cool has three kinds of constants: strings, ints, and booleans.
// This section defines code generation for each type.
//
// All string constants are listed in the global "stringtable" and have
// type StringEntry.  StringEntry methods are defined both for String
// constant definitions and references.
//
// All integer constants are listed in the global "inttable" and have
// type IntEntry.  IntEntry methods are defined for Int
// constant definitions and references.
//
// Since there are only two Bool values, there is no need for a table.
// The two booleans are represented by instances of the class BoolConst,
// which defines the definition and reference methods for Bools.
//
///////////////////////////////////////////////////////////////////////////////

//
// Strings
//
void StringEntry::code_ref(ostream& s)
{
  s << STRCONST_PREFIX << index;
}

//
// Emit code for a constant String.
// You should fill in the code naming the dispatch table.
//

void StringEntry::code_def(ostream& s, int stringclasstag)
{
  IntEntryP lensym = inttable.add_int(len);

  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s  << LABEL                                             // label
      << WORD << stringclasstag << endl                                 // tag
      << WORD << (DEFAULT_OBJFIELDS + STRING_SLOTS + (len+4)/4) << endl // size
      << WORD <<  "String" << DISPTAB_SUFFIX << endl;                   // dispatch table
      s << WORD;  lensym->code_ref(s);  s << endl;            // string length
  emit_string_constant(s,str);                                // ascii string
  s << ALIGN;                                                 // align to word
}

//
// StrTable::code_string
// Generate a string object definition for every string constant in the 
// stringtable.
//
void StrTable::code_string_table(ostream& s, int stringclasstag)
{  
  for (List<StringEntry> *l = tbl; l; l = l->tl())
    l->hd()->code_def(s,stringclasstag);
}

//
// Ints
//
void IntEntry::code_ref(ostream &s)
{
  s << INTCONST_PREFIX << index;
}

//
// Emit code for a constant Integer.
// You should fill in the code naming the dispatch table.
//

void IntEntry::code_def(ostream &s, int intclasstag)
{
  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s << LABEL                                // label
      << WORD << intclasstag << endl                      // class tag
      << WORD << (DEFAULT_OBJFIELDS + INT_SLOTS) << endl  // object size
      << WORD << "Int" << DISPTAB_SUFFIX << endl;         // dispatch table
      s << WORD << str << endl;                           // integer value
}


//
// IntTable::code_string_table
// Generate an Int object definition for every Int constant in the
// inttable.
//
void IntTable::code_string_table(ostream &s, int intclasstag)
{
  for (List<IntEntry> *l = tbl; l; l = l->tl())
    l->hd()->code_def(s,intclasstag);
}


//
// Bools
//
BoolConst::BoolConst(int i) : val(i) { assert(i == 0 || i == 1); }

void BoolConst::code_ref(ostream& s) const
{
  s << BOOLCONST_PREFIX << val;
}
  
//
// Emit code for a constant Bool.
// You should fill in the code naming the dispatch table.
//

void BoolConst::code_def(ostream& s, int boolclasstag)
{
  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s << LABEL                                  // label
      << WORD << boolclasstag << endl                       // class tag
      << WORD << (DEFAULT_OBJFIELDS + BOOL_SLOTS) << endl   // object size
      << WORD << "Bool" << DISPTAB_SUFFIX << endl;          // dispatch table
      s << WORD << val << endl;                             // value (0 or 1)
}

//////////////////////////////////////////////////////////////////////////////
//
//  CgenClassTable methods
//
//////////////////////////////////////////////////////////////////////////////

//***************************************************
//
//  Emit code to start the .data segment and to
//  declare the global names.
//
//***************************************************

void CgenClassTable::code_global_data()
{
  Symbol main    = idtable.lookup_string(MAINNAME);
  Symbol string  = idtable.lookup_string(STRINGNAME);
  Symbol integer = idtable.lookup_string(INTNAME);
  Symbol boolc   = idtable.lookup_string(BOOLNAME);

  str << "\t.data\n" << ALIGN;
  //
  // The following global names must be defined first.
  //
  str << GLOBAL << CLASSNAMETAB << endl;
  str << GLOBAL; emit_protobj_ref(main,str);    str << endl;
  str << GLOBAL; emit_protobj_ref(integer,str); str << endl;
  str << GLOBAL; emit_protobj_ref(string,str);  str << endl;
  str << GLOBAL; falsebool.code_ref(str);  str << endl;
  str << GLOBAL; truebool.code_ref(str);   str << endl;
  str << GLOBAL << INTTAG << endl;
  str << GLOBAL << BOOLTAG << endl;
  str << GLOBAL << STRINGTAG << endl;

  //
  // We also need to know the tag of the Int, String, and Bool classes
  // during code generation.
  //
  str << INTTAG << LABEL
      << WORD << intclasstag << endl;
  str << BOOLTAG << LABEL 
      << WORD << boolclasstag << endl;
  str << STRINGTAG << LABEL 
      << WORD << stringclasstag << endl;    
}


//***************************************************
//
//  Emit code to start the .text segment and to
//  declare the global names.
//
//***************************************************

void CgenClassTable::code_global_text()
{
  str << GLOBAL << HEAP_START << endl
      << HEAP_START << LABEL 
      << WORD << 0 << endl
      << "\t.text" << endl
      << GLOBAL;
  emit_init_ref(idtable.add_string("Main"), str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("Int"),str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("String"),str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("Bool"),str);
  str << endl << GLOBAL;
  emit_method_ref(idtable.add_string("Main"), idtable.add_string("main"), str);
  str << endl;
}

void CgenClassTable::code_bools(int boolclasstag)
{
  falsebool.code_def(str,boolclasstag);
  truebool.code_def(str,boolclasstag);
}

void CgenClassTable::code_select_gc()
{
  //
  // Generate GC choice constants (pointers to GC functions)
  //
  str << GLOBAL << "_MemMgr_INITIALIZER" << endl;
  str << "_MemMgr_INITIALIZER:" << endl;
  str << WORD << gc_init_names[cgen_Memmgr] << endl;
  str << GLOBAL << "_MemMgr_COLLECTOR" << endl;
  str << "_MemMgr_COLLECTOR:" << endl;
  str << WORD << gc_collect_names[cgen_Memmgr] << endl;
  str << GLOBAL << "_MemMgr_TEST" << endl;
  str << "_MemMgr_TEST:" << endl;
  str << WORD << (cgen_Memmgr_Test == GC_TEST) << endl;
}

//Set up class_nameTab which maps from (class tag)*4
void CgenClassTable::code_class_name_tab()
{
  //offset position to a string object refering to the name
  //of the class in question
  StringEntry* classSym;
  //Put all class names as string objects in memory
  for(unsigned int i=0;i<classNames.size();i++)
  {
    classSym = stringtable.add_string((char*)classNames[i].c_str(),classNames[i].size());
    classSym->code_def(str,stringclasstag);
  }
  //Output pointers to the string objects
  str << CLASSNAMETAB << LABEL;
  for(unsigned int i=0;i<classNames.size();i++)
  {
    classSym = stringtable.add_string((char*)classNames[i].c_str(),classNames[i].size());
    str << WORD;
    classSym->code_ref(str);
    str << endl;
  }
}

//Generate code for the prototype objects
void CgenClassTable::code_prototype_objects()
{
  CgenNode* node;
  int currentClassTag,objSize;
  for(List<CgenNode> *l = nds; l != NULL; l=l->tl())
  {
    node = l->hd();
    currentClassTag = findClassTag(node->get_name());
    
    //Compute the object size
    //Size = DEFAULT_OBJFIELDS+NUMBER_OF_ATTR+NUMBER_OF_INHER_ATTR
    objSize = DEFAULT_OBJFIELDS + node->attributes.size();
    str << WORD << "-1" << endl;
    str << WORD << currentClassTag << PROTOBJ_SUFFIX << LABEL     // label
      << WORD << currentClassTag << endl                       // class tag
      << WORD << objSize << endl   // object size
      << WORD << node->get_name() << DISPTAB_SUFFIX << endl;          // dispatch table
    attr_class* attribute;
    for(unsigned int i=0;i<node->attributes.size();i++)
    {
      attribute = dynamic_cast<attr_class*>(node->attributes[i]);
      str << WORD;
      if (attribute->type_decl==Int)
      {
	IntEntry* intentry = (IntEntry*)inttable.add_int(0);
	intentry->code_ref(str);
      }
      else if (attribute->type_decl==Str)
      {
	StringEntry* strentry = (StringEntry*)stringtable.add_string("");
	strentry->code_ref(str);
      }
      else if (attribute->type_decl==Bool)
      {
	BoolConst falseb(false);
	falseb.code_ref(str);
      }
      else str << "0";
      str << endl;
    }
  }
}

void CgenClassTable::code_class_obj_table()
{
  str << WORD << CLASSOBJTAB << LABEL;
  List<CgenNode>* nodes = nds;
  for(CgenNode* node=nodes->hd(); node!=NULL; nodes=nodes->tl())
  {
    str << WORD << node->get_name() << PROTOBJ_SUFFIX << endl;
    str << WORD << node->get_name() << CLASSINIT_SUFFIX << endl;
  }
}


void setUpCallee(ostream &s)
{
  emit_addiu(SP, SP, -3 * WORD_SIZE, s);
  emit_store(FP, 3, SP, s);
  emit_store(SELF, 2, SP, s);
  emit_store(RA, 1, SP, s);
  emit_addiu(FP, SP, WORD_SIZE, s);
  emit_move(SELF, ACC, s);
}

void tearDownCallee(int nArgs,ostream &s)
{
  emit_load(FP,3,SP,s);
  emit_load(SELF,2,SP,s);
  emit_load(RA,1,SP,s);
  emit_addiu(SP,SP,(3+nArgs)*WORD_SIZE,s);
  emit_return(s);
}

void attr_class::code_init_attr(ostream &s,CgenNode* node)
{
  if (init!=NULL)
  {
    int offset = node->getFeatureOffsetByName(name,true);
    init->code(s,node->classTable); //Leaves init result in $a0
    emit_store(ACC,offset,SELF,s);
  }
}

void CgenNode::code_init_method(ostream &s)
{
  emit_init_ref(name,s);
  s << LABEL;
  setUpCallee(s);
  //First call the parent class's initialization method
  if ((get_parentnd()!=NULL) && (get_parentnd()->name!=No_class))
  {
    s << JAL << get_parentnd()->name << DISPTAB_SUFFIX << endl;
  }
  //Loop over all the features and init attributes
  Feature feature;
  for(int i=features->first();features->more(i);i=features->next(i))
  {
    feature = features->nth(i);
    if (!feature->isMethod()) feature->code_init_attr(s,this);
  }
  emit_move(ACC,SELF,s);
  tearDownCallee(0,s);
}

//Emit code for each init method of every class
void CgenClassTable::code_class_init_methods()
{
  List<CgenNode>* nodes = nds;
  for(;nodes;nodes=nodes->tl())
  {
    nodes->hd()->code_init_method(str);
  }
}

void CgenClassTable::code_dispatch_tables()
{
  List<CgenNode> * nodes = nds;
  CgenNode* node = NULL;
  for(;nodes->hd()!=NULL;nodes=nodes->tl())
  {
    node=nodes->hd();
    str << node->name << DISPTAB_SUFFIX << LABEL;
    for(unsigned int i=0;i<node->methods.size();i++)
      str << WORD << node->name << "." << node->methods[i]->getName() << endl;
  }
}

//********************************************************
//
// Emit code to reserve space for and initialize all of
// the constants.  Class names should have been added to
// the string table (in the supplied code, is is done
// during the construction of the inheritance graph), and
// code for emitting string constants as a side effect adds
// the string's length to the integer table.  The constants
// are emmitted by running through the stringtable and inttable
// and producing code for each entry.
//
//********************************************************

void CgenClassTable::code_constants()
{
  //
  // Add constants that are required by the code generator.
  //
  stringtable.add_string("");
  inttable.add_string("0");

  //Add stringtable entries, inttable entries and booleans
  stringtable.code_string_table(str,stringclasstag);
  inttable.code_string_table(str,intclasstag);
  code_bools(boolclasstag);
}

//Assign class tag to this class and its name to the class list
void CgenClassTable::setClassTag(CgenNode* node)
{
  node->setClassTag(classtagindex);
  if (node->name->equal_string("String",6)==1) stringclasstag = classtagindex;
  else if (node->name->equal_string("Int",3)==1) intclasstag = classtagindex;
  else if (node->name->equal_string("Bool",4)==1) boolclasstag = classtagindex;
  //add the current class name to the list of class names
  std::string className(node->name->get_string(),node->name->get_len());
  classNames.push_back(className);
  classtagindex++;
}

//Set methods and attributes from the fiven fromObj to this class node
void CgenNode::setMethodsAndAttributes(CgenNode* fromObj, bool checkOverride)
{
  //Loop over features
  Feature feature;
  for(int i=fromObj->features->first();fromObj->features->more(i);i=fromObj->features->next(i))
  {
    feature = fromObj->features->nth(i);
    if (feature->isMethod())
    {
      //OK feature is a method
      if (checkOverride)
      {
	//Check if this method has already been inherited
	//A method with the same name overrides a method inherited by an ancestor (types do not need to be the same)
	//Symbol methodName = feature->name;
	int methodIndex=-1;
	for(unsigned int j=0;j<methods.size();j++)
	{
	  if (methods[j]->getName()->equal_string(feature->getName()->get_string(),feature->getName()->get_len())==1) methodIndex=j;
	}
	if (methodIndex==-1) methods.push_back(feature);
	else methods[methodIndex]=feature; //override method
      }
      else methods.push_back(feature);
    }
    else
    {
      //OK feature is an attribute
      //Attributes cannot be inherited, checked already by the semantic analyzer
      attributes.push_back(feature);
    }
  }
}

//Set the all the attributes and methods of every class in the inheritance tree
void CgenClassTable::setClassAttributesAndMethods()
{
  //Initialize
  classtagindex=0;
  classNames.clear();
  //Start from the root node
  CgenNode* rootNode = root();
  setClassTag(rootNode);
  CgenNode* curNode = NULL;
  rootNode->setMethodsAndAttributes(rootNode,false);
  std::stack<CgenNode*> nodeStack;
  nodeStack.push(rootNode);
  //DFS traversal through the class nodes
  while(!nodeStack.empty())
  {
    curNode = nodeStack.top();
    nodeStack.pop();
    setClassTag(curNode);
    curNode->setMethodsAndAttributes(curNode->get_parentnd(),false); //add parent class methods and attributes
    curNode->setMethodsAndAttributes(curNode,true); //add this object's class methods and attributes
    List<CgenNode>* list = curNode->get_children();
    for(;list!=NULL;list=list->tl()) nodeStack.push(list->hd());
  }
}

CgenClassTable::CgenClassTable(Classes classes, ostream& s) : nds(NULL) , str(s)
{
   enterscope();
   if (cgen_debug) cout << "Building CgenClassTable" << endl;
   install_basic_classes();
   install_classes(classes);
   build_inheritance_tree();

   setClassAttributesAndMethods(); //find every class's list of attributes and methods and set the class tags
   code();
   exitscope();
}

//Look up a class node
CgenNode* CgenClassTable::getClassByName(Symbol className)
{
  List<CgenNode>* nodes = nds;
  CgenNode* node;
  for(;nodes;nodes=nodes->tl())
  {
    node = nodes->hd();
    if (node->name==className) return node;
  }
  return NULL;
}

//Get the attribute or method offset
int CgenNode::getFeatureOffsetByName(Symbol featureName, bool isAttribute)
{
  std::vector<Feature> feats;
  if (isAttribute) feats = attributes;
  else feats = methods;
  for(unsigned int i=0;i<feats.size();i++)
  {
    if (feats[i]->getName()==featureName) return i;
  }
  return -1;
}

void CgenClassTable::install_basic_classes()
{

// The tree package uses these globals to annotate the classes built below.
  //curr_lineno  = 0;
  Symbol filename = stringtable.add_string("<basic class>");

//
// A few special class names are installed in the lookup table but not
// the class list.  Thus, these classes exist, but are not part of the
// inheritance hierarchy.
// No_class serves as the parent of Object and the other special classes.
// SELF_TYPE is the self class; it cannot be redefined or inherited.
// prim_slot is a class known to the code generator.
//
  addid(No_class,
	new CgenNode(class_(No_class,No_class,nil_Features(),filename),
			    Basic,this));

  addid(SELF_TYPE,
	new CgenNode(class_(SELF_TYPE,No_class,nil_Features(),filename),
			    Basic,this));

  addid(prim_slot,
	new CgenNode(class_(prim_slot,No_class,nil_Features(),filename),
			    Basic,this));

// 
// The Object class has no parent class. Its methods are
//        cool_abort() : Object    aborts the program
//        type_name() : Str        returns a string representation of class name
//        copy() : SELF_TYPE       returns a copy of the object
//
// There is no need for method bodies in the basic classes---these
// are already built in to the runtime system.
//
  install_class(
   new CgenNode(
    class_(Object, 
	   No_class,
	   append_Features(
           append_Features(
           single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
           single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
           single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	   filename),
    Basic,this));
 //Installed Object
// 
// The IO class inherits from Object. Its methods are
//        out_string(Str) : SELF_TYPE          writes a string to the output
//        out_int(Int) : SELF_TYPE               "    an int    "  "     "
//        in_string() : Str                    reads a string from the input
//        in_int() : Int                         "   an int     "  "     "
//
   install_class(
    new CgenNode(
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
	   filename),	    
    Basic,this));
 //Installed IO
//
// The Int class has no methods and only a single attribute, the
// "val" for the integer. 
//
   install_class(
    new CgenNode(
     class_(Int, 
	    Object,
            single_Features(attr(val, prim_slot, no_expr())),
	    filename),
     Basic,this));
 //Installed Int
//
// Bool also has only the "val" slot.
//
    install_class(
     new CgenNode(
      class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename),
      Basic,this));
 //Installed Bool
//
// The class Str has a number of slots and operations:
//       val                                  ???
//       str_field                            the string itself
//       length() : Int                       length of the string
//       concat(arg: Str) : Str               string concatenation
//       substr(arg: Int, arg2: Int): Str     substring
//       
   install_class(
    new CgenNode(
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
	     filename),
        Basic,this));
 //Installed String
}

// CgenClassTable::install_class
// CgenClassTable::install_classes
//
// install_classes enters a list of classes in the symbol table.
//
void CgenClassTable::install_class(CgenNodeP nd)
{
  Symbol name = nd->get_name();
  if (probe(name))
    {
      return;
    }

  // The class name is legal, so add it to the list of classes
  // and the symbol table.
  nds = new List<CgenNode>(nd,nds); //extend list of classes. Prepend current class
  addid(name,nd); //add this class to the symbol table
}

void CgenClassTable::install_classes(Classes cs)
{
  for(int i = cs->first(); cs->more(i); i = cs->next(i))
    install_class(new CgenNode(cs->nth(i),NotBasic,this));
}

//
// CgenClassTable::build_inheritance_tree
//
void CgenClassTable::build_inheritance_tree()
{
  for(List<CgenNode> *l = nds; l; l = l->tl())
      set_relations(l->hd());
}

//
// CgenClassTable::set_relations
//
// Takes a CgenNode and locates its, and its parent's, inheritance nodes
// via the class table.  Parent and child pointers are added as appropriate.
//
void CgenClassTable::set_relations(CgenNodeP nd)
{
  CgenNode *parent_node = probe(nd->get_parent());
  nd->set_parentnd(parent_node);
  parent_node->add_child(nd);
}

void CgenNode::add_child(CgenNodeP n)
{
  children = new List<CgenNode>(n,children);
}

void CgenNode::set_parentnd(CgenNodeP p)
{
  assert(parentnd == NULL);
  assert(p != NULL);
  parentnd = p;
}

void CgenNode::code(ostream &s,CgenClassTable* table)
{
  table->currentNode = this; //store the current CgenNode
  for(int i=features->first();features->more(i);i=features->next(i))
  {
    features->nth(i)->code(s,table);
  }
}

void CgenClassTable::code_program_methods()
{
  CgenNode* node = NULL;
  for(List<CgenNode>* l = nds;l;l=l->tl())
  {
    node = l->hd();
    if (node->basic()) continue; //ignore basic classes which are implemented in the runtime system
    node->code(str,this);
  }
}

void CgenClassTable::code()
{
  //Output of .data information
  if (cgen_debug) cout << "coding global data" << endl;
  code_global_data();

  if (cgen_debug) cout << "choosing gc" << endl;
  code_select_gc();

  if (cgen_debug) cout << "coding constants" << endl;
  code_constants();

  if (cgen_debug) cout << "coding class name table" << endl;
  code_class_name_tab();

  if (cgen_debug) cout << "coding class object table" << endl;
  code_class_obj_table();
  
  if (cgen_debug) cout << "coding prototype objects" << endl;
  code_prototype_objects();

  if (cgen_debug) cout << "coding dispatch tables" << endl;
  code_dispatch_tables();

  //Output of text information
  if (cgen_debug) cout << "coding global text" << endl;
  code_global_text();

  if (cgen_debug) cout << "coding class initialization methods" << endl;
  code_class_init_methods();

  if (cgen_debug) cout << "coding program methods" << endl;
  code_program_methods();
}

int CgenClassTable::findClassTag(Symbol sym)
{
  std::string strsym(sym->get_string(),sym->get_len());
  for(unsigned int i=0;i<classNames.size();i++) if (classNames[i].compare(strsym)==0) return i;
  return -1;
}

CgenNodeP CgenClassTable::root()
{
   return probe(Object);
}


///////////////////////////////////////////////////////////////////////
//
// CgenNode methods
//
///////////////////////////////////////////////////////////////////////

CgenNode::CgenNode(Class_ nd, Basicness bstatus, CgenClassTableP ct) :
   class__class((const class__class &) *nd),
   parentnd(NULL),
   children(NULL),
   basic_status(bstatus)
{ 
   stringtable.add_string(name->get_string());          // Add class name to string table
   classTable = ct;
}


//******************************************************************
//
//   Fill in the following methods to produce code for the
//   appropriate expression.  You may add or remove parameters
//   as you wish, but if you do, remember to change the parameters
//   of the declarations in `cool-tree.h'  Sample code for
//   constant integers, strings, and booleans are provided.
//
//*****************************************************************

void method_class::code(ostream &s,CgenClassTable* table){
  //Emit method label
  emit_method_ref(table->currentNode->name,name,s);
  s << LABEL;
  //Generate method code
  setUpCallee(s);
  table->currentNode->locations->enterscope();
  Formal f; //name, type_decl
  for(int i=formals->first();formals->more(i);i=formals->next(i))
  {
    f = formals->nth(i);
    table->currentNode->locations->addid(f->getName(),new Location(FP,FRAME_OFFSET+formals->len()-i-1));
  }
  expr->code(s,table);
  table->currentNode->locations->exitscope();
  tearDownCallee(formals->len(),s);
}

void class__class::code(ostream &s, CgenClassTable* table){};

void assign_class::code(ostream &s, CgenClassTable* table)
{
  expr->code(s,table);
  int offset = table->currentNode->getFeatureOffsetByName(name,true);
  emit_store(ACC,offset+DEFAULT_OBJFIELDS,SELF,s);
}

void dispatch_handler(Expression expr, Symbol type_name, Symbol name, Expressions actual,CgenClassTable* table, ostream &s)
{
  //Evaluate actual function parameters
  for(int i=actual->first();actual->more(i);i=actual->next(i))
  {
    actual->nth(i)->code(s,table);
    emit_push(ACC,s); //push actual params on stack for callee
  }
  //Evaluate expr
  expr->code(s,table); //current expr object in ACC after this line
  //Check if dispatch on void
  emit_bne(ACC,ZERO,curLabel,s);
  StringEntry* entry = stringtable.lookup_string(table->currentNode->get_filename()->get_string());
  emit_load_string(ACC,entry,s);
  emit_load_imm(T1,table->currentNode->get_line_number(),s);
  emit_jal("_dispatch_abort",s); //dispatch abort expects line number in T1
  emit_label_def(curLabel,s);
  curLabel++;
  if (type_name==No_type)
  {
    //Dynamic dispatch
    emit_load(T1,DISPTABLE_OFFSET,ACC,s); //load dispatch table reference into T1
    type_name = expr->get_type();
  }
  else
  {
    //Static dispatch
    emit_partial_load_address(T1,s); //Load dispatch table reference into T1
    emit_disptable_ref(type_name,s);
    s << endl;
  }
  //Look up method offset and jump to function
  CgenNode* classNode = table->getClassByName(type_name);
  int offset = classNode->getFeatureOffsetByName(name,true);
  emit_load(T1,offset,T1,s);
  emit_jalr(T1,s);
}

void static_dispatch_class::code(ostream &s, CgenClassTable* table)
{
  dispatch_handler(expr,type_name,name,actual,table,s);
}

void dispatch_class::code(ostream &s, CgenClassTable* table)
{
  dispatch_handler(expr,No_type,name,actual,table,s);
}

void cond_class::code(ostream &s, CgenClassTable* table)
{
  pred->code(s,table);
  emit_load_bool(T1,truebool,s);
  int label = curLabel;
  curLabel+=2; //we need two new labels
  //Check if pred is true or false => branch
  emit_beq(ACC,T1,label,s);
  //Fall through if false
  else_exp->code(s,table);
  emit_branch(label+1,s); //Jump over false code branch
  //Label true: Land here if true
  emit_label_ref(label,s);
  then_exp->code(s,table);
  emit_label_ref(label+1,s);
}

void loop_class::code(ostream &s, CgenClassTable* table)
{
  int label = curLabel;
  curLabel+=2;
  emit_label_def(label,s);
  pred->code(s,table);
  emit_load_bool(T1,truebool,s);
  emit_bne(ACC,T1,label+1,s); //if predicate does not hold: branch and quit loop
  body->code(s,table);
  emit_branch(label,s);
  emit_label_def(label+1,s);
  emit_load_imm(ACC,0,s);
}

void typcase_class::code(ostream &s, CgenClassTable* table) {
}

void block_class::code(ostream &s, CgenClassTable* table)
{
  for(int i=body->first();body->more(i);i=body->next(i)) body->nth(i)->code(s,table);
}

void let_class::code(ostream &s, CgenClassTable* table) {
}

void loadTwoInts(Expression e1, Expression e2, ostream &s, CgenClassTable* table)
{
  e1->code(s,table);
  emit_fetch_int(ACC,ACC,s);
  emit_push(ACC,s); //also changes SP: emit_addiu(SP,SP,-4,s)
  e2->code(s,table);
  emit_fetch_int(T2,ACC,s);
  emit_load(T1,1,SP,s);
  emit_addiu(SP,SP,4,s);
  emit_instantiate(s); //create new int object (ACC contains Int object)
}

void plus_class::code(ostream &s, CgenClassTable* table)
{
  loadTwoInts(e1,e2,s,table); //ints end up in T1, T2. A new Int is in ACC
  emit_addu(T1,T1,T2,s);
  emit_store_int(T1,ACC,s);
}

void sub_class::code(ostream &s, CgenClassTable* table)
{
  loadTwoInts(e1,e2,s,table);
  emit_sub(T1,T1,T2,s);
  emit_store_int(T1,ACC,s);
}

void mul_class::code(ostream &s, CgenClassTable* table)
{
  loadTwoInts(e1,e2,s,table);
  emit_mul(T1,T1,T2,s);
  emit_store_int(T1,ACC,s);
}

void divide_class::code(ostream &s, CgenClassTable* table)
{
  loadTwoInts(e1,e2,s,table);
  emit_div(T1,T1,T2,s);
  emit_store_int(T1,ACC,s);
}

void neg_class::code(ostream &s, CgenClassTable* table)
{
  e1->code(s,table);
  emit_instantiate(s);
  emit_fetch_int(T1,ACC,s);
  emit_neg(T1,T1,s);
  emit_store_int(T1,ACC,s);
}

void trueOrFalseBoolResult(ostream &s)
{
  //Store false boolean in ACC
  emit_load_bool(ACC,falsebool,s);
  emit_branch(curLabel+1,s);
  emit_label_def(curLabel,s);
  //Otherwise store true boolean in ACC
  emit_load_bool(ACC,truebool,s);
  emit_label_def(curLabel+1,s);
  curLabel+=2;
}

void lt_class::code(ostream &s, CgenClassTable* table)
{
  loadTwoInts(e1,e2,s,table);
  emit_blt(T1,T2,curLabel,s);
  trueOrFalseBoolResult(s);
}

void eq_class::code(ostream &s, CgenClassTable* table)
{
  e1->code(s,table);
  emit_push(ACC,s); //push on to stack
  e2->code(s,table);
  emit_move(T2,ACC,s);
  emit_load(T1,1,SP,s);
  emit_addiu(SP,SP,WORD_SIZE,s);
  //First check if the pointers of the two objects are equal
  emit_beq(T1,T2,curLabel,s); //If equal jump over equality_test
  emit_load_bool(ACC,truebool,s);
  emit_load_bool(A1,falsebool,s);
  emit_jal("equality_test",s); //if equal ACC is returned otherwise A1
  emit_label_def(curLabel,s);
  curLabel++;
}

void leq_class::code(ostream &s, CgenClassTable* table)
{
  loadTwoInts(e1,e2,s,table);
  emit_bleq(T1,T2,curLabel,s);
  trueOrFalseBoolResult(s);
}

void comp_class::code(ostream &s, CgenClassTable* table)
{
  int label = curLabel;
  curLabel+=2;
  e1->code(s,table);
  emit_load_bool(T1,truebool,s);
  emit_beq(ACC,T1,label,s);
  //False in ACC => convert to true
  emit_load_bool(ACC,truebool,s);
  emit_branch(label+1,s);
  //True in ACC => convert to false
  emit_label_def(label,s);
  emit_load_bool(ACC,falsebool,s);
  emit_label_def(label+1,s);
}

void int_const_class::code(ostream& s, CgenClassTable* table)  
{
  //
  // Need to be sure we have an IntEntry *, not an arbitrary Symbol
  //
  emit_load_int(ACC,inttable.lookup_string(token->get_string()),s);
}

void string_const_class::code(ostream& s, CgenClassTable* table)
{
  emit_load_string(ACC,stringtable.lookup_string(token->get_string()),s);
}

void bool_const_class::code(ostream& s, CgenClassTable* table)
{
  emit_load_bool(ACC, BoolConst(val), s);
}

void new__class::code(ostream &s, CgenClassTable* table)
{
  if (type_name==SELF_TYPE)
  {
    //Get the class tag
    emit_load(T1,0,SELF,s);
    //Find the init method of this class in class_objTab => word position = classTag*2
    emit_load_imm(T2,2,s);
    emit_mul(T1,T2,T1,s);
    emit_load_address(T2,CLASSOBJTAB,s);
    emit_addu(T1,T2,T1,s);
    emit_load(ACC,0,T1,s); //load reference to prototype object into ACC
    //Create a new instance of the class
    emit_instantiate(s);
    //Call initialization method
    emit_load(T1,1,T2,s);
    s << JALR << T1 << endl;
  }
  else if (type_name==Bool) emit_load_bool(ACC,falsebool,s);
  else
  {
    //Use the type name to call the copy method and the init method
    emit_partial_load_address(ACC,s);
    emit_protobj_ref(type_name,s);
    s << endl;
    emit_instantiate(s);
    s << JAL;
    emit_init_ref(type_name,s);
    s << endl;
  }
}

void isvoid_class::code(ostream &s, CgenClassTable* table)
{
  int label = curLabel;
  curLabel+=2;
  e1->code(s,table);
  emit_beq(ACC,ZERO,label,s);
  emit_load_bool(ACC,falsebool,s);
  emit_branch(label+1,s);
  emit_label_def(label,s);
  emit_load_bool(ACC,truebool,s);
  emit_label_def(label+1,s);
}

void no_expr_class::code(ostream &s, CgenClassTable* table)
{
  emit_load_imm(ACC,0,s);
}

void object_class::code(ostream &s, CgenClassTable* table)
{
  if (name==self) emit_move(ACC,SELF,s);
  else
  {
    Location* loc = table->currentNode->locations->lookup(name);
    emit_load(ACC,loc->getOffset(),loc->getRegister(),s);
  }
}


