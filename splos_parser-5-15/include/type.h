/*************************************************************************
	> File Name: type.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月14日 星期五 13时03分27秒

	> Description: 
 ************************************************************************/

#ifndef __TYPE_H
#define __TYPE_H
 
/****************************************************
 * 数据结构的定义
 ****************************************************/ 

/* 整形：char 到enum
 * 浮点：float 到 long double 
 * 指针和空类型：pointer void
 * 复合类型：union struct 
 * 数组：array 
 * 函数：function 
 * */
enum {

    CHAR, UCHAR, SHORT, USHORT, INT, UINT, LONG, ULONG, LONGLONG, ULONGLONG, ENUM, 
    FLOAT, DOUBLE, LONGDOUBLE, POINTER, VOID, UNION, STRUCT, ARRAY, FUNCTION
}; 

/* const 和volatile 修饰符 */
enum {CONST = 0x1, VOLATILE = 0x2 };

/* 不同类型的标号(根据占用字节的大小)
 * I1: signed 1 byte       U1: unsigned 1 byte
 * I2: signed 2 byte       U2: unsigned 2 byte
 * I4: signed 4 byte       U4: unsigned 4 byte
 * F4: 4 byte floating     F8: 8 byte floating
 * V: no type              B: memory block, used for struct/union and array.
 * */
enum {I1, U1, I2, U2, I4, U4, F4, F8, V, B};

/* 所有类型公有的字段 
 * categ: 类别上边枚举的类型的类别码
 * qual:  类型修饰符(const 或者volatile)
 * size:  类型的大小
 * bty:   该类型的基类(为函数时,表示返回值类型)*/
#define TYPE_COMMON     \
        int categ : 8;  \
        int qual  : 8;  \
        int align : 16; \
        int size;       \
        struct type *bty

/* 基本类型的数据结构 */
typedef struct type {

    TYPE_COMMON;
} *Type;

/* 枚举类型的数据结构 */
typedef struct enumType {

    TYPE_COMMON;
    /* 枚举的名称 */
    char *id;
} *EnumType;

/* 描述一个字段 */
typedef struct field {

    /* 相对变量的起始偏移 */
    int offset;
    /* 字段名称 */
    char *id;
    /* 段的位数 */
    int bits;
    /* 位段的开始位置 */
    int pos;
    /* 该字段类型 */
    Type ty;
    /* 指向下一个字段 */
    struct field *next;
} *Field;

/* 描述struct 和union 类型的数据结构 */
typedef struct recordType {

    TYPE_COMMON;
    /* 复合类型的名称 */
    char *id;
    /* 所有字段的列表 */
    Field flds;
    Field *tail;
    /* 是否包含常量类型的字段 */
    int hasConstFld  :16;
    /* 是否包含灵活数组(即大小为0 的数组),
     * 灵活数组必须是最后一个字段 */
    int hasFlexArray :16;
} *RecordType;

/* 函数头部信息的描述 */
typedef struct signature {

    /* 有无参数 */
    int hasProto    :16;
    /* 是否拥有可变参 */
    int hasEllipse  :16;
    /* 参数集合 */
    Vector params;
} *Signature;

/* 函数类型描述 */
typedef struct functionType {
    
    TYPE_COMMON;
    /* 函数的参数列表 */
    Signature sig;
} *FunctionType;

/* 函数参数的描述 */
typedef struct parameter {
    
    /* 参数名 */
    char *id;
    /* 参数类型 */
    Type ty;
    /* 是否被register 修饰 */
    int reg;
} *Parameter;



/****************************************************
 * 接口定义
 ****************************************************/ 
