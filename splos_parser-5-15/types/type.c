/*************************************************************************
	> File Name: type.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月14日 星期五 13时01分50秒

	> Description: 类型子系统
 ************************************************************************/

#include "pcl.h"
#include "output.h"
#include "type.h"
#include "config.h"

/* 保存基本类型的属性 */
struct type Types[VOID - CHAR + 1] = {};
/* 保存默认函数类型的属性 */
Type DefaultFunctionType;
/* 保存宽字符的属性 */
Type WCharType = NULL;

/* 提升类型 */
Type Promote (Type ty) 
{
    return ty->categ < INT ? T(INT) : (ty->categ == FLOAT ? T (DOUBLE) : ty);
}

/* 添加限定符 */
Type Qualify (int qual, Type ty) 
{
    Type qty;

    /* 没有修饰符,或者同一修饰符再次修饰 */
    if (!qual || qual == ty->qual) 
        return ty;

    ALLOC (qty);
    *qty = *ty;
    qty->qual |= qual;

    if ( ty->qual ) {

        /* 之前有类型修饰符,这直接赋值基类 */
        qty->bty = ty->bty;
    } else {

        /* 之前没有类型修饰符,则将ty设置为qty的基类 */
        qty->bty = ty;
    }
    return qty;
}

/* 去掉限定符 */
Type Unqual (Type ty) 
{
    if (ty->qual) 
        ty = ty->bty;
    return ty;
}

/* 将ty 设为指针类型,如int 将返回int* */
Type PointerTo (Type ty) 
{
    Type pty;
    /* 构建指针类型 */
    CALLOC (pty);
    pty->qual  = 0;
    pty->categ = POINTER;
    pty->align = T(POINTER)->align;
    pty->size  = T(POINTER)->size;

    /* 将ty 设为指针的基本类型 */
    pty->bty = ty;
    /* 返回指针类型 */
    return pty;
}

/* 返回类型级别最高的,或占字节数最多的类型 */
Type CommonRealType (Type ty1, Type ty2)
{
    /* 将类型都提升为浮点类型 */
    if (LONGDOUBLE == ty1->categ || LONGDOUBLE == ty2->categ)
        return T(LONGDOUBLE);
    if (DOUBLE == ty1->categ || DOUBLE == ty2->categ) 
        return T(DOUBLE);
    if (FLOAT == ty1->categ || FLOAT == ty2->categ) 
        return T(FLOAT);

    /* 将类型小于INT 的都提升为INT */
    ty1 = ty1->categ < INT ? T(INT) : ty1;
    ty2 = ty2->categ < INT ? T(INT) : ty2;

    if (ty1->categ == ty2->categ) 
        return ty1;

    /* 都被unsigned修饰,或者都不被unsigned修饰 */
    if (0 == (IsUnsigned (ty1) ^ IsUnsigned (ty2))) 
        return ty1->categ > ty2->categ ? ty1 : ty2;
    
    /* 用ty1表示被unsigned修饰的类型 */
    if (IsUnsigned(ty2)) {

        Type ty;
        ty = ty1;
        ty1 = ty2;
        ty2 = ty;
    }

    /* 如果ty1的级别高,则直接返回 */
    if (ty1->categ >= ty2->categ) 
        return ty1;

    /* 其他的类型用所占字节的大小比较 */
    if (ty2->size > ty1->size) 
        return ty2;

    /* ty2 是枚举类型 */
    return T(ty2->categ + 1);
}

/* 若是数组或函数,则返回指针类型 */
Type AdjustParameter (Type ty) 
{
    ty = Unqual (ty);
    /* 若为数组,则返回指针类型 */
    if (ARRAY == ty->categ) 
        return PointerTo (ty->bty);

    /* 若为函数,函数也返回指针类型 */
    if (FUNCTION == ty->categ) 
        return PointerTo (ty);

    return ty;
}

