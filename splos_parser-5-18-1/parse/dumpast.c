/*************************************************************************
	> File Name: dumpast.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月30日 星期日 18时02分24秒

	> Description: 打印语法树
 ************************************************************************/

#include "pcl.h"
#include "ast.h"
#include "output.h"
#include "decl.h"
#include "stmt.h"
#include "expr.h"

/* 打印表达式的语法树 */
static void DumpExpression(AstExpression expr, int pos)
{
    /* 操作名字 */
	char *opname = NULL;

	if (expr == NULL) {
        /* 空语句 */
		fprintf(ASTFile, "NIL");
		return;
	}

    /* 算数逻辑操作符 */
	if (expr->op >= OP_COMMA && expr->op <= OP_POSTDEC) 
		opname = OPNames[expr->op];

	switch (expr->op) {

        /* 双目操作 */
	    case OP_COMMA:          case OP_ASSIGN:         case OP_BITOR_ASSIGN:
	    case OP_BITXOR_ASSIGN:  case OP_BITAND_ASSIGN:  case OP_LSHIFT_ASSIGN:
	    case OP_RSHIFT_ASSIGN:  case OP_ADD_ASSIGN:     case OP_SUB_ASSIGN:
    	case OP_MUL_ASSIGN:     case OP_DIV_ASSIGN:     case OP_MOD_ASSIGN:
    	case OP_OR:     case OP_AND:    case OP_BITOR:  case OP_BITXOR:  
	    case OP_BITAND:	case OP_EQUAL:  case OP_UNEQUAL:case OP_GREAT: 
	    case OP_LESS:   case OP_GREAT_EQ:               case OP_LESS_EQ:
	    case OP_LSHIFT: case OP_RSHIFT: case OP_ADD:    case OP_SUB:
	    case OP_MUL:    case OP_DIV:    case OP_MOD:    case OP_INDEX: {

            /* 打印操作符,及操作的数 */
	    	fprintf (ASTFile, "(%s ", opname);
		    pos += strlen (opname) + 2;
    		DumpExpression (expr->kids[0], pos);
            /* 左对齐 */
	    	LeftAlign (ASTFile, pos);
		    DumpExpression (expr->kids[1], pos);
    		fprintf (ASTFile, ")");
        }break;

        /* 函数调用 */
	    case OP_CALL: {

            /* 实参 */
			AstExpression p = expr->kids[1];

			fprintf (ASTFile, "(%s ", opname);
			pos += strlen(opname) + 2;
            /* 函数名 */
			DumpExpression (expr->kids[0], pos);

			for ( ; p; p = (AstExpression)p->next) {

				LeftAlign (ASTFile, pos);
				DumpExpression (p, pos);
				if ( p->next )
					fprintf (ASTFile, ",");
			}
			fprintf(ASTFile, ")");
		}
		break;

        /* 单目操作 */
	    case OP_PREINC: case OP_PREDEC: case OP_POS:    case OP_NEG:
        case OP_NOT:    case OP_COMP:   case OP_ADDRESS:case OP_DEREF:{

		    fprintf (ASTFile, "(%s ", opname);
		    pos += strlen (opname) + 2;
		    DumpExpression (expr->kids[0], pos);
		    fprintf (ASTFile, ")");
        }break;

        /* 常量 */
	    case OP_CAST: {

		    fprintf (ASTFile, "(%s ", opname);
		    pos += strlen (opname) + 2;
    		fprintf (ASTFile, "%s", TypeToString (expr->ty));
		    LeftAlign (ASTFile, pos);
    		DumpExpression (expr->kids[0], pos);
	    	fprintf (ASTFile, ")");
        }break;

        /* sizeof 运算 */
    	case OP_SIZEOF: {

    		fprintf (ASTFile, "(%s ", opname);
	    	pos += strlen (opname) + 2;
		    if (expr->kids[0]->kind == NK_Expression) 
			    DumpExpression (expr->kids[0], pos);
    		fprintf (ASTFile, ")");
        }break;

        /* 指针 */
	    case OP_MEMBER: case OP_PTR_MEMBER: {

		    fprintf (ASTFile, "(%s ", opname);
    		pos += strlen (opname) + 2;
	    	DumpExpression (expr->kids[0], pos);
		    LeftAlign (ASTFile, pos);
    		fprintf (ASTFile, "%s", ((Field)expr->val.p)->id);
        }break;

        /* 三目运算符 */
	    case OP_QUESTION: {

		    fprintf (ASTFile, "(%s ", opname);
    		pos += strlen (opname) + 2;;
	    	DumpExpression (expr->kids[0], pos);
		    LeftAlign (ASTFile, pos);
		    DumpExpression (expr->kids[1]->kids[0], pos);
	    	LeftAlign (ASTFile, pos);
		    DumpExpression (expr->kids[1]->kids[1], pos);
    		fprintf (ASTFile, ")");
        }break;

        /* 之后自增减 */
	    case OP_POSTINC: case OP_POSTDEC: {

    		DumpExpression (expr->kids[0], pos);
	    	fprintf (ASTFile, "%s", opname);
        }break;

        /* 标示符 */
	    case OP_ID: {

		    fprintf (ASTFile, "%s", ((Symbol)expr->val.p)->name);
        }break;

        /* 字符串 */
	    case OP_STR: {

			int i;
			String str = ((Symbol)expr->val.p)->val.p;

            fprintf (ASTFile, (expr->ty->categ != CHAR ? "L" : "\""));
            for (i = 0; i < str->len; ++i) 
                fprintf (ASTFile, (isprint (str->chs[i]) ? "%c" : "\\x%x"), str->chs[i]);
			fprintf(ASTFile, "\"");
		}break;

        /* 常量 */
    	case OP_CONST: {

			int categ = expr->ty->categ;

			if (categ == INT || categ == LONG || categ == LONGLONG) {

				fprintf (ASTFile, "%d", expr->val.i[0]);
			} else if (categ == UINT || categ == ULONG || categ == ULONGLONG || categ == POINTER) {

				fprintf (ASTFile, "%u", expr->val.i[0]);
			} else if (categ == FLOAT) {

				fprintf (ASTFile, "%g", expr->val.f);
			} else {

				fprintf (ASTFile, "%g", expr->val.d);
			}
		}break;

    	default:
	    	fprintf(ASTFile, "ERR");
		    break;
	}
}

