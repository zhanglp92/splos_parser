/*************************************************************************
	> File Name: declchk.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月19日 星期三 22时16分13秒

	> Description: C 语言声明语义检查
 ************************************************************************/

#include "pcl.h"
#include "ast.h"
#include "decl.h"
#include "expr.h"
#include "stmt.h"

/* 当前的函数 */
AstFunction     CURRENTF;
/* 当前函数符号表 */
FunctionSymbol  FSYM;

/* 检查语句的语义 */
static void CheckDeclarationSpecifiers (AstSpecifiers specs);
static void CheckDeclarator(AstDeclarator dec);
static Type DeriveType (TypeDerivList tyDrvList, Type ty);
static void CheckInitializer (AstInitializer init, Type ty);
static void CheckInitConstant (AstInitializer init);

/* 检查地址初始化数据
 * e.g. 
 * int a;
 * int b = &a */
static AstExpression CheckAddressConstant (AstExpression expr)
{
	AstExpression addr;
	AstExpression p;
	int offset = 0;

    /* 判断是否为指针类型的表达式 */
	if (!IsPtrType (expr->ty))
		return NULL;

    /* 指针的加减操作 */
	if (expr->op == OP_ADD || expr->op == OP_SUB) {

		addr = CheckAddressConstant (expr->kids[0]);
        /* 操作数1必须是地址,第二个操作数必须是常量 */
		if (addr == NULL || expr->kids[1]->op != OP_CONST)
			return NULL;
		
		expr->kids[0] = addr->kids[0];
        /* 操作数2,常量的值 */
		expr->kids[1]->val.i[0] += (expr->op == OP_ADD ? 1 : -1) * addr->kids[1]->val.i[0];

		return expr;
	}

    /* 指针的赋值操作是取地址 */
	if (expr->op == OP_ADDRESS)
		addr = expr->kids[0];
	else
		addr = expr;

    /* 下标或者点操作 */
	while (addr->op == OP_INDEX || addr->op == OP_MEMBER) {

        /* 下标操作 */
		if (addr->op == OP_INDEX) {

			if (addr->kids[1]->op != OP_CONST)
				return NULL;
			offset += addr->kids[1]->val.i[0] * addr->ty->size;
		} else {

			Field fld = addr->val.p;

			offset += fld->offset;
		}
		addr = addr->kids[0];
	}

	if (addr->op != OP_ID || (expr->op != OP_ADDRESS && !addr->isarray && !addr->isfunc))
		return NULL;

	((Symbol)addr->val.p)->ref++;
    /* 构造地址表达式 */
	CREATE_AST_NODE (p, Expression);
	p->op   = OP_ADD;
	p->ty   = expr->ty;
	p->kids[0] = addr;
	p->kids[1] = Constant (addr->coord, T(INT), p->val);

	return p;
}

#if 1

/* 检查typedef 类型别名 */
/* 检查类型, 重定义, 添加 */
static void CheckTypedef (AstDeclaration decl)
{
    /* typedef 定义只有类型名和别名两部分 */
	AstInitDeclarator initDec;
	Type    ty;
	Symbol  sym = NULL;

    /* 检查初始化的模块 */
	initDec = (AstInitDeclarator)decl->initDecs;
	for (; initDec; initDec = (AstInitDeclarator)initDec->next) {

        /* 检查第二部分的语义类型之后的部分 */
		CheckDeclarator (initDec->dec);
        /* 别名 */
		if (initDec->dec->id == NULL)
			continue;

        /* 类型 */
		ty = DeriveType (initDec->dec->tyDrvList, decl->specs->ty);
		if (ty == NULL) {

			Error (&initDec->coord, "Illegal type");
			ty = T(INT);
		}

        /* typedef 不能有初始化模块 */
		if (initDec->init) {

			Error (&initDec->coord, "Can't initialize typedef name");
		}

		sym = LookupID (initDec->dec->id);
		if (sym && sym->level == Level && (sym->kind != SK_TypedefName || !IsCompatibleType (ty, sym->ty))) {

			Error (&initDec->coord, "Redeclaration of %s", initDec->dec->id);
		} else {

			AddTypedefName (initDec->dec->id, ty);
		}
	}
}

/* 检查类型名字 */
Type CheckTypeName (AstTypeName tname)
{
	Type ty;

	CheckDeclarationSpecifiers (tname->specs);
	CheckDeclarator (tname->dec);
	ty = DeriveType (tname->dec->tyDrvList, tname->specs->ty);
	if (ty == NULL) {

		Error(&tname->coord, "Illegal type");
		ty = T(INT);
	}
	return ty;
}