/* 将一个类型用完整的字符串表达出来 */
const char* TypeToString (Type ty) 
{
    /* 基本类型的名称 */
    char *names[] = {

        "char", "unsigned char", "short", "unsigned short", 
        "int", "unsigned int", "long", "unsigned long", 
        "long long", "unsigned long long", "enum", "float", 
        "double", "long double"
    };

    int qual = 0;
    char *str = NULL;

    /* 有const 或volatile 修饰 */
    if ( ty->qual ) {
    
        qual = ty->qual;
        /* 求取基类 */
        ty = Unqual (ty);
        if (CONST == qual) 
            str = "const";
        else if (VOLATILE == qual) 
            str = "volatile";
        else 
            str = "const volatile";
        return FormatName ("%s %s", str, TypeToString (ty));
    }

    /* 整形和浮点(除枚举) */
    if (CHAR <= ty->categ && LONGDOUBLE >= ty->categ && ty->categ != ENUM) 
        return names[ty->categ];

    /* 其他类型 */
    switch (ty->categ) {
        
        case ENUM: return FormatName ("enum %s", ((EnumType)ty)->id);
        case POINTER: return FormatName ("%s *", TypeToString (ty->bty));
        case UNION: return FormatName ("union %s", ((RecordType)ty)->id);
        case STRUCT: return FormatName ("struct %s", ((RecordType)ty)->id);
        case ARRAY: return FormatName ("%s[%d]", TypeToString (ty->bty), ty->size);
        case VOID: return "void";
        case FUNCTION: {
            FunctionType fty = (FunctionType)ty;
            return FormatName ("%s ()", TypeToString (fty->bty));
        }
        /* 不属于任何类型,终止进程 */
        default: 
            assert (0);
            return NULL;
    }
}

/* 设置基本类型的属性,从config.h 中读取属性的配置 */
void SetupTypeSystem (void) 
{
    /* 设置基本类型所占字节大小 */
    T(CHAR)->size   = T(UCHAR)->size    = CHAR_SIZE;
    T(SHORT)->size  = T(USHORT)->size   = SHORT_SIZE;
    T(INT)->size    = T(UINT)->size     = INT_SIZE;
    T(LONG)->size   = T(ULONG)->size = LONG_SIZE;
    T(LONGLONG)->size = T(ULONGLONG)->size = LONG_LONG_SIZE;
    T(FLOAT)->size  = FLOAT_SIZE;
    T(DOUBLE)->size = DOUBLE_SIZE;
    T(LONGDOUBLE)->size = LONG_DOUBLE_SIZE;
    T(POINTER)->size    = INT_SIZE;

    int i;
    /* 设置基本类型的类别和字节对齐的基数 */
    for (i = CHAR; i <= VOID; i++) {

        T(i)->categ = i;
        T(i)->align = T(i)->size;
    }

    /* 函数类型的设置 */
    FunctionType fty; 
    ALLOC (fty); 

    fty->categ = FUNCTION;
    fty->align = fty->size = T(POINTER)->size;
    fty->bty   = T(INT);

    CALLOC (fty->sig);
    CALLOC (fty->sig->params);

    /* 默认函数类型(有函数类型强制转换成TYPE类型) */
    DefaultFunctionType = (Type)fty;
    /* 宽字符类型 */
    WCharType = T(WCHAR);
}

/* 构造一个数组(长度和类型) */
Type ArrayOf (int len, Type ty) 
{
    Type aty;
    CALLOC (aty);

    aty->categ  = ARRAY;
    aty->qual   = 0;
    aty->size   = len * ty->size;
    aty->align  = ty->align;
    aty->bty    = ty;
    return (Type)aty;
}

/* 构造一个构造类型(初始化) */
Type StartRecord (char *id, int categ)
{
    RecordType rty;
    CALLOC (rty);

    rty->categ  = categ;
    rty->id     = id;
    rty->tail   = &rty->flds;
    return (Type)rty;
}

/* 计算字段偏移 */
static void AddOffset (RecordType rty, int offset)
{
    /* 构造类型字段列表 */
    Field fld = rty->flds;

    for (; fld; fld = fld->next) {

        fld->offset += offset;
        if (!fld->id && IsRecordType (fld->ty)) {

            /* 该字段是构造类型,且没有名称 */
            AddOffset ((RecordType)fld->ty, fld->offset);
        }
    }
}

