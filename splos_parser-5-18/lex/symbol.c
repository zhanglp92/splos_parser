/*************************************************************************
	> File Name: symbol.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月19日 星期三 19时56分31秒

	> Description: 
 ************************************************************************/

#include "pcl.h"
#include "output.h"

/* 保存全局范围的,struct/union/enum 的定义 */
static struct table GlobalTags;
/* 保存全局范围的,标示符 */
static struct table GlobalIDs;
/* 存储所有的常量 */
static struct table Constants;
/* 指向本层的符号表 */
static Table Tags, Identifiers;

/* 当前函数 */
Symbol Functions;
/* 保存全局和静态变量 */
Symbol Globals;
/* 保存字符串 */
Symbol Strings;
/* 保存浮点常量 */
Symbol FloatConstants;
static Symbol *FunctionTail, *GlobalTail, *StringTail, *FloatTail;
/* 表示对应符号的数量(临时变量,标签,字符串常量) */
unsigned int TempNum, LabelNum, StringNum;
/* 当前内嵌的层数 */
int Level;


/* 保存define 符号 */
static struct table DefineTable;
Symbol Defines;

/* 将某个符号插入到某个符号表中 */
static Symbol AddSymbol (Table tbl, Symbol sym)
{
    /* 计算一个哈希值 */
    unsigned int h = (unsigned long long)sym->name & SYM_HASH_MASK;

    if (NULL == tbl->buckets) {
    
        /* 给该符号表申请一段存放空间 */
        int size = sizeof (Symbol) * (SYM_HASH_MASK + 1);
        tbl->buckets = HeapAllocate (CurrentHeap, size);
        memset (tbl->buckets, 0, size);
    }

    /* 头插 */
    sym->link = tbl->buckets[h];
    tbl->buckets[h] = sym;
    /* 此时表的层次(范围)就是符号的范围 */
    sym->level = tbl->level;
    INFO (5, "add Symbol %s \n", sym->name);

    return sym;
}

/* 查找系统调用号
 * 0 号为普通调用 */
int LookupSysCall (const char *name)
{
    struct sysCall {
        const char  *name;
        int     len: 16;
        int     num: 16;
    };

    static const struct sysCall sysC[] = {
    
        #define SYSCALL(TK, name, len, num) {name, len, num}, 
        #include "syscall.h"
        #undef  SYSCALL
        {NULL}
    };
    int i;

    for (i = 0; sysC[i].name; i++) 
        if (0 == strncmp (name, sysC[i].name, sysC[i].len)) 
            return sysC[i].num;
    return 0;
}

/* 在表中查找某个符号 */
static Symbol LookupSymbol (Table tbl, const char *name) 
{
    Symbol p;
    unsigned h = (unsigned long long)name & SYM_HASH_MASK;
    
    do {

        if (tbl->buckets) {

            for (p = tbl->buckets[h]; p; p = p->link) {

                /* 注意:是字符串的地址直接比较 */
                if (p->name == name)
                    return p;
            }
        }
        /* 如果有外部链接的符号表,继续查找 */
    } while ((tbl = tbl->outer));
    
    return NULL;
}

/* 没有真的删除, 只是将有效位标记 */
static Symbol DeleteSymbol (Table tbl, const char *name)
{
    Symbol p = LookupSymbol (tbl, name);
    if (p == NULL)
        return NULL;

    /* 无效 */
    p->unvalid = true;
    return p;
}

/* 进入一个模块 */
void EnterScope (void)
{
    Table t;

    /* 进入一个模块,则层次(范围)+1 */
    Level++;

    /* 创建模块内部的标示符表 */
    ALLOC (t);
    t->level    = Level;
    /* 上一层的标示符为链接到外部链接 */
    t->outer    = Identifiers;
    t->buckets  = NULL;
    /* Identifiers 指向本层的符号表 */
    Identifiers = t;

    /* 创建模块内部的类型表(和上边一样) */
    ALLOC (t);
    t->level    = Level;
    t->outer    = Tags;
    t->buckets  = NULL;
    Tags        = t;
}