/* 打印一条语句的语法数 */
static void DumpStatement (AstStatement stmt, int pos)
{
	switch (stmt->kind) {

	    case NK_ExpressionStatement: {

	    	DumpExpression (AsExpr(stmt)->expr, pos);
		}break;

	    case NK_LabelStatement: {

    		fprintf (ASTFile, "(label %s:\n", AsLabel(stmt)->id);
	    	LeftAlign (ASTFile, pos + 2);
            /* 标签名称之后的模块 */
		    DumpStatement (AsLabel(stmt)->stmt, pos + 2);
    		LeftAlign (ASTFile, pos);
	    	fprintf (ASTFile, "end-label)");
		}break;

	    case NK_CaseStatement: {

		    fprintf (ASTFile, "(case  ");
            /* case 之后的表达式 */
		    DumpExpression (AsCase(stmt)->expr, pos + 7);
    		LeftAlign(ASTFile, pos + 2);
            /* 表达式之后的模块 */
	    	DumpStatement (AsCase(stmt)->stmt, pos + 2);
    		LeftAlign (ASTFile, pos);
	    	fprintf (ASTFile, "end-case)");
		}break;

    	case NK_DefaultStatement: {

    		fprintf (ASTFile, "(default  ");
            /* default 之后的模块 */
	    	DumpStatement(AsDef(stmt)->stmt, pos + 10);
    		LeftAlign (ASTFile, pos);
	    	fprintf (ASTFile, "end-default)");
		}break;

    	case NK_IfStatement: {

    		fprintf (ASTFile, "(if  ");
            /* if的条件 */
		    DumpExpression (AsIf(stmt)->expr, pos + 5);
    		LeftAlign (ASTFile, pos + 2);
    		fprintf (ASTFile, "(then  ");
    		LeftAlign (ASTFile, pos + 4);
            /* if之后的模块 */
    		DumpStatement (AsIf(stmt)->thenStmt, pos + 4);
    		LeftAlign (ASTFile, pos + 2);
	    	fprintf (ASTFile, "end-then)");

            /* 处理else 之后的模块 */
		    if (AsIf(stmt)->elseStmt != NULL) {

			    LeftAlign (ASTFile, pos + 2);
			    fprintf (ASTFile, "(else  ");
			    LeftAlign (ASTFile, pos + 4);
			    DumpStatement (AsIf(stmt)->elseStmt, pos + 4);
			    LeftAlign (ASTFile, pos + 2);
			    fprintf (ASTFile, "end-else)");
		    }

		    LeftAlign (ASTFile, pos);
    		fprintf (ASTFile, "end-if)");
		}break;

	    case NK_SwitchStatement: {

    		fprintf (ASTFile, "(switch ");
            /* switch 之后的表达式 */
    		DumpExpression (AsSwitch(stmt)->expr, pos + 9);
    		LeftAlign (ASTFile, pos + 2);
            /* switch 之后的模块 */
    		DumpStatement (AsSwitch(stmt)->stmt, pos + 2);
    		LeftAlign (ASTFile, pos);
	    	fprintf (ASTFile, "end-switch)");
		}break;

    	case NK_WhileStatement: {

    		fprintf (ASTFile, "(while  ");
            /* while 的条件 */
	    	DumpExpression (AsLoop(stmt)->expr, pos + 8);
		    LeftAlign (ASTFile, pos + 2);
            /* while的循环体 */
    		DumpStatement (AsLoop(stmt)->stmt, pos + 2);
    		LeftAlign (ASTFile, pos);
    		fprintf (ASTFile, "end-while)");
		}break;

    	case NK_DoStatement: {

    		fprintf (ASTFile, "(do  ");
            /* do 的条件 */
	    	DumpExpression (AsLoop(stmt)->expr, pos + 5);
		    LeftAlign (ASTFile, pos + 2);
            /* do 的循环体 */
    		DumpStatement (AsLoop(stmt)->stmt, pos + 2);
		    LeftAlign (ASTFile, pos);
    		fprintf (ASTFile, ")");
		}break;

	    case NK_ForStatement: {

    		fprintf (ASTFile, "(for  ");
            /* for 的初始化 */
    		DumpExpression (AsFor(stmt)->initExpr, pos + 6);
    		LeftAlign (ASTFile, pos + 6);
            /* for 的条件 */
    		DumpExpression (AsFor(stmt)->expr, pos + 6);
    		LeftAlign (ASTFile, pos + 6);
            /* for 的步长 */
    		DumpExpression (AsFor(stmt)->incrExpr, pos + 6);
    		LeftAlign (ASTFile, pos + 2);
            /* for 循环体 */
    		DumpStatement (AsFor(stmt)->stmt, pos + 2);
    		LeftAlign (ASTFile, pos);
		    fprintf (ASTFile, "end-for)");
		}break;

	    case NK_GotoStatement: {

		    fprintf (ASTFile, "(goto %s)", AsGoto(stmt)->id);
		}break;

    	case NK_ContinueStatement: {

	    	fprintf (ASTFile, "(continue)");
		}break;

    	case NK_BreakStatement: {

    		fprintf (ASTFile, "(break)");
		}break;

	    case NK_ReturnStatement: {

    		fprintf (ASTFile, "(ret ");
    		DumpExpression (AsRet(stmt)->expr, pos + 5);
	    	fprintf (ASTFile, ")");
		}break;

        case NK_ParallelStatement: {

            fprintf (ASTFile, "(parallel ");
            DumpStatement (AsParall(stmt)->stmt, pos + 10);
            fprintf (ASTFile, ")");
        }break;

    	case NK_CompoundStatement: {

            /* 复合句或者是一个模块 */
			fprintf (ASTFile, "{");
			AstNode p = ((AstCompoundStatement)stmt)->stmts;
			for ( ; p; p = p->next) {

				LeftAlign (ASTFile, pos + 2);
				DumpStatement ((AstStatement)p, pos + 2);
				if (p->next != NULL)
					fprintf (ASTFile, "\n");
			}
			LeftAlign (ASTFile, pos);
			fprintf (ASTFile, "}");
		}break;

    	default:
	    	assert(0);
	}
}

/* 打印一个函数的语法树 */
static void DumpFunction (AstFunction func) 
{
    /* 函数名 */
    fprintf (ASTFile, "function %s \n", func->fdec->dec->id);
    /* 函数体(只包含状态语句,不包括变量的定义) */
    DumpStatement (func->stmt, 0);
    fprintf (ASTFile, "\n\n");
}

/* 生成并打印语法树 */
void DumpTranslationUnit (AstTranslationUnit transUnit) 
{
    AstNode p;

    /* 构造文件名,并且以写的方式打开 */
    ASTFile = CreateOutput (Input.filename, ".ast");

    for (p = transUnit->extDecls; p; p = p->next) {
        
        /* 打印函数的语法树 */
        if (NK_Function == p->kind) {

            DumpFunction ((AstFunction)p);
        }
    }
    
    fclose (ASTFile);
}