/* 处理 struct/union 的字段问题 */
void EndRecord (Type ty)
{
	RecordType  rty = (RecordType)ty;
    /* 字段列表 */
	Field       fld = rty->flds;
	int bits = 0;
    /* int 的位数 */
	int intBits = T(INT)->size * 8;

	if (rty->categ == STRUCT) {

        /* 处理struct 情况(字段) */
		for ( ; fld; fld = fld->next ) {

            /* 按照该字段字节对齐 */
			fld->offset = rty->size = ALIGN (rty->size, fld->ty->align);
			if (!fld->id && IsRecordType (fld->ty)) {

                /* struct 内部遇到struct 计算新的偏移 */
				AddOffset ((RecordType)fld->ty, fld->offset);
			}

			if (!fld->bits) {

                /* 最后整个按照最长的对齐 */
				if ( bits ) {

					fld->offset = rty->size = ALIGN(rty->size + T(INT)->size, fld->ty->align);
				}
				bits = 0;
				rty->size = rty->size + fld->ty->size;
			} else if (bits + fld->bits <= intBits) {

                /* 字节对齐 */
				fld->pos = LITTLE_ENDIAN ? bits : intBits - bits;
				bits = bits + fld->bits;
				if (bits == intBits) {

					rty->size += T(INT)->size;
					bits = 0;
				}
			} else {

				/// current bit-field can't be placed together with previous bit-fields,
				/// must start a new chunk of memory
				rty->size += T(INT)->size;
				fld->offset += T(INT)->size;
				fld->pos = LITTLE_ENDIAN ? 0 : intBits - fld->bits;
				bits = fld->bits;
			}

			if (fld->ty->align > rty->align) {

				rty->align = fld->ty->align;
			}
		}

		if ( bits ) {

			rty->size += T(INT)->size;
		}
		rty->size = ALIGN(rty->size, rty->align);
	} else {

        /* union 的字段 */
		for ( ; fld; fld = fld->next) {

            /* 选最大的左字节对齐基数 */
			if (fld->ty->align > rty->align)
				rty->align = fld->ty->align;

			if (fld->ty->size > rty->size) 
				rty->size = fld->ty->size;
		}
	}
}

/* 添加一个字段到struct/union 中,fty:字段的类型,bits:是否表示位段 */
Field AddField (Type ty, char *id, Type fty, int bits)
{
    RecordType rty = (RecordType)ty;
    Field fld;

    if ( fty->size == 0 ) {

        /* 灵活数组 */
        assert (ARRAY == fty->categ);
        rty->hasFlexArray = 1;
    }

    if (CONST & fty->qual) {

        /* const 修饰字段 */
        rty->hasConstFld = 1;
    }

    CALLOC (fld);
    fld->id = id;
    fld->ty = fty;
    fld->bits = bits;

    *rty->tail = fld;
    rty->tail = &(fld->next);

    return fld;
}

/* 构造一个函数类型,返回值类型为ty */
Type FunctionReturn (Type ty, Signature sig) 
{
    FunctionType fty;
    ALLOC (fty);

    fty->categ  = FUNCTION;
    fty->qual   = 0;
    fty->size   = T (POINTER)->size;
    fty->align  = T (POINTER)->align;
    fty->sig    = sig;
    fty->bty    = ty;

    return (Type)fty;
}

/* 在构造类型ty 中查找 id这个字段 */
Field LookupField (Type ty, char *id)
{
    RecordType rty = (RecordType)ty;
    /* 所有字段列表 */
    Field fld = rty->flds;

    for ( ; fld; fld = fld->next) {

        /* 该字段是复合类型 */
        if (!fld->id && IsRecordType (fld->ty)) {

            Field p = LookupField (fld->ty, id);
            if ( p ) return p;
        } else if (fld->id == id) 
            return fld;
    }
    return NULL;
}

/* 构造一个枚举类型 */
Type Enum (char *id) 
{
    EnumType ety;
    CALLOC (ety);

    ety->categ  = ENUM;
    ety->id     = id;

    ety->bty    = T (INT);
    ety->size   = ety->bty->size;
    ety->align  = ety->bty->align;
    ety->qual   = 0;

    return (Type)ety;
}