/* 检查全局描述符 */
/* 检查 typedef 定义 */
static void CheckGlobalDeclaration (AstDeclaration decl)
{
	AstInitDeclarator initDec;
	Type    ty;
	Symbol  sym;
	int     sclass;

    /* 第一部分检查 */
	CheckDeclarationSpecifiers (decl->specs);
	if (decl->specs->sclass == TK_TYPEDEF) {

        /* 检查typedef 别名的语义 */
		CheckTypedef (decl);
		return;
	}

	ty = decl->specs->ty;
	sclass = decl->specs->sclass;
    /* 全局变量不能被register和auto修饰 */
	if (sclass == TK_REGISTER || sclass == TK_AUTO) {

		Error(&decl->coord, "Invalid storage class");
		sclass = TK_EXTERN;
	}

	initDec = (AstInitDeclarator)decl->initDecs;
	for ( ; initDec; initDec = (AstInitDeclarator)initDec->next) {

        /* 检查第二部分的语义 */
		CheckDeclarator (initDec->dec);
		if (initDec->dec->id == NULL)
            continue;

		ty = DeriveType (initDec->dec->tyDrvList, decl->specs->ty);
		if (ty == NULL) {

			Error(&initDec->coord, "Illegal type");
			ty = T(INT);
		}

		if (initDec->dec->kind == NK_NameDeclarator && IsFunctionType (ty)) {

			ty = PointerTo (ty);
		}
    
        /* 是函数定义 */
		if (IsFunctionType(ty)) {

			if (initDec->init) {

                /* 函数定义没有初始化模块 */
				Error(&initDec->coord, "please don't initalize function");
			}

			if ((sym = LookupID (initDec->dec->id)) == NULL) {

                /* 查找函数名并添加 */
				sym = AddFunction (initDec->dec->id, ty, sclass == 0 ? TK_EXTERN : sclass);
			} else {

                /*  函数已经定义 */
				if (sym->sclass == TK_EXTERN && sclass == TK_STATIC) {

					Error(&decl->coord, "Conflict linkage of function %s", initDec->dec->id);
				}

				if (!IsCompatibleType (ty, sym->ty)) {

					Error(&decl->coord, "Incomptabile with previous declaration");
				} else {

					sym->ty = CompositeType (ty, sym->ty);
				}
			}
            continue;
		}

		if ((sym = LookupID (initDec->dec->id)) == NULL) {

            /* 查找变量并添加(非函数即变量) */
			sym = AddVariable (initDec->dec->id, ty, sclass);
		}

		if (initDec->init) {

            /* 初始化 */
			CheckInitializer (initDec->init, ty);
			CheckInitConstant (initDec->init);
		}

        /* 类型的大小无法判断 */
		if (ty->size == 0 && ! (sclass != TK_STATIC && ty->categ == ARRAY) && 
			! (sclass == TK_EXTERN && IsRecordType(ty)))
		{
			Error(&initDec->coord, "Unknown size of %s", initDec->dec->id);
			ty = T(INT);
		}

		
		sclass = sclass == TK_EXTERN ? sym->sclass : sclass;

		if ((sclass == 0 && sym->sclass == TK_STATIC) || (sclass != 0 && sym->sclass != sclass)) {

			Error(&decl->coord, "Conflict linkage of %s", initDec->dec->id);
		}
		if (sym->sclass == TK_EXTERN)
			sym->sclass = sclass;

		if (! IsCompatibleType (ty, sym->ty)) {

			Error(&decl->coord, "Incompatible with previous definition", initDec->dec->id);
            continue;
		} else {

			sym->ty = CompositeType (sym->ty, ty);
		}

		if (initDec->init) {

			if (sym->defined) {

				Error(&initDec->coord, "redefinition of %s", initDec->dec->id);
			} else {

				AsVar(sym)->idata = initDec->init->idata;
				sym->defined = 1;
			}
		}
	}
}

#endif

/* 检查静态变量赋值时的赋值内容 
 * 静态变量的初始化数据只能是常量 */
static void CheckInitConstant (AstInitializer init)
{
	InitData initd = init->idata;

	for (; initd; initd = initd->next) {

        /* 判断是否为常量,字符串,地址 */
		if (! (initd->expr->op == OP_CONST || initd->expr->op == OP_STR ||
		       (initd->expr = CheckAddressConstant (initd->expr))))
		{
			Error (&init->coord, "Initializer must be constant");
		}
	}
	return;
}


/* 两个表达式做按位或操作 */
static AstExpression BORBitField (AstExpression expr1, AstExpression expr2)
{
	AstExpression   bor;

	if (expr1->op == OP_CONST && expr2->op == OP_CONST) {

        /* 常量直接做按位或的操作 */
		expr1->val.i[0] |= expr2->val.i[0];
		return expr1;
	}
	
    /* 构造按位或表达式 */
	CREATE_AST_NODE(bor, Expression);

	bor->coord  = expr1->coord;
	bor->ty     = expr1->ty;
	bor->op     = OP_BITOR;
	bor->kids[0] = expr1;
	bor->kids[1] = expr2;

	return bor;
}


/* 计算位段字段 */
static AstExpression PlaceBitField (Field fld, AstExpression expr)
{
	AstExpression   lsh;
	union value     val;

	if (expr->op == OP_CONST) {

		expr->val.i[0] <<= fld->pos;
		return expr;
	}

	CREATE_AST_NODE (lsh, Expression);

	lsh->coord  = expr->coord;
	lsh->ty     = expr->ty;
	lsh->op     = OP_LSHIFT;
	lsh->kids[0] = expr;
	val.i[1]    = 0;
	val.i[0]    = fld->pos;
    /* 构造一个常量表达式 */
	lsh->kids[1] = Constant (expr->coord, T(INT), val);

	return lsh;
}

