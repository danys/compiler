/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>
#include <stdlib.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */
curr_lineno = 1;
int counter=0;
void setStringSymbol()
{
	Entry* sEntry = new Entry(yytext,yyleng,counter++);
	cool_yylval.symbol = sEntry;
}

void addToIntTable()
{
	inttable.add_int(atoi(yytext));
}

void addToStringTable()
{
	stringtable.add_string(yytext,yyleng);
}

void addToIdTable()
{
	idtable.add_string(yytext,yyleng);
}

%}

/*
 * Define names for regular expressions here.
 */

DARROWp		          =>
DIGIT			  [0-9]
UALPHA			  [A-Z] 
LALPHA			  [a-z]
ALPHA			  {UALPHA}|{LALPHA}
ALPHAUSCORE		  {ALPHA}|_
CLASSp			  [cC][lL][aA][sS]{2}
ELSEp			  [eE][lL][sS][eE]
FALSEp			  f[aA][lL][sS][eE]
FIp			  [fF][iI]
IFp			  [iI][fF]
INp			  [iI][nN]
INHERITSp		  [iI][nN][hH][eE][rR][iI][tT][sS]
ISVOIDp			  [iI][sS][vV][oO][iI][dD]
LETp			  [lL][eE][tT]
LOOPp			  [lL][oO]{2}[pP]
POOLp			  [pP][oO]{2}[lL]
THENp			  [tT][hH][eE][nN]
WHILEp			  [wW][hH][iI][lL][eE]
CASEp			  [cC][aA][sS][eE]
ESACp			  [eE][sS][aA][cC]
NEWp			  [nN][eE][wW]
OFp			  [oO][fF]
NOTp			  [nN][oO][tT]
TRUEp			  t[rR][uU][eE]
WHITESPACE		  [ \f\r\t\v]
TYPEIDp			  {UALPHA}{ALPHAUSCORE}*
OBJECTIDp		  {LALPHA}{ALPHAUSCORE}*
STRINGp			  "[^"]*"

%%

\n			{curr_lineno++;setStringSymbol();addToStringTable();return STR_CONST;}
{WHITESPACE}		{setStringSymbol();addToStringTable();return STR_CONST;}
{DIGIT}+		{setStringSymbol();addToIntTable();return INT_CONST;}


 /*
  *  Nested comments
  */


 /*
  *  The multiple-character operators.
  */
{DARROW}		{ return (DARROW); }

 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */
{CLASSp}		{return CLASS;}
{ELSEp}			{return ELSE;}
{FALSEp}		{return FALSE;}
{FIp}			{return FI;}
{IFp}			{return IF;}
{INp}			{return IN;}
{INHERITSp}		{return INHERITS;}
{ISVOIDp}		{return ISVOID;}
{LETp}			{return LET;}
{LOOPp}			{return LOOP;}
{POOLp}			{return POOL;}
{THENp}			{return THEN;}
{WHILEp}		{return WHILE;}
{CASEp}			{return CASE;}
{ESACp}			{return ESAC;}
{NEWp}			{return NEW;}
{OFp}			{return OF;}
{NOTp}			{return NOT;}
{TRUEp}			{return TRUE;} 

{TYPEIDp}		{setStringSymbol();addToStringTable();return TYPEID;}
{OBJECTIDp}		{setStringSymbol();addToStringTable();return OBJECTID;}
 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */


%%
