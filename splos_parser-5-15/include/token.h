/*************************************************************************
	> File Name: token.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月17日 星期一 13时45分57秒

	> Description: 
 ************************************************************************/

#ifndef TOKEN
#error "You must define TOKEN macro before include this file"
#endif

/* 所有关键字 */

/* 1 */
TOKEN (TK_AUTO, "auto")
TOKEN (TK_EXTERN,    "extern")
TOKEN (TK_REGISTER,  "register")
TOKEN (TK_STATIC,    "static")
TOKEN (TK_TYPEDEF,   "typedef")
TOKEN (TK_CONST,     "const")
TOKEN (TK_VOLATILE,  "volatile")
TOKEN (TK_SIGNED,    "signed")
TOKEN (TK_UNSIGNED,  "unsigned")
TOKEN (TK_SHORT,     "short")
/* 11 */
TOKEN (TK_LONG,      "long")
TOKEN (TK_CHAR,      "char")
TOKEN (TK_INT,       "int")
TOKEN (TK_INT64,     "__int64")
TOKEN (TK_FLOAT,     "float")
TOKEN (TK_DOUBLE,    "double")
TOKEN (TK_ENUM,      "enum")
TOKEN (TK_STRUCT,    "struct")
TOKEN (TK_UNION,     "union")
TOKEN (TK_VOID,      "void")
/* 21 */
TOKEN (TK_BREAK,     "break")
TOKEN (TK_CASE,      "case")
TOKEN (TK_CONTINUE,  "continue")
TOKEN (TK_DEFAULT,   "default")
TOKEN (TK_DO,        "do")
TOKEN (TK_ELSE,      "else")
TOKEN (TK_FOR,       "for")
TOKEN (TK_GOTO,      "goto")
TOKEN (TK_IF,        "if")
TOKEN (TK_RETURN,    "return")
/* 31 */
TOKEN (TK_SWITCH,    "switch")
TOKEN (TK_WHILE,     "while")
TOKEN (TK_SIZEOF,    "sizeof")
TOKEN (TK_PARALLEL,  "parallel")
TOKEN (TK_IMPORT,    "import")
TOKEN (TK_ID,        "ID")
TOKEN (TK_INTCONST,     "int")
TOKEN (TK_UINTCONST,    "unsigned int")
TOKEN (TK_LONGCONST,    "long")
TOKEN (TK_ULONGCONST,   "unsigned long")
TOKEN (TK_LLONGCONST,   "long long")
/* 42 */
TOKEN (TK_ULLONGCONST,  "unsigned long long")
TOKEN (TK_FLOATCONST,   "float")
TOKEN (TK_DOUBLECONST,  "double")
TOKEN (TK_LDOUBLECONST, "long double")
TOKEN (TK_STRING,       "STR")
TOKEN (TK_WIDESTRING,   "WSTR")
TOKEN (TK_COMMA,         ",")
TOKEN (TK_QUESTION,      "?")
TOKEN (TK_COLON,         ":")
TOKEN (TK_ASSIGN,        "=")
/* 52 */
TOKEN (TK_BITOR_ASSIGN,  "|=")
TOKEN (TK_BITXOR_ASSIGN, "^=")
TOKEN  (TK_BITAND_ASSIGN, "&=")
TOKEN (TK_LSHIFT_ASSIGN, "<<=")
TOKEN (TK_RSHIFT_ASSIGN, ">>=")
TOKEN (TK_ADD_ASSIGN,    "+=")
TOKEN (TK_SUB_ASSIGN,    "-=")
TOKEN (TK_MUL_ASSIGN,    "*=")
TOKEN (TK_DIV_ASSIGN,    "/=")
TOKEN (TK_MOD_ASSIGN,    "%=")
/* 62 */
TOKEN (TK_OR,            "||")
TOKEN (TK_AND,           "&&")
TOKEN (TK_BITOR,         "|")
TOKEN (TK_BITXOR,        "^")
TOKEN (TK_BITAND,        "&")
TOKEN (TK_EQUAL,         "==")
TOKEN (TK_UNEQUAL,       "!=")
TOKEN (TK_GREAT,         ">")
TOKEN (TK_LESS,          "<")
TOKEN (TK_GREAT_EQ,      ">=")
/* 72 */
TOKEN (TK_LESS_EQ,       "<=")
TOKEN (TK_LSHIFT,        "<<")
TOKEN (TK_RSHIFT,        ">>")
TOKEN (TK_ADD,           "+")
TOKEN (TK_SUB,           "-")
TOKEN (TK_MUL,           "*")
TOKEN (TK_DIV,           "/")
TOKEN (TK_MOD,           "%")
TOKEN (TK_INC,           "++")
TOKEN (TK_DEC,           "--")
/* 82 */
TOKEN (TK_NOT,           "!")
TOKEN (TK_COMP,          "~")
TOKEN (TK_DOT,           ".")
TOKEN (TK_POINTER,       "->")
TOKEN (TK_LPAREN,        "(")
TOKEN (TK_RPAREN,        ")")
TOKEN (TK_LBRACKET,      "[")
TOKEN (TK_RBRACKET,      "]")
TOKEN (TK_LBRACE,        "{")
TOKEN (TK_RBRACE,        "}")
/* 92 */
TOKEN (TK_SEMICOLON,     ";")
TOKEN (TK_ELLIPSE,       "...")
TOKEN (TK_POUND,         "#")
TOKEN (TK_NEWLINE,       "\n")
TOKEN (TK_END,           "EOF")