/* 检查初始化数据是否正确 */
static AstInitializer CheckInitializerInternal (InitData *tail, AstInitializer init, Type ty, int *offset, int *error)
{
	AstInitializer p;
	int         size = 0;
	InitData    initd;

    /* 基本类型和指针类型 */
	if (IsScalarType (ty)) {

		p = init;
		if (init->lbrace) {

            /* 基本类型不需要或括号初始化 */
			Error (&init->coord, "Can't use brace-enclosed initializer list for scalar");
			*error = 1;
			p = (AstInitializer)init->initials;
		}
		
        /* 调整表达式 */
		p->expr = Adjust (CheckExpression (p->expr), 1);
        /* 检查类型和表达式能否正确匹配 */
		if (!CanAssign (ty, p->expr)) {

			Error (&init->coord, "Wrong initializer");
			*error = 1;
		} else {

            /* 强制类型转换 */
			p->expr = Cast (ty, p->expr);
		}

		CALLOC (initd);
		initd->offset   = *offset;
		initd->expr     = p->expr;
		initd->next     = NULL;
		(*tail)->next   = initd;
		*tail = initd;

		return (AstInitializer)init->next;
	} else if (ty->categ == UNION) {

        /* 处理公用体,的初始化 */
		p = init->lbrace ? (AstInitializer)init->initials : init;
        /* 字段对应的类型 */
		ty = ((RecordType)ty)->flds->ty;

        /* 递归检查 */
		p = CheckInitializerInternal (tail, p, ty, offset, error);

		if (init->lbrace) {

			if (p != NULL) {

				Error(&init->coord, "too many initializer for union");
			}
			return (AstInitializer)init->next;
		}
		return p;
	} else if (ty->categ == ARRAY) {

        /* 处理数组,的初始化 */
		int start = *offset;
		p = init->lbrace ? (AstInitializer)init->initials : init;

		if (((init->lbrace && !p->lbrace && p->next == NULL) || !init->lbrace) &&
		    p->expr->op == OP_STR && ty->categ / 2 == p->expr->ty->categ / 2) {

			size = p->expr->ty->size;
			if (ty->size == 0 || ty->size == size) {

				ty->size = size;
			} else if (ty->size == size - 1) {

				p->expr->ty->size = size - 1;
			} else if (ty->size < size) {

				Error(&init->coord, "string is too long");
				*error = 1;
			}

			CALLOC (initd);
			initd->offset   = *offset;
			initd->expr     = p->expr;
			initd->next     = NULL;
			(*tail)->next   = initd;
			*tail = initd;

			return (AstInitializer)init->next;
		}

        /* 多维数组 */
		while (p != NULL) {

			p = CheckInitializerInternal (tail, p, ty->bty, offset, error);
			size    += ty->bty->size;
			*offset = start + size;
			if (ty->size == size)
				break;
		}
		
		if (ty->size == 0) {

			ty->size = size;
		} else if (ty->size < size) {

            /* 初始化的数据太多 */
			Error (&init->coord, "too many initializer");
			*error = 1;
		}
	
		if (init->lbrace) {

			return (AstInitializer)init->next;
		}
		return p;
	} else if (ty->categ == STRUCT) {

        /* 处理结构体 */
		int     start = *offset;
        /* 所有字段的列表 */
		Field   fld = ((RecordType)ty)->flds;

        /* 对应的初始化数据 */
		p = init->lbrace ? (AstInitializer)init->initials : init;
		for (; fld && p; fld = fld->next) {

            /* 结构体内部的偏移 */
			*offset = start + fld->offset;
			p = CheckInitializerInternal (tail, p, fld->ty, offset, error);
			if (fld->bits != 0) {

                /* 计算偏移 */
				(*tail)->expr = PlaceBitField (fld, (*tail)->expr);
			}
		}

		if (init->lbrace) {

			if (p != NULL) {

				Error(&init->coord, "too many initializer");
				*error = 1;
			}
			*offset = ty->size;
			return (AstInitializer)init->next;
		}

		return (AstInitializer)p;
	}

	return init;
}


/* 检查初始化部分 */
static void CheckInitializer (AstInitializer init, Type ty)
{
	int     offset = 0, error = 0;
	struct initData header = {};
	InitData tail = &header;
	InitData prev, curr;

	if (IsScalarType (ty) && init->lbrace) {

        /* 判断是否为基本类型,并且要有大括号 */
		init = (AstInitializer)init->initials;
	} else if (ty->categ == ARRAY && ! (ty->bty->categ == CHAR || ty->bty->categ == UCHAR)) {

        /* 数组或者字符数组,并且要有大括号 */
		if (! init->lbrace) {

			Error(&init->coord, "Can't initialize array without brace");
			return;
		}
	} else if ((ty->categ == STRUCT || ty->categ == UNION) && ! init->lbrace) {

        /* struct 或者union 类型的初始化 */
		init->expr = Adjust (CheckExpression (init->expr), 1);
        /* 判断能否被赋值(即赋值是否正确) */
		if (! CanAssign (ty, init->expr)) {

			Error (&init->coord, "Wrong initializer");
		} else {

			CALLOC (init->idata);
			init->idata->expr   = init->expr;
		}
		return;
	}

    /* 检查初始化数据是否和类型匹配 */
	CheckInitializerInternal (&tail, init, ty, &offset, &error);
    /* 初始化出错直接结束 */
	if (error) return;

	init->idata = header.next;
	prev = NULL;

    /* 同一个类型的位段用一个表达式赖表示 */
	for (curr = init->idata; curr; curr = curr->next) {

		if (prev && prev->offset == curr->offset) {

            /* 构造按位或表达式 */
			prev->expr = BORBitField (prev->expr, curr->expr);
			prev->next = curr->next;
		} else {

			prev = curr;
		}
	}
}

