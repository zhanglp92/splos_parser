#include "pcl.h"
#include "output.h"
#include "ast.h"
#include "decl.h"
#include "gen.h"

/* 三地址码 */
#define DST  inst->opds[0]
#define SRC1 inst->opds[1]
#define SRC2 inst->opds[2]

static char *OPCodeNames[] = {

    #define OPCODE(code, name, func) name,
    #include "opcode.h"
    #undef OPCODE
};

/* 打印三地址码 */
static void DAssemUIL (IRInst inst)
{
	int op = inst->opcode;

	fprintf (IRFile, "\t");
	switch (op) {

        /* 算数运算 */
        /* |, ^, &, <<, >>, +, -, *, /, % */
    	case BOR:   case BXOR:  case BAND:  case LSH: 
    	case RSH:   case ADD:   case SUB:   case MUL: 
	    case DIV:   case MOD: {
		
            fprintf (IRFile, "%s = %s %s %s", DST->name, SRC1->name, OPCodeNames[op], SRC2->name);
        }break;

        /* ++, -- */
	    case INC:   case DEC: {
		
            fprintf (IRFile, "%s%s", OPCodeNames[op], DST->name);
        }break;

        /* ~, -, &, * */
	    case BCOM:  case NEG:   case ADDR:  case DEREF: {

		    fprintf (IRFile, "%s = %s%s", DST->name, OPCodeNames[op], SRC1->name);
        }break;

        /* 赋值运算 */
        /* = */
    	case MOV: {
		
            fprintf (IRFile, "%s = %s", DST->name, SRC1->name);
        }break;

        /* 间接赋值 */
        /* *= */
    	case IMOV: {
		
            fprintf (IRFile, "*%s = %s", DST->name, SRC1->name);
        }break;

        /* 比较运算 */
        /* ==, !=, >, <, >=, <= */
    	case JE:    case JNE:   case JG:    case JL:
	    case JGE:   case JLE: {

		    fprintf (IRFile, "if (%s %s %s) goto %s", SRC1->name, OPCodeNames[op],
			    SRC2->name, ((BBlock)DST)->sym->name);
        }break;

        /* 等于0 跳转 */
    	case JZ: {
		
            fprintf (IRFile, "if (! %s) goto %s", SRC1->name, ((BBlock)DST)->sym->name);
        }break;

        /* 非0 跳转 */
    	case JNZ: {
		
            fprintf (IRFile, "if (%s) goto %s", SRC1->name, ((BBlock)DST)->sym->name);
        }break;

        /* 跳转 */
    	case JMP: {
		
            fprintf (IRFile, "goto %s", ((BBlock)DST)->sym->name);
        }break;

        /* 间接跳转 */
    	case IJMP: {

			BBlock *p = (BBlock *)DST;

			fprintf (IRFile, "goto (");
			for ( ; *p; p++) {

				fprintf (IRFile, "%s,",  (*p)->sym->name);
			}
			fprintf (IRFile, ")[%s]", SRC1->name);
		}break;

        /* 函数调用 */
    	case CALL: {

			ILArg   arg;
			Vector  args = (Vector)SRC2;
			int i;

            /* 函数调用是的赋值 */
			if (DST != NULL) {

				fprintf (IRFile, "%s = ", DST->name);
			}

            if (inst->callNum)
    			fprintf (IRFile, "%s[%d](", SRC1->name, inst->callNum);
            else 
    			fprintf (IRFile, "%s(", SRC1->name);

            /* 实参 */
			for (i = 0; i < LEN(args); ++i) {

				arg = GET_ITEM (args, i);
				if (i != LEN(args) - 1)
					fprintf (IRFile, "%s, ", arg->sym->name);
				else
					fprintf (IRFile, "%s", arg->sym->name);
			}
			fprintf (IRFile, ")");
		}break;

        /* 返回值 */
    	case RET: {
		
            fprintf (IRFile, "return %s", DST->name);
        }break;

    	default: {
		
            fprintf (IRFile, "%s = %s%s", DST->name, OPCodeNames[op], SRC1->name);
        }break;
	}
	fprintf (IRFile, ";\n");
}

/* 函数的中间代码打印 */
void DAssemFunction (AstFunction func)
{
	FunctionSymbol  fsym = func->fsym;
	BBlock  bb;
	IRInst  inst;
    int     fParall = 0;

	if (!fsym->defined)
		return;

	fprintf (IRFile, "function %s\n", fsym->name);

    /* 函数内部的所有块 */
	for (bb = fsym->entryBB; bb; bb = bb->next) {

        /* 被引用的块 */
		if ( bb->ref ) {

			fprintf (IRFile, "%s:\n", bb->sym->name);
		}

        if ( !fParall && bb->isParall ) {

            fParall = 1;;
            fprintf (IRFile, "(parallel){ \n");
        }

        /* 打印一个块中的三地址码 */
		for (inst = bb->insth.next; inst != &bb->insth; inst = inst->next) {

			DAssemUIL (inst);
		}

        if ( fParall && bb->next && !bb->next->isParall) {
            
            fParall = 0;
            fprintf (IRFile, "} \n");
        }
	}

	if (fsym->exitBB->npred != 0)
		fprintf (IRFile, "\tret\n");

	fprintf (IRFile, "\n\n");
}

/* 打印中间代码 */
void DAssemTranslationUnit (AstTranslationUnit transUnit)
{
	AstNode p = transUnit->extDecls;

	IRFile = CreateOutput (Input.filename, ".uil");

	for ( ; p; p = p->next) {

		if (p->kind == NK_Function) {

			DAssemFunction ((AstFunction)p);
		}
	}

	fclose (IRFile);
}