/* 退出一个模块 */
void ExitScope (void)
{
    Level--;
    /* 还原符号表的指向 */
    Identifiers = Identifiers->outer;
    Tags = Tags->outer;
}

/* 查找某个标示符 */
Symbol LookupID (const char *name) 
{
    return LookupSymbol (Identifiers, name);
}

/* 查找某个类型 */
Symbol LookupTag (const char *name) 
{
    return LookupSymbol (Tags, name);
}

Symbol DeleteDefine (char *name) 
{
    return DeleteSymbol (&DefineTable, name);
}

Symbol AddDefine (char *name, char *content)
{
    Symbol p, res;
    CALLOC (p);

    p->name = name;
    p->val.p = content;

    res = LookupSymbol (&DefineTable, name);
    if (res == NULL) {

        return AddSymbol (&DefineTable, p);
    }

    // 添加时无论前边的define 定义是否有效,均覆盖
    res->unvalid = false;
    res->val.p = content;
    return res;
}

Symbol LookupDefine (const char *name) 
{
    Symbol p = LookupSymbol (&DefineTable, name);
    return (!p || p->unvalid) ? NULL : p;
}

/* 添加类型到符号表中 */
Symbol AddTag (char *name, Type ty)
{
    Symbol p;
    CALLOC (p); 
    /* 类型的类别码, 名字和类型 */
    p->kind = SK_Tag;
    p->name = name;
    p->ty = ty;

    return AddSymbol (Tags, p);
}

/* 添加一个常量到常量符号表中 */
Symbol AddEnumConstant (char *name, Type ty, int val) 
{
    Symbol p;
    CALLOC (p);

    p->kind = SK_EnumConstant;
    p->name = name;
    p->ty   = ty;
    /* 常量的值 */
    p->val.i[0] = val;

    return AddSymbol (Identifiers, p);
}

/* 添加Typedef 自定义类型到符号表中 */
Symbol AddTypedefName (char *name, Type ty) 
{
    Symbol p;
    CALLOC (p);

    p->kind = SK_TypedefName;
    p->name = name;
    p->ty = ty;
    return AddSymbol (Identifiers, p);
}

/* 添加一个变量到符号表中 */
Symbol AddVariable (char *name, Type ty, int sclass) 
{
    VariableSymbol p;
    CALLOC (p);

    p->kind = SK_Variable;
    p->name = name;
    p->ty   = ty;
    /* 变量的类别 */
    p->sclass = sclass;

    /* 全局变量,或静态变量 */
    if (0 == Level || sclass == TK_STATIC) {
    
        /* 添加全局变量 */
        *GlobalTail = (Symbol)p;
        GlobalTail  = &p->next;
    } else if (TK_EXTERN != sclass) {
    
        /* 局部变量(FSYM 指向当前函数) */
        *FSYM->lastv = (Symbol)p;
        FSYM->lastv  = &p->next;
    }
    return AddSymbol (Identifiers, (Symbol)p);
}

/* 添加一个函数到符号表中 */
Symbol AddFunction (char *name, Type ty, int sclass) 
{
    FunctionSymbol p;
    CALLOC (p);

    p->kind = SK_Function;
    p->name = name;
    p->ty   = ty;
    p->sclass = sclass;
    /* 指向函数形参 */
    p->lastv = &p->params;

    *FunctionTail = (Symbol)p;
    FunctionTail  = &p->next;
    return AddSymbol (&GlobalIDs, (Symbol)p);
}