/* 检查局部描述符(局部变量) */
void CheckLocalDeclaration (AstDeclaration decl, Vector v)
{
	AstInitDeclarator initDec;
	Type    ty;
	int     sclass;
	Symbol  sym;

    /* 检查第一部分的语义 */
	CheckDeclarationSpecifiers (decl->specs);
	if (decl->specs->sclass == TK_TYPEDEF) {

        /* 是typedef 的别名,检查类型 */
		CheckTypedef (decl);
		return;
	}

	ty      = decl->specs->ty;
	sclass  = decl->specs->sclass;
    /* 变量默认是auto 类型 */
	if (sclass == 0) 
		sclass = TK_AUTO;

	initDec = (AstInitDeclarator)decl->initDecs;
	for ( ; initDec; initDec = (AstInitDeclarator)initDec->next) {

        /* 检查第二部分的语义 */
		CheckDeclarator (initDec->dec);
		if (initDec->dec->id == NULL)
            continue;

		ty = DeriveType (initDec->dec->tyDrvList, decl->specs->ty);
		if (ty == NULL) {

			Error (&initDec->coord, "Illegal type");
			ty = T(INT);
		}

		if (initDec->dec->kind == NK_NameDeclarator && IsFunctionType (ty)) {

            /* 函数类型 */
			ty = PointerTo (ty);
		}

		if (IsFunctionType (ty)) {

			if (sclass == TK_STATIC) {

				Error(&decl->coord, "can't specify static for block-scope function");
			}

			if (initDec->init != NULL) {

				Error(&initDec->coord, "please don't initialize function");
			}

			if ((sym = LookupID (initDec->dec->id)) == NULL) {

                /* 经以上判断是函数,则添加到符号表 */
				sym = AddFunction (initDec->dec->id, ty, TK_EXTERN);
			} else if (!IsCompatibleType (sym->ty, ty)) {

				Error (&decl->coord, "Incompatible with previous declaration");
			} else {

				sym->ty = CompositeType (sym->ty, ty);
			}
		}

        /* 外部引入不用初始化 */
		if (sclass == TK_EXTERN && initDec->init != NULL) {

			Error(&initDec->coord, "can't initialize extern variable");
			initDec->init = NULL;
		}

        /* 该符号没有定义,或者不属于本层 */
		if ((sym = LookupID (initDec->dec->id)) == NULL || sym->level != Level) {

			VariableSymbol  vsym;

            /* 添加标量到符号表 */
			vsym = (VariableSymbol)AddVariable (initDec->dec->id, ty, sclass);
            /* 变量的初始化部分 */
			if (initDec->init) {

                /* 检查变量的初始化部分 */
				CheckInitializer (initDec->init, ty);
				if (sclass == TK_STATIC) {

                    /* 检查静态变量赋值时的赋值内容 */
					CheckInitConstant (initDec->init);
				} else {

                    /* 将变量添加到集合中 */
					INSERT_ITEM(v, vsym);
				}
                /* 变量的初始化数据 */
				vsym->idata = initDec->init->idata;
			}
		} else if (! (sym->sclass == TK_EXTERN && sclass == TK_EXTERN && IsCompatibleType (sym->ty, ty))) {

			Error(&decl->coord, "Variable redefinition");
		}
	}
}

/* 检查参数名, 及参是否有初始化 */
static void CheckIDDeclaration (AstFunctionDeclarator funcDec, AstDeclaration decl)
{
	Type    ty, bty;
	int     sclass;
	AstInitDeclarator initDec;
	Parameter param;
    /* 参数列表 */
	Vector params = funcDec->sig->params;

    /* 检查变量的第一部分 */
	CheckDeclarationSpecifiers (decl->specs);
	sclass  = decl->specs->sclass;
	bty     = decl->specs->ty;
    
	if (!(sclass == 0 || sclass == TK_REGISTER)) {

		Error(&decl->coord, "Invalid storage class");
		sclass = 0;
	}

    /* 检查初始化部分 */
	initDec = (AstInitDeclarator)decl->initDecs;
	for ( ; initDec; initDec = (AstInitDeclarator)initDec->next) {

		if (initDec->init) {

			Error(&initDec->coord, "Parameter can't be initialized");
		}

        /* 检查参数的第二部分 */
		CheckDeclarator (initDec->dec);
		if (initDec->dec->id == NULL)
            continue;

		FOR_EACH_ITEM (Parameter, param, params)
			if (param->id == initDec->dec->id)
				goto ok;
		ENDFOR
		Error (&initDec->coord, "%s is not from the identifier list", initDec->dec->id);
        continue;

ok:
		ty = DeriveType (initDec->dec->tyDrvList, bty);
		if (ty == NULL) {

			Error (&initDec->coord, "Illegal type");
			ty = T(INT);
		}

		if (param->ty == NULL) {

			param->ty   = ty;
			param->reg  = sclass == TK_REGISTER;
		} else {
			Error(&initDec->coord, "Redefine parameter %s", param->id);
		}
	}
}


/* 检查枚举的每个元素一个元素(last 表示当前最后一个的值) */
static int CheckEnumerator (AstEnumerator enumer, int last, Type ty)
{
    /* 该元素之后没有表达式 */
	if (!enumer->expr) {

        /* 构造并添加一个枚举元素到符号表中 */
		AddEnumConstant (enumer->id, ty, last + 1);
		return last + 1;
	} else {

        /* 检查表达式 */
		enumer->expr = CheckConstantExpression (enumer->expr);
		if (!enumer->expr) {
            /* 表达式错误 */
			Error (&enumer->coord, "The enumerator value must be integer constant.");
            /* 按照没有表达式去处理 */
			AddEnumConstant (enumer->id, ty, last + 1);
			return last + 1;
		}
        /* 枚举元素的值为表达式的值 */
		AddEnumConstant (enumer->id, ty, enumer->expr->val.i[0]);
		return enumer->expr->val.i[0];
	}
}