/* 检查两个函数的兼容性 */
static int IsCompatibleFunction (FunctionType fty1, FunctionType fty2)
{
    /* 函数的参数情况 */
	Signature sig1 = fty1->sig;
	Signature sig2 = fty2->sig;
	Parameter p1, p2;
	int parLen1, parLen2;
	int i;

    /* 检查两个函数的返回值类型是否相同 */
	if (!IsCompatibleType (fty1->bty, fty2->bty))
		return 0;

    /* 都没有参数 */
	if (!sig1->hasProto && !sig2->hasProto) 
		return 1;
	else if (sig1->hasProto && sig2->hasProto) {

        /* 参数的个数 */
		parLen1 = LEN (sig1->params);
		parLen2 = LEN (sig2->params);

        /* 判断参数个数 */
		if ((sig1->hasEllipse ^ sig2->hasEllipse) || (parLen1 != parLen2))
			return 0;

		for (i = 0; i < parLen1; ++i) {

			p1 = (Parameter)GET_ITEM (sig1->params, i);
			p2 = (Parameter)GET_ITEM (sig2->params, i);
            /* 判断每个参数类型是否相同 */
			if (!IsCompatibleType (p1->ty, p2->ty))
				return 0;
		}
		return 1;
	} else if (!sig1->hasProto && sig2->hasProto) {

        /* 一个有参数一个没有参数(不包括不定参) */
		sig1 = fty2->sig;
		sig2 = fty1->sig;
	}

	parLen1 = LEN(sig1->params);
	parLen2 = LEN(sig2->params);

	if (sig1->hasEllipse)
		return 0;

	if (parLen2 == 0) {

		FOR_EACH_ITEM (Parameter, p1, sig1->params)
			if (!IsCompatibleType (Promote (p1->ty), p1->ty))
				return 0;
		ENDFOR
		return 1;
	} else if (parLen1 != parLen2) {
		return 0;
	} else {
		for (i = 0; i < parLen1; ++i) {

			p1 = (Parameter)GET_ITEM(sig1->params, i);
			p2 = (Parameter)GET_ITEM(sig2->params, i);

			if (! IsCompatibleType(p1->ty, Promote(p2->ty)))
				return 0;
		}		
		return 1;
	}
}

/* 检查类型是否兼容 */
int IsCompatibleType (Type ty1, Type ty2)
{
    if ( ty1 == ty2 ) 
        return 1;

    /* 修饰符不同,则类型不兼容 */
    if ( ty1->qual != ty2->qual ) return 0;

    /* 下面判断两个类型基类 */
    ty1 = Unqual (ty1);
    ty2 = Unqual (ty2);

    /* 如果有枚举类型,则枚举类型的基类和另一个类型必须相同 */
    if (ty1->categ == ENUM && ty2 == ty1->bty || 
        ty2->categ == ENUM && ty1 == ty2->bty) 
        return 1;

    /* 基类不同,则类型不兼容 */
    if (ty1->categ != ty2->categ) 
        return 0;

    /* 两个类别相同 */
    switch (ty1->categ) {
        
        /* 如果是指针,则继续判断兼容 */
        case POINTER: return IsCompatibleType (ty1->bty, ty2->bty);
        /* 如果是数组,则继续判断兼容 */
        case ARRAY: 
            return IsCompatibleType (ty1->bty, ty2->bty) && 
                (ty1->size == ty2->size || !ty1->size || !ty2->size);
        /* 如果是函数,则继续判断兼容 */
        case FUNCTION: return IsCompatibleFunction ((FunctionType)ty1, (FunctionType)ty2);

        /* 其他类型,只有两个类型相同才兼容 */
        default: return ty1 == ty2;
    }
}

Type CompositeType (Type ty1, Type ty2)
{
    /* 类型不兼容,出错终端 */
    assert (IsCompatibleType (ty1, ty2));

    if (ENUM == ty1->categ) 
        return ty1;
    if (ENUM == ty2->categ) 
        return ty2;

    switch (ty1->categ) {
    
        case POINTER: 
            return Qualify (ty1->qual, PointerTo (CompositeType (ty1->bty, ty2->bty)));

        case ARRAY:
            return ty1->size ? ty1 : ty2;

        case FUNCTION: {
            
            FunctionType fty1 = (FunctionType)ty1;
            FunctionType fty2 = (FunctionType)ty2;

            fty1->bty = CompositeType (fty1->bty, fty2->bty);
            if (fty1->sig->hasProto && fty2->sig->hasProto) {
    
                Parameter p1, p2;
                int i, len = LEN (fty1->sig->params);

                for (i = 0; i < len; i++) {

                    p1 = (Parameter)GET_ITEM (fty1->sig->params, i);
                    p2 = (Parameter)GET_ITEM (fty2->sig->params, i);
                    p1->ty = CompositeType (p1->ty, p2->ty);
                }
                return ty1;
            }

            return fty1->sig->hasProto ? ty1 : ty2;
        }

        default: return ty1;
    }
}

/* 返回类型标号 */
int TypeCode (Type ty)
{
    /* 对应在type.h 中枚举出来的类型（各种类型,不包含函数类型）*/
    static int optypes[] = {I1, U1, I2, U2, I4, U4, I4, U4, I4, U4, I4, F4, F8, F8, U4, V, B, B, B};

    assert (ty->categ != FUNCTION);
    return optypes[ty->categ];
}