/* 添加常量到符号表中 */
Symbol AddConstant (Type ty, union value val) 
{
    unsigned h = (unsigned)val.i[0] & SYM_HASH_MASK;
    Symbol p;

    /* 去掉修饰符 */
    ty = Unqual (ty);
    if (IsIntegType (ty)) {
        ty = T (INT);
    } else if (IsPtrType (ty)) {
        ty = T (POINTER);
    } else if (LONGDOUBLE == ty->categ) {
        ty = T (DOUBLE);
    }

    /* 如果已经添加直接返回 */
    for (p = Constants.buckets[h]; p; p = p->link) {
        if (p->ty == ty && p->val.i[0] == val.i[0] && p->val.i[1] == val.i[1]) 
            return p;
    }

    /* 将常量添加到符号表中 */
    CALLOC (p);
    p->kind = SK_Constant;
    switch (ty->categ) {
        case INT:
            p->name = FormatName ("%d", val.i[0]); break;
        case POINTER:
            p->name = (val.i[0] ? FormatName ("0x%x", val.i[0]) : "0"); break;
        case FLOAT:
            /* %g可以省略浮点多余的0 */
            p->name = FormatName ("%g", val.f); break;
        case DOUBLE:
            p->name = FormatName ("%g", val.d); break;
        default:
            assert (0);
    }
    p->ty       = ty;
    p->sclass   = TK_STATIC;
    p->val      = val;

    p->link = Constants.buckets[h];
    Constants.buckets[h] = p;
    if (FLOAT == ty->categ || DOUBLE == ty->categ) {
        
        *FloatTail = p;
        FloatTail = &p->next;
    }
    return p;
}

/* 添加int 类型的常量到符号表 */
Symbol IntConstant (int i) 
{
    union value val = {};
    val.i[0] = i;
    return AddConstant (T(INT), val);
}

/* 添加字符串到字符串符号表中 */
Symbol AddString (Type ty, String str) 
{
    Symbol p;
    CALLOC (p);

    /* 设置p的属性 */
    p->kind = SK_String;
    p->name = FormatName ("str%d", StringNum++);
    p->ty   = ty;
    p->sclass = TK_STATIC;
    p->val.p = str;
    /* 添加到字符串符号表中 */
    *StringTail = p;
    StringTail = &p->next;
    return p;
}

/* 创建函数内部的临时变量 */
Symbol CreateTemp (Type ty) 
{
    VariableSymbol p;
    CALLOC (p);

    p->kind = SK_Temp;
    p->name = FormatName ("t%d", TempNum++);
    p->ty   = ty;
    p->level = 1;

    /* 添加到函数符号表中 */
    *FSYM->lastv = (Symbol)p;
    FSYM->lastv  = &p->next;
    return (Symbol)p;
}
 
/* 给基本块创建名字 */
Symbol CreateLabel (void) 
{
    Symbol p;
    CALLOC (p);

    p->kind = SK_Label;
    p->name = FormatName ("BB%d", LabelNum++);
    return p;
}

/* 创建偏移符号 */
Symbol CreateOffset (Type ty, Symbol base, int coff) 
{
    VariableSymbol p;
    if (0 == coff) 
        return base;

    CALLOC (p);
    if (SK_Offset == base->kind) {
    
        coff += AsVar(base)->offset;
        base = base->link;
    }
    p->addressed = 1;
    p->kind      = SK_Offset;
    p->ty        = ty;
    p->link      = base;
    p->offset    = coff;
    p->name      = FormatName ("%s[%d]", base->name, coff);
    base->ref++;
    return (Symbol)p;
}

/* 初始化符号表 */
void InitSymbolTable (void) 
{
    /* 初始化符号表 */
    GlobalTags.buckets = GlobalIDs.buckets = NULL;
    GlobalTags.outer = GlobalIDs.outer = NULL;
    GlobalTags.level = GlobalIDs.level = 0;

    /* Tags 和Identifiers 分别指向两个符号表 */
    Tags        = &GlobalTags; 
    Identifiers = &GlobalIDs; 

    /* 给常量符号表分配存储空间 */
    int size = sizeof (Symbol) * (SYM_HASH_MASK + 1);
    Constants.buckets = HeapAllocate (CurrentHeap, size);
    memset (Constants.buckets, 0, size);
    Constants.outer = NULL;
    Constants.level = 0;

    Functions   = Globals = Strings = FloatConstants = NULL;
    FunctionTail = &Functions;
    GlobalTail  = &Globals;
    StringTail  = &Strings;
    FloatTail   = &FloatConstants;

    TempNum = LabelNum = StringNum = 0;
}