/* 检查枚举类型的语义 */
static Type CheckEnumSpecifier (AstEnumSpecifier enumSpec)
{
	AstEnumerator enumer;
	Symbol  tag;
	Type    ty;
	int     last;

    /* 没有名字,没有列表 */
	if (!enumSpec->id && !enumSpec->enumers)
		return T(INT);

    /* 枚举有名字,没有列表 */
	if (enumSpec->id && !enumSpec->enumers) {

        /* 在类型集合中查找 */
		tag = LookupTag (enumSpec->id);
        /* 没有找到,或者在本层有且类型不是枚举 */
		if (!tag || (tag->level == Level && tag->ty->categ != ENUM)) {

			Error(&enumSpec->coord, "Undeclared enum type.");
			return T(INT);
		}
		return tag->ty;
	}
    
    /* 没有名字,有列表 */
    if (!enumSpec->id && enumSpec->enumers) {

		ty = T(INT);
		goto chk_enumer;
	} else {

        /* 有名字,有列表 */
		tag = LookupTag (enumSpec->id);
        /* 如果没有找到,或者此类型是内嵌的就添加 */
		if (!tag || tag->level < Level) {

            /* Enum() 是构造枚举类型 */
			tag = AddTag (enumSpec->id, Enum (enumSpec->id));
		} else if (tag->ty->categ != ENUM) {
            /* 找到了相同类型,但不是枚举类型 */
			Error(&enumSpec->coord, "Tag redefinition.");
			return T(INT);
		}

		ty = tag->ty;
		goto chk_enumer;
	}

chk_enumer:

	enumer = (AstEnumerator)enumSpec->enumers;
	last = -1;
	for ( ; enumer; enumer = (AstEnumerator)enumer->next){

        /* 检查枚举的每个元素 */
		last = CheckEnumerator (enumer, last, ty);
	}

	return ty;
}



/* 检查指针的语义 */
static void CheckPointerDeclarator (AstPointerDeclarator ptrDec)
{
	int         qual = 0;
    /* 指针的限定符 */
	AstToken    tok = (AstToken)ptrDec->tyQuals;

    /* 检查第二部分的语义 */
	CheckDeclarator (ptrDec->dec);

	for ( ; tok; tok = (AstToken)tok->next) {

		qual |= tok->token == TK_CONST ? CONST : VOLATILE;
	}

	ALLOC(ptrDec->tyDrvList);
	ptrDec->tyDrvList->ctor = POINTER_TO;
	ptrDec->tyDrvList->qual = qual;
	ptrDec->tyDrvList->next = ptrDec->dec->tyDrvList;
	ptrDec->id = ptrDec->dec->id;
}


/* 添加参数到集合中 */
static void AddParameter (Vector params, char *id, Type ty, int reg, Coord coord)
{
	Parameter param;

	FOR_EACH_ITEM (Parameter, param, params)
        /* 检查参数名是否已经定义过 */
		if (param->id && param->id == id) {

			Error(coord, "Redefine parameter %s", id);
			return;
		}
	ENDFOR

	ALLOC(param);

	param->id = id;
	param->ty = ty;
	param->reg = reg;
	INSERT_ITEM(params, param);
}


/* 构造指针, 数组, 函数等类型 */
static Type DeriveType (TypeDerivList tyDrvList, Type ty)
{
	while ( tyDrvList ) {

		if (tyDrvList->ctor == POINTER_TO) {

            /* 指针类型 */
			ty = Qualify (tyDrvList->qual, PointerTo (ty));
		} else if (tyDrvList->ctor == ARRAY_OF) {

			if (ty->categ == FUNCTION || !ty->size || (IsRecordType (ty) && ((RecordType)ty)->hasFlexArray))
				return NULL;
            /* 构造一个数组 */
			ty = ArrayOf (tyDrvList->len, ty);
		} else {

			if (ty->categ == ARRAY || ty->categ == FUNCTION)
				return NULL;
            /* 构造一个函数返回值 */
			ty = FunctionReturn (ty, tyDrvList->sig);
		}
		tyDrvList = tyDrvList->next;
	}
	return ty;
}


/* 检查一个参数类型 */
static void CheckParameterDeclaration (AstFunctionDeclarator funcDec, AstParameterDeclaration paramDecl)
{
	char *id    = NULL;
	Type ty     = NULL;

    /* 检查参数的第一部分语义 */
	CheckDeclarationSpecifiers (paramDecl->specs);

	if (paramDecl->specs->sclass && paramDecl->specs->sclass != TK_REGISTER) {

        /* 参数如果有存储类型说明符, 则必须是 register */
		Error(&paramDecl->coord, "Invalid storage class");
	}
	ty = paramDecl->specs->ty;

    /* 检查第二部分的语义 */
	CheckDeclarator (paramDecl->dec);
    /* 无参 */
	if ( !paramDecl->dec->id && !paramDecl->dec->tyDrvList &&
	    ty->categ == VOID && !LEN (funcDec->sig->params) ) {

		if (paramDecl->next || ty->qual || paramDecl->specs->sclass) {

			Error(&paramDecl->coord, "'void' must be the only parameter");
			paramDecl->next = NULL;
		}
		return;
	}

    /* 构造指针, 数组, 函数返回值类型 */
	ty = DeriveType (paramDecl->dec->tyDrvList, ty);
	if ( ty ) {

        /* 若是数组, 或者函数同样当做指针对待 */
		ty = AdjustParameter (ty);
    }

	if ( !ty || !ty->size ) {

		Error(&paramDecl->coord, "Illegal parameter type");
		return;
	}
		
	id = paramDecl->dec->id;
	if (!id && funcDec->partOfDef) {

        /* 参数没有名字 */
		Error(&paramDecl->coord, "Expect parameter name");
		return;
	}
	
    /* 添加参数到集合中 */
	AddParameter (funcDec->sig->params, id, ty, paramDecl->specs->sclass == TK_REGISTER, &paramDecl->coord);
}

/* 检查参数类型列表, 并添加到函数的描述符中 */
static void CheckParameterTypeList (AstFunctionDeclarator funcDec)
{
	AstParameterTypeList    paramTyList = funcDec->paramTyList;
	AstParameterDeclaration paramDecl;
	
    /* 参数类型列表 */
	paramDecl = (AstParameterDeclaration)paramTyList->paramDecls;
	for ( ; paramDecl; paramDecl = (AstParameterDeclaration)paramDecl->next) {

        /* 检查参数列表中的参数,并添加到函数的描述符中 */
		CheckParameterDeclaration (funcDec, paramDecl);
	}
    /* 可变参 */
	funcDec->sig->hasEllipse = paramTyList->ellipse;
}