/* Tyeps 基本类型数组在type.c 中定义 */
#define T(categ)            (Types + categ)
/* 所有整形类型的 */
#define IsIntegType(ty)     (ENUM >= ty->categ)
/* 在定义中单数都是unsigned 所修饰的 */
#define IsUnsigned(ty)      (IsIntegType(ty) && ty->categ & 0x1)
/* 判断浮点类型 */
#define IsRealType(ty)      (ty->categ >= FLOAT && ty->categ <= LONGDOUBLE)
/* 整形和浮点类型 */
#define IsArithType(ty)     (ty->categ <= LONGDOUBLE)
/* 整形 浮点和指针类型 */
#define IsScalarType(ty)    (ty->categ <= POINTER)
/* 判断指针类型 */
#define IsPtrType(ty)       (ty->categ == POINTER)
/* 构造类型 */
#define IsRecordType(ty)    (ty->categ == STRUCT || ty->categ == UNION)
/* 函数类型 */
#define IsFunctionType(ty)  (ty->categ == FUNCTION)
/* 完整的指针类型(指类大小不为0,切非函数类型) */
#define IsObjectPtr(ty)     (ty->categ == POINTER && ty->bty->size != 0 && ty->bty->categ != FUNCTION)
/* 不完整的指针类型(指类的大小为0) */
#define IsIncompletePtr(ty) (ty->categ == POINTER && ty->bty->size == 0)
/* void 指针 */
#define IsVoidPtr(ty)       (ty->categ == POINTER && ty->bty->categ == VOID)
/* 非函数指针 */
#define NotFunctionPtr(ty)  (ty->categ == POINTER && ty->bty->categ != FUNCTION)

/* 两个类型均是整形 */
#define BothIntegType(ty1, ty2)     (IsIntegType(ty1) && IsIntegType(ty2))
/* 两个类型均是整形或者浮点 */
#define BothArithType(ty1, ty2)     (IsArithType(ty1) && IsArithType(ty2))
/* 两个类型均是整形或者浮点或者指针 */
#define BothScalarType(ty1, ty2)    (IsScalarType(ty1) && IsScalarType(ty2))
/* 两个指针类型是否相互兼容 */
#define IsCompatiblePtr(ty1, ty2)   (IsPtrType(ty1) && IsPtrType(ty2) &&  \
    IsCompatibleType(Unqual(ty1->bty), Unqual(ty2->bty)))


/* 将类型转换成字符串 */
const char* TypeToString (const Type ty);
/* 设置基本类型的属性,从config.h 中读取属性的配置 */
void    SetupTypeSystem (void);
/* 添加修饰符 */
Type    Qualify (int qual, Type ty);
/* 去掉修饰符 */
Type    Unqual (Type ty);
/* 构造一个数组(长度和类型) */
Type    ArrayOf (int len, Type ty);
/* 构造一个构造类型(初始化) */
Type    StartRecord (char *id, int categ);
/* 处理struct/union 的每个字段的位置 */
void    EndRecord (Type ty);
/* 添加一个字段到struct/union 中 */
Field   AddField (Type ty, char *id, Type fty, int bits);
/* 将ty 设为指针类型,如int 将返回int* */
Type    PointerTo (Type ty);
/* 构造一个函数类型,返回值类型为ty */
Type    FunctionReturn (Type ty, Signature sig);
/* 若是数组或函数,则返回指针类型 */
Type    AdjustParameter (Type ty);
/* 在构造类型ty 中查找 id这个字段 */
Field   LookupField (Type ty, char *id);
/* 构造一个枚举类型 */
Type    Enum (char *id);
/* 检查两个类型是否兼容 */
int     IsCompatibleType (Type ty1, Type ty2);
Type    CompositeType (Type ty1, Type ty2);
int     TypeCode (Type ty);
/* 返回类型级别最高的,或占字节数最多的类型 */
Type    CommonRealType (Type ty1, Type ty2);
/* 提升类型,级别小于int的提升为int,是float的提升为double,其他类型的不变 */
Type    Promote (Type ty);


/* 宽字符类型 */
extern struct type Types[VOID - CHAR + 1];
extern Type WCharType;
extern Type DefaultFunctionType;


#endif
