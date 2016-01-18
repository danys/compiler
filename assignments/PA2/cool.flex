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
#include <string>

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
int counter=0;
int nNestedComments=0;
std::string text; //the string to record strings and comments
bool invalidStr=false;

%}

/*
 * Define names for regular expressions here.
 */

DARROWp		          =>
LEp			  <=
ARROWp			  <-
DIGIT			  [0-9]
UALPHA			  [A-Z] 
LALPHA			  [a-z]
ALPHA			  {UALPHA}|{LALPHA}
ALPHAUSCORE		  {ALPHA}|_
ALPHANUMUSCORE		  {ALPHAUSCORE}|{DIGIT}
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
WHITESPACE		  [ \f\r\t\v\32]
TYPEIDp			  {UALPHA}{ALPHANUMUSCORE}*
OBJECTIDp		  {LALPHA}{ALPHANUMUSCORE}*
DOUBLEDASH		  --

%x INCOMMENT INSTRING INONELINECOMMENT
%%

<INITIAL,INCOMMENT>\n	{curr_lineno++;}
{WHITESPACE}		{}
{DIGIT}+		{cool_yylval.symbol=inttable.add_string(yytext,yyleng);return INT_CONST;}

 /* Single line comments */
{DOUBLEDASH}[^\n]*	 	{}

 /*
  *  Nested comments
  */
<INITIAL,INCOMMENT>\(\*		{
   nNestedComments++;
   BEGIN(INCOMMENT);
}

<INCOMMENT>\*\)		{
   nNestedComments--;
   if (nNestedComments==0)
   {
	BEGIN(INITIAL);
   }
   else if (nNestedComments<0)
   {
	BEGIN(INITIAL);
	text.clear();
	nNestedComments=0;
	cool_yylval.error_msg="Unmatched *)";
	return ERROR;
   }
}

<INITIAL>\*\)		{
	cool_yylval.error_msg="Unmatched *)";
	return ERROR;	
}

<INCOMMENT><<EOF>>	{
	BEGIN(INITIAL);
	cool_yylval.error_msg="EOF in comment";
	return ERROR;
}

<INCOMMENT>.		{text += yytext;}

 /*
  *  The multiple-character operators.
  */
{DARROWp}		{ return (DARROW); }
{ARROWp}		{ return ASSIGN;}
{LEp}			{ return LE;}

 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */
{CLASSp}		{return CLASS;}
{ELSEp}			{return ELSE;}
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

{TRUEp}			{cool_yylval.boolean=1;return BOOL_CONST;}
{FALSEp}		{cool_yylval.boolean=0;return BOOL_CONST;}

{TYPEIDp}		{cool_yylval.symbol=idtable.add_string(yytext,yyleng);return TYPEID;}
{OBJECTIDp}		{cool_yylval.symbol=idtable.add_string(yytext,yyleng);return OBJECTID;}

"+"			{return '+';}
"-"			{return '-';}
"*"			{return '*';}
"/"			{return '/';}
"~"			{return '~';}
"<"			{return '<';}
"="			{return '=';}
"("			{return '(';}
")"			{return ')';}
"{"			{return '{';}
"}"			{return '}';}
";"			{return ';';}
":"			{return ':';}
","			{return ',';}
"@"			{return '@';}
"\."			{return '.';}

 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */

 /* Match end of string  */
<INSTRING>\" 		{
   BEGIN(INITIAL);
   if (text.size()>=MAX_STR_CONST){
      cool_yylval.error_msg="String constant too long";
      text.clear(); 
      return ERROR;
   }
   if (!invalidStr)
   {
	cool_yylval.symbol=stringtable.add_string(const_cast<char*>(text.c_str()),text.size());
	text.clear();
	return STR_CONST;
   }
   else
   {
	cool_yylval.error_msg="String contains null character";
	return ERROR;	
   }
}

\"			{BEGIN(INSTRING);invalidStr=false;text.clear();}

<INSTRING>\n		{
   curr_lineno++;
   BEGIN(INITIAL);
   cool_yylval.error_msg="Unterminated string constant";
   return ERROR;
}

<INSTRING>\0		{
			invalidStr=true;
}

 /* Convert \c to c and compact two letter special chars  */
<INSTRING>\\.|\\\n	{
   char c;
   if (yytext[1]=='b') c='\b';
   else if (yytext[1]=='t') c='\t';
   else if (yytext[1]=='n') c='\n';
   else if (yytext[1]=='f') c='\f';
   else if (yytext[1]=='\n') {c=yytext[1];curr_lineno++;}
   else if (yytext[1]=='\0') {invalidStr=true;c='0';}
   else c=yytext[1];
   text += c;
}

<INSTRING><<EOF>>	{BEGIN(INITIAL);cool_yylval.error_msg="EOF in string constant";return ERROR;}

<INSTRING>.		{text += yytext;}

 /* All other unmatches characters result in an error */
.			{cool_yylval.error_msg=yytext;return ERROR;}
%%