/* 检查函数的语义 */
static void CheckFunctionDeclarator (AstFunctionDeclarator dec)
{
	AstFunctionDeclarator funcDec = (AstFunctionDeclarator)dec;

    /* 检查第二部分的语义 */
	CheckDeclarator (funcDec->dec);

	CALLOC (funcDec->sig);

    /* 有无参数 */
	funcDec->sig->hasProto = funcDec->paramTyList != NULL;
    /* 可变参数 */
	funcDec->sig->hasEllipse = 0;
    /* 函数参数集合 */
	funcDec->sig->params = CreateVector (4);

	if ( funcDec->sig->hasProto ) {

        /* 检查参数列表 */
		CheckParameterTypeList (funcDec);
	} else if ( funcDec->partOfDef ) {

		char *id;
		FOR_EACH_ITEM (char*, id, funcDec->ids)
			AddParameter (funcDec->sig->params, id, NULL, 0, &funcDec->coord);
		ENDFOR 
	} else if ( LEN(funcDec->ids) ) {

		Error(&funcDec->coord, "Identifier list should be in definition.");
	}

	ALLOC(funcDec->tyDrvList);
	funcDec->tyDrvList->ctor = FUNCTION_RETURN;
	funcDec->tyDrvList->sig  = funcDec->sig;
	funcDec->tyDrvList->next = funcDec->dec->tyDrvList;
	funcDec->id = funcDec->dec->id;
}


/* 检查数组语义, 填充数组的长度, id等 */
static void CheckArrayDeclarator (AstArrayDeclarator arrDec)
{
    /* 检查第二部分的语义 */
	CheckDeclarator (arrDec->dec);

	if ( arrDec->expr ) {

        /* 数组的表达式必须是const 或者整形 */
		if ( !(arrDec->expr = CheckConstantExpression (arrDec->expr)) ) 
			Error(&arrDec->coord, "The size of the array must be integer constant.");
	}
	
    /* 填充数组的属性 */
	ALLOC (arrDec->tyDrvList);
    /* 表示数组类型 */
	arrDec->tyDrvList->ctor = ARRAY_OF;
    /* 数组的长度 */
	arrDec->tyDrvList->len  = arrDec->expr ? arrDec->expr->val.i[0] : 0;
	arrDec->tyDrvList->next = arrDec->dec->tyDrvList;
	arrDec->id = arrDec->dec->id;
}

/* 检查第二部分的语义 */
static void CheckDeclarator (AstDeclarator dec)
{
	switch (dec->kind) {
    
        /* 常数的语义检查 */
    	case NK_NameDeclarator: break;

    	case NK_ArrayDeclarator: {
            /* 检查数组语义 */
		    CheckArrayDeclarator ((AstArrayDeclarator)dec);
        }break;

	    case NK_FunctionDeclarator: {
            /* 检查函数的语义 */
		    CheckFunctionDeclarator ((AstFunctionDeclarator)dec);
        }break;

    	case NK_PointerDeclarator: {
            /* 检查指针的语义 */
		    CheckPointerDeclarator ((AstPointerDeclarator)dec);
        }break;
    	default: assert(0);
	}
}

/* 检查一个字段的第二部分 */
static void CheckStructDeclarator (Type rty, AstStructDeclarator stDec, Type fty)
{
	char    *id = NULL;
	int     bits = 0;

    /* 字段的第二部分 */
	if ( stDec->dec ) {

        /* 检查第二部分的语义 */
		CheckDeclarator (stDec->dec);
        /* 字段变量名 */
		id = stDec->dec->id;
        /* 构造指针, 数组, 函数返回值类型 */
		fty = DeriveType (stDec->dec->tyDrvList, fty);
	}

	if (!fty || fty->categ == FUNCTION || (!fty->size && fty->categ != ARRAY)) {

		Error(&stDec->coord, "illegal type");
		return;
	}

    /* 是否包含灵活数组(即大小为0 的数组),  灵活数组必须是最后一个字段 */
	if ( ((RecordType)rty)->hasFlexArray ) {

		Error(&stDec->coord, "the flexible array must be the last member");
		return;
	}

	if (IsRecordType (fty) && ((RecordType)fty)->hasFlexArray) {

		Error(&stDec->coord, "A structure has flexible array shall not be a member");
		return;
	}

    /* 判断字段名是否重定义 */
	if (id && LookupField (rty, id)) {

		Error(&stDec->coord, "member redefinition");
		return;
	}

	if ( stDec->expr ) {

		stDec->expr = CheckConstantExpression (stDec->expr);
		if ( !stDec->expr ) {

			Error(&stDec->coord, "The width of bit field should be integer constant.");
		} else if (id && stDec->expr->val.i[0] == 0) {

			Error(&stDec->coord, "bit field's width should not be 0");
		}

		if (fty->categ != INT && fty->categ != UINT) {

			Error(&stDec->coord, "Bit field must be integer type.");
			fty = T(INT);
		}

		bits = stDec->expr ? stDec->expr->val.i[0] : 0;
		if (bits > T(INT)->size * 8) {

			Error(&stDec->coord, "Bit field's width exceeds");
    		bits = 0;
		}
	}

    /* 将字段添加到列表中 */
	AddField (rty, id, fty, bits);
}


