%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

int lines;
int pos_start;
int pos_end;

void pass_node(char *text){
     yylval.node = new_syntax_tree_node(text);
}

/*****************声明和选项设置  end*****************/

%}

digit [0-9]
INTEGERS {digit}+
FLOATS ({digit}+".")|({digit}*"."{digit}+)

letter [a-zA-Z]
IDENTIFIER {letter}+

%x COMMENT

%%
 /* to do for students */
 /* two cases for you, pass_node will send flex's token to bison */
\+ 	     {pos_start = pos_end; pos_end += 1; pass_node(yytext); return ADD;}
\- 	     {pos_start = pos_end; pos_end += 1; pass_node(yytext); return SUB;}
\* 	     {pos_start = pos_end; pos_end += 1; pass_node(yytext); return MUL;}
\/ 	     {pos_start = pos_end; pos_end += 1; pass_node(yytext); return DIV;}
\<        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return LT;}
\<\=      {pos_start = pos_end; pos_end += 2; pass_node(yytext); return LTE;}
\>        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return GT;}
\>\=      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return GTE;}
\=\=      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return EQ;}
\!\=      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return NEQ;}
\=        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return ASSIN;}
\;        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return SEMICOLON;}
\,        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return COMMA;}
\(        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return LPARENTHESE;}
\)        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return RPARENTHESE;}
\[        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return LBRACKET;}
\]        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return RBRACKET;}
\{        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return LBRACE;}
\}        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return RBRACE;}

\t           {pos_start = pos_end; pos_end += strlen(yytext);  }
\n           {lines++; pos_start=1;pos_end=1; }
" "          {pos_start = pos_end; pos_end += 1;}

else      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return ELSE;}
if        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return IF;}
int       {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INT;}
return    {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return RETURN;}
void      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return VOID;}
while     {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return WHILE;}
float     {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOAT;}

{IDENTIFIER} {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return IDENTIFIER;}
{INTEGERS}   {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTEGER;}
{FLOATS}     {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATPOINT;}

"/*"         BEGIN(COMMENT);
    <COMMENT>[^*\n]*        /* 吃掉注释中的内容 */
    <COMMENT>"*"+[^*/\n]*   /* 若不是* /则继续吃掉注释中的内容 */
    <COMMENT>\n             lines++; /* 若是换行符则行数加一 */
    <COMMENT>"*"+"/"        BEGIN(INITIAL); /* 正确处理comments的情况 */

 /****请在此补全所有flex的模式与动作  end******/
%%