/* 检查构造类型一个字段的语义,  并将其添加到字段列表中 */
static void CheckStructDeclaration (AstStructDeclaration stDecl, Type rty)
{
    AstStructDeclarator stDec;

    /* 检查字段的第一部分语法 */
    CheckDeclarationSpecifiers (stDecl->specs);
    /* 第二三部分 */
    stDec = (AstStructDeclarator)stDecl->stDecs;

    if (stDec) {

        /* 将该字段添加到构造类型的字段列表中, (没有id, 和位段) */
        AddField (rty, NULL, stDecl->specs->ty, 0);
    }

    /* 同一类型, 用逗号分隔的几个变量 */
    for ( ; stDec; stDec = (AstStructDeclarator)stDec->next) {

        /* 检查第二部分的语义 */
        CheckStructDeclarator (rty, stDec, stDecl->specs->ty);
    }
}


/* 检查构造类型的语义 */
static Type CheckStructOrUnionSpecifier (AstStructSpecifier stSpec)
{
	int categ = (stSpec->kind == NK_StructSpecifier) ? STRUCT : UNION;
	Symbol  tag;
	Type    ty;
	AstStructDeclaration stDecl;

    /* 至返回类型是已经定义过的构造类型 */
	if (stSpec->id && !stSpec->stDecls) {
        
        /* 查询构造类型是否定义过 */
		tag = LookupTag (stSpec->id);
		if ( !tag ) {

            /* 记录此类型 */
			ty = StartRecord (stSpec->id, categ);
            /* 添加此类型到符号表中 */
			tag = AddTag (stSpec->id, ty);
		} else if (tag->ty->categ != categ) {
            
            /* 如果类型定义过, 判读类别码是否吻合 */
			Error(&stSpec->coord, "Inconsistent tag declaration.");
		}
		return tag->ty;
	} else if (!stSpec->id && stSpec->stDecls) {
    
        /* 构造类型没有ID, 有字段 */
		ty = StartRecord (NULL, categ);
		goto chk_decls;
	} else if (stSpec->id && stSpec->stDecls) {

        /* 构造类型有ID, 和字段列表(即, 新定义的构造类型) */
		tag = LookupTag (stSpec->id);
		if (!tag || tag->level < Level) {

            /* 前边没有定义过, 或者之前定义过, 在本层新定义 */
			ty = StartRecord (stSpec->id, categ);
			AddTag (stSpec->id, ty);
		} else if (tag->ty->categ == categ && !tag->ty->size) {
    
            /* struct Type ; (空定义) */
			ty = tag->ty;
		} else {

			Error(&stSpec->coord, "Tag redefinition.");
			return tag->ty;
		}
		goto chk_decls;
	} else {

		ty = StartRecord (NULL, categ);
		EndRecord (ty);
		return ty;
	}

/* 检查构造类型字段的语义 */
chk_decls:
	stDecl = (AstStructDeclaration)stSpec->stDecls;
	for ( ; stDecl; stDecl = (AstStructDeclaration)stDecl->next)
		CheckStructDeclaration (stDecl, ty);
	EndRecord(ty);

	return ty;
}


/* 检查第一部分的语义 */
static void CheckDeclarationSpecifiers (AstSpecifiers specs)
{
	AstToken    tok;
	AstNode     p;
	Type        ty;
	int size = 0, sign = 0;
	int signCnt = 0, sizeCnt = 0, tyCnt = 0;
	int qual = 0;

    /* 存储类型符 */
	tok = (AstToken)specs->stgClasses;
	if ( tok ) {
    
        /* 函数的存储类型修饰符不能有多个 */
		if ( tok->next )
			Error(&specs->coord, "At most one storage class");
		specs->sclass = tok->token;
	}

    /* 限定符 */
	tok = (AstToken)specs->tyQuals;
	for ( ; tok; tok = (AstToken)tok->next) {
        
		qual |= (tok->token == TK_CONST ? CONST : VOLATILE);
	}

    /* 函数返回值类型 */
	for (p = specs->tySpecs; p; p = p->next) {

		if (p->kind == NK_StructSpecifier || p->kind == NK_UnionSpecifier) {

            /* 检查构造类型的语义 */
			ty = CheckStructOrUnionSpecifier ((AstStructSpecifier)p);
			tyCnt++;
		} else if (p->kind == NK_EnumSpecifier) {

            /* 检查枚举类型的语义 */
			ty = CheckEnumSpecifier ((AstEnumSpecifier)p);
			tyCnt++;
		} else if (p->kind == NK_TypedefName) {

            /* 检查typedef 重命名的类型 */
			Symbol sym = LookupID (((AstTypedefName)p)->id);
            /* 类型错误则结束 */
			assert (sym->kind == SK_TypedefName);
			ty = sym->ty;
			tyCnt++;
		} else  {

            switch ((tok = (AstToken)p)->token) {

                /* signed unsigned 类型 */
    			case TK_SIGNED: case TK_UNSIGNED: {

	    			sign = tok->token;
		    		signCnt++;
                }break;

                /* short long 类型 */
	    		case TK_SHORT: case TK_LONG: {

				    if (size == TK_LONG && sizeCnt == 1)
    					size = TK_LONG + TK_LONG;
				    else {

					    size = tok->token;
    					sizeCnt++;
				    }
                }break;

	    		case TK_CHAR: {

    				ty = T(CHAR);
	    			tyCnt++;
                }break;

	    		case TK_INT: {

    				ty = T(INT);
	    			tyCnt++;
                }break;

    			case TK_INT64: {

    				ty = T(INT);
	    			size = TK_LONG + TK_LONG;
		    		sizeCnt++;
                }break;

    			case TK_FLOAT: {

	    			ty = T(FLOAT);
		    		tyCnt++;
                }break;

    			case TK_DOUBLE: {

	    			ty = T(DOUBLE);
		    		tyCnt++;
                }break;

	    		case TK_VOID: {

		    		ty = T(VOID);
			    	tyCnt++;
                }break;
			}
		}
	}

	if (tyCnt == 0) 
        ty = T(INT);

	if (sizeCnt > 1 || signCnt > 1 || tyCnt > 1)
		goto err;

    /* 处理复合类型 long double */
	if (ty == T(DOUBLE) && size == TK_LONG) {

		ty = T(LONGDOUBLE);
		size = 0;
	} else if (ty == T(CHAR)) {

        /* unsigned char */
		sign = (sign == TK_UNSIGNED);
		ty = T(CHAR + sign);
		sign = 0;
	}

	if (ty == T(INT)) {

        /* 判断是否有unsigned 修饰 */
		sign = (sign == TK_UNSIGNED);

		switch (size) {

    		case TK_SHORT:
	    		ty = T(SHORT + sign);
		    	break;

    		case TK_LONG:
	    		ty = T(LONG + sign);
		    	break;

    		case TK_LONG + TK_LONG:
	    		ty = T(LONGLONG + sign);
		    	break;

    		default:
	    		assert(size == 0);
		    	ty = T(INT + sign);
			    break;
		}
		
	} else if (size != 0 || sign != 0) {

		goto err;
	}

	specs->ty = Qualify (qual, ty);
	return;

err:
	Error(&specs->coord, "Illegal type specifier.");
	specs->ty = T(INT);
	return;
}

/* 函数语义检查 */
/* 检查函数首部信息, 将参数添加到符号表中, 检查函数体, 检查函数标签 */
void CheckFunction (AstFunction func)
{
	Symbol  sym;
	Type    ty;
	int     sclass;
	Label   label;
	AstNode p;
    /* 存储参数集合 */
	Vector  params;
	Parameter param;
	
    /* 置标志位 */
	func->fdec->partOfDef = 1;

    /* 检查函数第一部分的语义 */
	CheckDeclarationSpecifiers (func->specs);
	if ((sclass = func->specs->sclass) == 0) {

		sclass = TK_EXTERN;
	}

    /* 检查第二部分的语义 */
	CheckDeclarator (func->dec);


	for (p = func->decls; p; p = p->next) {

        /* 检查参数的ID, 及有没有被初始化 */
		CheckIDDeclaration (func->fdec, (AstDeclaration)p);
	}

	params = func->fdec->sig->params;
	FOR_EACH_ITEM (Parameter, param, params)
		if (param->ty == NULL)
			param->ty = T(INT);
	ENDFOR
	
    /* 检查函数的类型 */
	ty = DeriveType (func->dec->tyDrvList, func->specs->ty);
	if (ty == NULL) {

		Error (&func->coord, "Illegal function type");
		ty = DefaultFunctionType;
	}

    /* 查找函数名, 是否被定义过 */
	sym = LookupID (func->dec->id);
	if (sym == NULL) {

        /* 添加一个函数到符号表中 */
		func->fsym = (FunctionSymbol)AddFunction (func->dec->id, ty, sclass);
	} else if (sym->ty->categ != FUNCTION) {

        /* 变量和函数重名 */
		Error(&func->coord, "Redeclaration as a function");
		func->fsym = (FunctionSymbol)AddFunction (func->dec->id, ty, sclass);
	} else {

        /* 有函数同名 */
		func->fsym = (FunctionSymbol)sym;
		if (sym->sclass == TK_EXTERN && sclass == TK_STATIC) {

			Error(&func->coord, "Conflict function linkage");
		}

        /* 检查两个类型是否兼容 */
		if (!IsCompatibleType (ty, sym->ty)) {

			Error (&func->coord, "Incompatible with previous declaration");
			sym->ty = ty;
		} else {

			sym->ty = CompositeType (ty, sym->ty);
		}

		if (func->fsym->defined) {

			Error (&func->coord, "Function redefinition");
		}
	}

    /* 标志函数已经定义 */
	func->fsym->defined = 1;
	func->loops         = CreateVector (4);
	func->breakable     = CreateVector (4);
	func->swtches       = CreateVector (4);

    /* 当前函数 */
	CURRENTF    = func;
    /* 函数符号表 */
	FSYM        = func->fsym;

	EnterScope ();
	{
		Vector v= ((FunctionType)ty)->sig->params;
		
		FOR_EACH_ITEM (Parameter, param, v)
            /* 将参数逐个添加当FSYM (当前函数符号表) */
			AddVariable (param->id, param->ty, param->reg ? TK_REGISTER : TK_AUTO);
		ENDFOR

		FSYM->locals    = NULL;
		FSYM->lastv     = &FSYM->locals;
	}

    /* 检查函数体语义 */
	CheckCompoundStatement (func->stmt);
    /* 退出一个模块 */
	ExitScope ();

    /* 检查函数内部的标签 */
	for (label = func->labels; label; label = label->next) {

		if (!label->defined) {

			Error(&label->coord, "Undefined label");
		}
	}

    /* 判断返回值 */
	if (ty->bty != T(VOID) && !func->hasReturn) {

		Warning(&func->coord, "missing return value");
	}
}

/* 语法分析单元的语义分析 */
/* transUnit 语法分析单元 */
void CheckTranslationUnit (AstTranslationUnit transUnit)
{
    AstNode p;
    Symbol  f;

    /* 遍历每条语句, 检查语义 */
    for (p = transUnit->extDecls; p; p = p->next) {
    
        if (NK_Function == p->kind) {

            /* 检查函数的语义 */
            CheckFunction ((AstFunction)p);
        } else {

            assert (NK_Declaration == p->kind);
            /* 全局定义 */
            CheckGlobalDeclaration ((AstDeclaration)p);
        }
    }

    for (f = Functions; f; f = f->next) {
        
        /* static 修饰的函数必须在此模块中定义 */
        if (f->sclass == TK_STATIC && !f->defined) {

            Error (NULL, "static function %s not defined", f->name);
        }
    }
}
