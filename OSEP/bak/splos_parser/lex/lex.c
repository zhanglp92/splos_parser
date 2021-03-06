/*************************************************************************
	> File Name: lex.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月15日 星期六 21时38分03秒

	> Description: 词法分析
 ************************************************************************/

#include "pcl.h"
#include "lex.h"
#include "keyword.h"
#include "config.h"

/* 文件内容的当前位置 */
#define CURRENT     (Input.current)
#define LINE        (Input.line)
#define LINEHEAD    (Input.lineHead)

/* 定义一个函数类型 */
typedef int (*Scanner)(void);
/* 窃取目前扫描到的位置 */
static unsigned char    *PeekPoint;
/* 窃取当前扫描的单词的值 */
static union value      PeekValue;
/* 窃取当前扫描单词在文件中的坐标 */
static struct coord     PeekCoord;
/* 该数组挂载,各自对用的处理函数 */
static Scanner Scanners[256];
/* 当前词法单元在文件中的坐标 */
struct coord TokenCoord;
/* 在处理include 时使用 */
//struct coord PreTokenCoord;
/* 前一个词法单元在文件中的坐标 */
struct coord PrevCoord;
/* 当前词法单元的值及个别属性 */
union value TokenValue;
char *TokenString[] = {
 
    NULL, 
    #define TOKEN(k, s) s, 
    #include "token.h"
    #undef  TOKEN
};

/* 加载文件时使用的链表, 保存前边的信息 */
Vector InLink;
unsigned endifCnt;

/* 存储外部指定搜索头文件的路径 */
Vector UserIncPath;

int GetNextToken (void);
static int ScanIdentifierOR (int def);
static int ScanEOF (void);
static void FindAndLoadFile (char *filename, char **IncPath);

static int ScanIdentifier (void) 
{
    return ScanIdentifierOR (false);
}

static int ScanIdentifierDef (void) 
{
    return ScanIdentifierOR (true);
}


/* 处理转移字符 */
static int ScanEscapeChar (int wide)
{
    int i, v, overflow;
    /* 跳过转移字符的 \ */
    CURRENT++;

    switch (*CURRENT++) {

        /* 返回转义后的ASCII 值 */
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';

        /* 返回源字符的ASCII 值即可 */
        case '\'': case '"': 
        case '\\': case '\?': return *(CURRENT-1);

        case 'x': 
            /* ｘ之后若不是十六进制的数字则错误 */
            if (!IsHexDigit (*CURRENT)) {
    
                Error (&TokenCoord, "Expect hex digit");
                return 'x';
            }
            /* 如果非宽字符
             * 且从０开始可处理3个字符,
             * 非０开始可处理2个字符 
             * 如果宽字符
             * 且从０开始可处理9个字符,
             * 非０开始可处理8个字符 */
            i = ('0' == *CURRENT ? 0 : 1) + (wide ? 0 : 6);
            for (v = 0; i < 9 && IsHexDigit (*CURRENT); CURRENT++, i++) {
                v = (v << 4) + (IsDigit (*CURRENT) ? (*CURRENT-'0') : 
                    (ToUpper (*CURRENT) - 'A' + 10));
            }
            /* 如果ｉ不到９则字符中间有非法字符 */
            if (9 != i) 
                Warning (&TokenCoord, "Hexademical espace sequence overflo");
            return v;

        /* 处理八进制 */
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7': 
            /* 如果非宽字符
             * 且从０开始可处理三个字符,
             * 非０开始可处理两个字符 
             * 若是宽字符可处理三个字符 */
            v = *(CURRENT - 1) - '0';
           //i = wide ? 
            for (i = v ? 1 : 0; i < 2 && IsOctDigit (*CURRENT); i++) 
                v = (v << 3) + *CURRENT++ - '0';
            return v;
        default: 
            Warning (&TokenCoord, "Unrecognized escape sequence: \\%c", *CURRENT);
            return *CURRENT;
    }
}

/* 字符的处理函数 */
static int ScanCharLiteral (void) 
{
    /* 是否为宽字符标示位 */
    unsigned char wide = 0;
    int count = 0, ch = 0;

    if ('L' == *CURRENT) {

        /* 跳过款字符标示Ｌ，设置标志位 */
        CURRENT++;
        wide = 1;
    }
    /* 跳过' 字符*/
    CURRENT++;

    /* 处理字符 */
    while ('\'' != *CURRENT) {
    
        /* 不合法 */
        if ('\n' == *CURRENT || *CURRENT == END_OF_FILE) 
            break;

        /* 处理转移字符 */
        ch = *CURRENT == '\\' ? ScanEscapeChar (wide) : *CURRENT++;
        count++;
    }
    
    /* 判断结束的字符是否为' */
    if ('\'' != *CURRENT) {
        
        Error (&TokenCoord, "Expect '");
        goto end_char;
    }
    CURRENT++;

    if (1 < count) 
        Warning (&TokenCoord, "Tow many characters");

end_char:
    /* 存储该字符的值 */
    TokenValue.i[0] = ch;
    TokenValue.i[1] = 0;
    /* 返回字符的类型 */
    return TK_INTCONST;
}

/* include import 加载文件 */
static int LoadFile (char *filename)
{
    /* 如果文件存在,
     * 保存以前的Input 信息
     * 读取新的文件 */
    if (FileExist (filename)) {
        
        Include p;
        CALLOC (p);

        p->PreInput = Input;
        p->isDef = false;
        ReadSourceFile (filename);
        p->PreTokenCoord = TokenCoord;
        p->endifCnt = endifCnt;
        INSERT_ITEM (InLink, p);

        TokenCoord.filename = filename;
        TokenCoord.line = TokenCoord.col = TokenCoord.ppline = 1;
        endifCnt = 0;
        INFO (4, "load %s file.", filename);

        return 1;
    }
    return 0;
}

static int LoadDefine (Symbol def)
{
    Include p;
    CALLOC (p);

    p->PreInput = Input;
    p->isDef = true;
    Input.filename = def->name;
    Input.base = def->val.p;
    Input.size = strlen (Input.base);
    Input.base[Input.size] = END_OF_FILE;
    Input.current = Input.lineHead = Input.base;
    Input.line = 1;

    p->PreTokenCoord = TokenCoord;
    p->endifCnt = endifCnt;
    INSERT_ITEM (InLink, p);

    TokenCoord.filename = def->name;
    TokenCoord.line = TokenCoord.col = TokenCoord.ppline = 1;
    endifCnt = 0;
    INFO (4, "load %s define.", def->name);
}

// 跳过设置的空白集合
static void SKIP_SPACE (char *spaceVector) 
{
    // 默认空白符集合
    char space[] = " \t";
    
    while (1) {

        char *tmp = spaceVector ? spaceVector : space;
        for ( ;*tmp ; tmp++) {
            
            if (*tmp == *CURRENT) {

                CURRENT++;
                break ;
            }
        }

        if ( !*tmp )
            break;
        else if ( *tmp == '\n' ) {

            TokenCoord.ppline++;
            LINE++;
            LINEHEAD = CURRENT;
        }
    }
}

static void ImportLoadFile (void)
{
    char *filename  = NULL;
    char space[]    = " \t\n";
    char *IncPath[] = {

            ".",
            #define INC_PATH(path) path, 
            #include "confg/inc_path.h"
            #undef  INC_PATH
            NULL
    };

    SKIP_SPACE (space);

    filename = CURRENT;
    while (';' != *CURRENT && END_OF_FILE != *CURRENT)
        CURRENT++;

    filename = InternStr (filename, (char*)CURRENT - filename);

    if (';' != *CURRENT) {
    
        Error (&TokenCoord, "Expect ;");
    } else CURRENT++;

    /* 读到下一个不为空的字符 */
    SKIP_SPACE (space);

    FindAndLoadFile (filename, IncPath);
}

void Import (void)
{

    while (' ' == *CURRENT || '\t' == *CURRENT)  \
        CURRENT++;
    
    if ('{' == *CURRENT) {

        CURRENT++;
        Input.importRbrace += 1;
    }

    ImportLoadFile ();

}

static void FindAndLoadFile (char *filename, char **IncPath)
{
    int i, flags = 0;

    /* 准备加载include 文件 */
    for (i = 0; IncPath[i]; i++) {
    
        char *path;
        if ( !(path = calloc ((strlen (IncPath[i]) + strlen (filename) + 2), sizeof (char)))) {

            Fatal ("calloc failer!");
        }
        strcpy (path, IncPath[i]);
        strcat (path, "/");
        strcat (path, filename);
 
        if (LoadFile (path)) {

            flags = 1;
            break;
        }
    }



    // flags == 0 表示文件没有找到继续查找
    if (0 == flags && UserIncPath) {
        
        char *userPath;
        FOR_EACH_ITEM (char*, userPath, UserIncPath)
            char *path;
            if ( !(path = calloc ((strlen (userPath) + strlen (filename) + 2), sizeof (char)))) {

                Fatal ("calloc failer!");
            }
            strcpy (path, userPath);
            strcat (path, "/");
            strcat (path, filename);

            if (LoadFile (path)) {

                flags = 1;
                goto exit;
            }
        ENDFOR
    }

exit:
    if (0 == flags)
        Error (&TokenCoord, "load %s fail!", filename);
}

static void ScanPPInclude (void)
{
    SKIP_SPACE (NULL);
    /* 分别以< 和" 包含的头文件 */
    if ('<' == *CURRENT) {

        char *filename = NULL;
        char *IncPath[] = {

            #define INC_PATH(path) path, 
            #include "confg/inc_path.h"
            #undef  INC_PATH
            NULL
        };

        /* 取得文件名 */
        filename = ++CURRENT;
        while ('>' != *CURRENT && *CURRENT != END_OF_FILE && '\n' != *CURRENT) 
            CURRENT++;

        if ('>' != *CURRENT) {
            
            Error (&TokenCoord, "Expect >");
        }

        filename = InternStr (filename, (char*)CURRENT - filename);
        /* 读完此行 */
        while ('\n' != *CURRENT && END_OF_FILE != *CURRENT)
            CURRENT++;

        FindAndLoadFile (filename, IncPath);
    } else if ('\"' == *CURRENT) {

        /* 双引号的先在当前目录查找 */
        char *filename = NULL;
        char *IncPath[] = {

            ".",
            #define INC_PATH(path) path, 
            #include "confg/inc_path.h"
            #undef  INC_PATH
            NULL
        };

        /* 取得文件名 */
        filename = ++CURRENT;
        while ('\"' != *CURRENT && *CURRENT != END_OF_FILE && '\n' != *CURRENT) 
            CURRENT++;

        if ('\"' != *CURRENT) {
            
            Error (&TokenCoord, "Expect \"");
        }

        filename = InternStr (filename, (char*)CURRENT - filename);
        /* 读完此行 */
        while ('\n' != *CURRENT && END_OF_FILE != *CURRENT)
            CURRENT++;

        FindAndLoadFile (filename, IncPath);
    }
}

static unsigned char* ReadLine (void)
{
    unsigned char *line;
    unsigned char *tmp;
    unsigned len = 0, i, k;

    SKIP_SPACE (NULL);
    tmp = CURRENT;

    while ( 1 ) {
    
        if ('\\' == *CURRENT) {
            
            if ('\n' == CURRENT[1]) {

                CURRENT += 2;
                TokenCoord.ppline++;
                LINE++;
                LINEHEAD = CURRENT;
            }
            else {

                Error (&TokenCoord, "\'\\\' error");
                break ;
            }
        } else if ('\n' == *CURRENT) {

            TokenCoord.ppline++;
            LINE++;
            LINEHEAD = ++CURRENT;
            break; 
        }
        else CURRENT++;
    }

    return InternStr (tmp, CURRENT - tmp);
}

static void ScanPPDefine (void) 
{
    char *name, *content;

    SKIP_SPACE (NULL);
    if (TK_ID != ScanIdentifierDef ()) {

        Error (&TokenCoord, "Define doesn't define key words");
    }
    name = TokenValue.p;

    if ('(' == *CURRENT) {

        PRINT ("待处理 \n");
        exit (1);
    }
    
    content = ReadLine ();
    AddDefine (name, content);
    INFO (5, "define : %s, %s", name, content);
}

static void ScanPPUndef (void)
{
    SKIP_SPACE (NULL);
    if (TK_BEGIN == ScanIdentifierDef ()) {

        Error (&TokenCoord, "ifdef use error!");
        return ;
    }

    if ( NULL == DeleteDefine (TokenValue.p))
        Error (&TokenCoord, "define %s don't exsit!", TokenValue.p);
}

static void ScanPPEndif (void)
{
    if (endifCnt <= 0) 
        Error (&TokenCoord, "undef use error!");
    else endifCnt--;

    SKIP_SPACE (" \t\n");
}

#define ScanPPIfdef()  ScanPPIfdefOrNdef (true)
#define ScanPPIfndef() ScanPPIfdefOrNdef (false)

// defOrNot true 表示def 
static void ScanPPIfdefOrNdef (int defOrNot)
{
    // ifdef 需要对应的 endif 
    endifCnt++;

    SKIP_SPACE (NULL);
    if (TK_BEGIN == ScanIdentifierDef ()) {

        Error (&TokenCoord, defOrNot ? "ifdef" : "ifndef" " use error!");
        return ;
    }

    if ((NULL != LookupDefine (TokenValue.p)) ^ defOrNot) {

        #define ReadLine \
            while ( *CURRENT == '\n' ) \
                CURRENT++; \
            TokenCoord.ppline++; \
            LINE++; \
            LINEHEAD = CURRENT++
        
        while ( 1 ) {
        
            // endif 不能夸文件
            if (*CURRENT == END_OF_FILE)
                break;

            SKIP_SPACE (" \t\n");
            if ( !strncmp (CURRENT, "#endif ", 7) || \
                 !strncmp (CURRENT, "#endif\t", 7) || \
                 !strncmp (CURRENT, "#endif\n", 7)) {
                
                CURRENT += 6;
                ScanPPEndif ();
                break;
            }
            ReadLine ;
        }
        #undef ReadLine
    }
}

static void ScanPreprocessor (void)
{
    // 预处理的#
    CURRENT++;

    if (!strncmp (CURRENT, "include", 7)) {
    
        CURRENT += 7;
        ScanPPInclude ();
    } else if (!strncmp (CURRENT, "define ", 7) || \
              !strncmp (CURRENT, "define\t", 7)) {
        
        CURRENT += 7;
        ScanPPDefine ();
    } else if (!strncmp (CURRENT, "undef ", 6) || \
               !(strncmp (CURRENT, "undef\t", 6))) {
                
        CURRENT += 6;
        ScanPPUndef ();
    } else if (!strncmp (CURRENT, "ifdef ", 6) || \
               !strncmp (CURRENT, "ifdef\t", 6)) {
                   
        CURRENT += 6;
        ScanPPIfdef ();
    } else if (!strncmp (CURRENT, "endif ", 6) || \
                 !strncmp (CURRENT, "endif\t", 6) ||\
                 !strncmp (CURRENT, "endif\n", 6)) {

        CURRENT += 6;
        ScanPPEndif ();
    } else if (!strncmp (CURRENT, "ifndef ", 7) || \
               !strncmp (CURRENT, "ifndef\t", 7)) {

        CURRENT += 7;
        ScanPPIfndef ();
    } else if (!strncmp (CURRENT, "import", 6)) {

        CURRENT += 6;
        Import ();
    }
}

/* 跳过空白符和预处理 */
static void SkipWhiteSpace (void) 
{
    int ch;

again:
    ch = *CURRENT;
    while (ch == '\t' || ch == '\v' || ch == '\f' || ch == ' '
        || ch == '\r' || ch == '\n' || ch == '/' || ch == '#') {

        switch (ch) {

            /* 新的一行 */
            case '\n': 
                TokenCoord.ppline++;
                LINE++;
                LINEHEAD = ++CURRENT;
                break;

            /* 一行预处理 */
            case '#':
                ScanPreprocessor ();
                break;

            /* 注释 */
            case '/':
                /* 非注释 */
                if ('/' != CURRENT[1] && '*' != CURRENT[1]) 
                    return ;

                /* 双斜线注释 */
                if ('/' == CURRENT[1]) {

                    while ('\n' != *CURRENT && END_OF_FILE != *CURRENT) 
                        CURRENT++;
                } else {

                    for (CURRENT +=2; '*' != CURRENT[0] || '/' != CURRENT[1]; CURRENT++) {

                        /* 下一行 */
                        if ('\n' == *CURRENT) {
    
                            TokenCoord.ppline++;
                            LINE++;
                        } else if (END_OF_FILE == CURRENT[0] || END_OF_FILE == CURRENT[1]) {

                            Error (&TokenCoord, "Comment is not closed");
                            return ;
                        }
                    }
                    CURRENT += 2;
                }
                break; 

            default:
                CURRENT++;
                break;
        }
        ch = *CURRENT;
    }

    /* 过滤用户自定义的空白符 */
    if (ExtraWhiteSpace) {
    
        char *p;
        /* 遍历 ExtraWhiteSpace 集合 */
        FOR_EACH_ITEM (char*, p, ExtraWhiteSpace) 
            if (strncmp (CURRENT, p, strlen (p))) {
    
                /* 过滤掉用户自定义的空白符 */
                CURRENT += strlen (p);
                /* 重新过滤 */
                goto again;
            }
        ENDFOR
    }
}

/* 扫描一个字符串 */
/* 可以将例如 "hello" " world"这样的字符串
 * 合并成"hello world" */
static int ScanStringLiteral (void)
{
    /* 表示是否是宽字符串 */
    int wide = 0;
    int maxlen = 512;
    int ch;
    int len = 0;

    /* 存储字符串的数组 */
    char tmp[512];
    /* 存储普通字符串 */
    char *cp = tmp;
    /* 存储宽字符串 */
    int *wcp = (int*)tmp;
    /* 保存整个字符串 */
    String str;
    CALLOC (str);

    /* 判断是否为宽字符串 */
    if ('L' == *CURRENT) {

        CURRENT++;
        wide = 1;
        maxlen /= WCHAR_SIZE;
    }
    /* 跳过前双引号 */
    CURRENT++;

next_string:
    /* 扫描串 */
    while ('"' != *CURRENT) {
    
        if ('\n' == *CURRENT || END_OF_FILE == *CURRENT) 
            break;
        /* 处理字符串中的字符 */
        ch = *CURRENT == '\\' ? ScanEscapeChar (wide) : *CURRENT++;
        /* 将得到的字符加入到数组中 */
        wide ? (wcp[len++] = ch) : (cp[len++] = (char)ch);
        if (len >= maxlen) {
        
            AppendSTR (str, tmp, len, wide);
            len = 0;
        }
    }
    /* 如果不是后引号则结束 */
    if ('"' != *CURRENT) {
    
        Error (&TokenCoord, "Expect \"");
        goto end_string;
    }
    CURRENT++;
    /* 处理串中的空白符(包括用户自定义空白符) */
    SkipWhiteSpace ();
    /* 判断后边是否还有字符串 */
    if ('"' == CURRENT[0]) {
    
        /* 前边的字符串和后边不匹配 */
        if (wide) {
            Error (&TokenCoord, "String wideness mismatch");
        }
        CURRENT++;
        /* 继续扫描 */
        goto next_string;
    } else if ('L' == CURRENT[0] && '"' == CURRENT[1]) {
        
        if (0 == wide) {
            Error (&TokenCoord, "String wideness mismatch");
        }
        CURRENT += 2;
        goto next_string;
    }

end_string:
    /* 将得到的tmp 追加到str 中 */
    AppendSTR (str, tmp, len, wide);
    TokenValue.p = str;
    return wide ? TK_WIDESTRING : TK_STRING;
}

/* 查找关键字，返回类别码 */
static int FindKeyword (const char *str, int len) 
{
    struct keyword *p = NULL;
    int index = 0;

    if ('_' != *str) 
        index = ToUpper (*str) - 'A' + 1;
    /* 遍历查找，返回类别码 */
    for (p = keywords[index]; p->name; p++) {
    
        if (p->len == len && !strncmp (str, p->name, len)) 
            return p->tok;
    }

    /* 若非关键字则返回标示符类别码 */
    return TK_ID;
}

/* 标示符扫描函数 */
static int ScanIdentifierOR (int def) 
{
    unsigned char *start = CURRENT;
    int tok;

    /* 处理宽字符或宽字符串 */
    if ('L' == *CURRENT) {
    
        /* 处理字符 */
        if ('\'' == CURRENT[1]) 
            return ScanCharLiteral ();

        /* 处理宽字符串 */
        if ('"' == CURRENT[1]) 
            return ScanStringLiteral ();
    }

    if ( !IsLetterOrDigit (*CURRENT) )
        return TK_BEGIN;

    CURRENT++;
    /* 字母和数字下画线都符合标示符的要求 */
    while (IsLetterOrDigit (*CURRENT)) CURRENT++;

    /* 判断是否是关键字 */
    tok = FindKeyword ((char*)start, (int)(CURRENT - start));

    if (TK_ID == tok) {
    
        Symbol p;
        /* 将字符串的值保存在字符串池中 */
        TokenValue.p = InternStr ((char*)start, (int)(CURRENT - start));
        
        // 该符号是define 
        if (!def && (p = LookupDefine (TokenValue.p))) {

            LoadDefine (p);
            return GetNextToken ();
        }  

    } else 
        TokenValue.p = TokenString[tok];

    return tok;
}

/* 判断浮点小数点后边的小数部分 */
static int ScanFloatLiteral (const unsigned char *start) 
{
    double d;

    /* 扫描以点开始的串 */
    if ('.' == *CURRENT) {
        while (IsDigit (*++CURRENT));
    }
    /* 扫描Ｅ后边紧跟的字符 */
    if ('e' == *CURRENT || 'E' == *CURRENT) {

        if ('+' == *++CURRENT || '-' == *CURRENT) 
            CURRENT++;
        if (!IsDigit (*CURRENT)) {
            Error (&TokenCoord, "Expect exponent value");
        } else {
            while (IsDigit (*++CURRENT));
        }
    }

    errno = 0;
    /* 计算小数部分的数值 */
    d = strtod ((char*)start, NULL);
    if (ERANGE == errno) {
        Warning (&TokenCoord, "Float literal overflow");
    }

    /* 校验数值和类别码 */
    TokenValue.d = d;
    if ('f' == *CURRENT || 'F' == *CURRENT) {
    
        CURRENT++;
        TokenValue.f = (float)d;
        return TK_FLOATCONST;
    } else if ('l' == *CURRENT || 'L' == *CURRENT) {

        CURRENT++;
        return TK_LDOUBLECONST;
    } else {
        return TK_DOUBLECONST;
    }
}

/* 处理整数部分 */
static int ScanIntLiteral (unsigned char *start, int len, int base) 
{
    unsigned char *p = start, *end = start + len;
    unsigned int i[2] = {0, 0};
    int tok = TK_INTCONST;
    int d = 0;
    int carry0 = 0, carry1 = 0;
    int overflow = 0;

    /* 将字符串转换成对应的数值 */
    for (; p != end; p++) {

        /* 取得一位数值 */
        if (16 == base) {
    
            if (IsLetter (*p)) {
                d = ToUpper (*p) - 'A' + 10;
            } else {
                d = *p - '0';
            }
        } else {
            d = *p - '0';
        }

        /* 将取得的一位添加进去 */
        switch (base) {

            case 16: 
                carry0 = HIGH_4BIT (i[0]);
                carry1 = HIGH_4BIT (i[1]);
                i[0] <<= 4; i[1] <<= 4; break;

            case 8:
                carry0 = HIGH_3BIT (i[0]);
                carry1 = HIGH_3BIT (i[1]);
                i[0] <<= 3; i[1] <<= 3; break;

            case 10: {
                unsigned int t1, t2;
                carry0 = HIGH_3BIT (i[0]) + HIGH_1BIT (i[0]);
                carry1 = HIGH_3BIT (i[1]) + HIGH_1BIT (i[1]);
                t1 = i[0] << 3, t2 = i[0] << 1;
                if (UINT_MAX - t2 < t1) 
                    carry0++;
                i[0] = t1 + t2;
                t1 = i[1] << 3;
                t2 = i[1] << 1;
                if (UINT_MAX - t2 < t1) 
                    carry1++;
                i[1] = t1 + t2;
            } break;
        }
        if (UINT_MAX - d < i[0]) 
            carry0 += i[0] - (UINT_MAX - d);
        if (carry1 || (i[1] > UINT_MAX - carry0)) 
            overflow = 1;
        i[0] += d, i[1] += carry0;
    }

    if (overflow || i[1] != 0) {
        Warning (&TokenCoord, "Integer literal is too big");
    }
    TokenValue.i[1] = 0;
    TokenValue.i[0] = i[0];
    tok = TK_INTCONST;

    /* 按照后缀设置类型码 */
    if ('u' == *CURRENT || 'U' == *CURRENT) {

        CURRENT++;
        if (TK_INTCONST == tok) tok = TK_UINTCONST;
        else if (TK_LLONGCONST == tok) tok = TK_ULLONGCONST;
    }
    if ('l' == *CURRENT || 'L' == *CURRENT) {

        CURRENT++;
        if (TK_INTCONST == tok) tok = TK_LONGCONST;
        else if (TK_UINTCONST == tok) tok = TK_ULONGCONST;
        if ('l' == *CURRENT || 'L' == *CURRENT) {

            CURRENT++;
            if (TK_LLONGCONST > tok) tok = TK_LLONGCONST;
        }
    }
    return tok;
}

/* 扫描常数序列 */
static int ScanNumericLiteral (void) 
{
    unsigned char *start = CURRENT;
    /* 表示数值是多少进制 */
    unsigned char base = 10;

    if ('.' == *CURRENT) {
        return ScanFloatLiteral (start);
    }

    /* 扫描前半部分（小数点的左边） */
    if ('0' == *CURRENT && ('x' == CURRENT[1] || 'X' == CURRENT[1])) {
    
        base = 16;
        start = (CURRENT += 2);
        if (!IsHexDigit (*CURRENT)) {
            
            Error (&TokenCoord, "Expect hex digit");
            TokenValue.i[0] = 0;
            return TK_INTCONST;
        }
        while (IsHexDigit (*CURRENT))
            CURRENT++;
    } else if ('0' == *CURRENT) {
    
        base = 8;
        while (IsOctDigit (*++CURRENT));
    } else {
        while (IsDigit (*++CURRENT));
    }

    /* 如果符合下边条件,按整形处理 */
    if (16 == base || ('.' != *CURRENT && 'e' != *CURRENT && 'E' != *CURRENT)) 
        return ScanIntLiteral (start, (int)(CURRENT - start), base);
    else return ScanFloatLiteral (start);
} 

/* 得到下一个词法单元 */
int GetNextToken (void) 
{
    /* 保存当前词法单元 */
    PrevCoord = TokenCoord;
    /* 滤过空白符和注释等 */
    SkipWhiteSpace ();
    /* 取得下一个词法单元 */
    TokenCoord.line = LINE;
    TokenCoord.col = (int)(CURRENT - LINEHEAD + 1);

    /* 处理下一个词法单元 */
    return (*Scanners[*CURRENT]) ();
}

/* 处理非法字符 */
static int ScanBadChar (void) 
{
    Error (&TokenCoord, "illegal character:\\x%x", *CURRENT);
    /* 下一个词法单元 */
    return GetNextToken ();
}

/* 文件扫描结束处理函数 */
static int ScanEOF (void) 
{
    Include p = POP_ITEM (InLink);

    if ( endifCnt > 0 )
        Error (&TokenCoord, "lack endif");

    if ( NULL == p ) {

        return TK_END;
    }

    if ( false == p->isDef )
        CloseSourceFile ();
    else 
        Input.base[Input.size] = 0;

    /* 处理完了include 文件 */
    Input = p->PreInput;
    TokenCoord = p->PreTokenCoord;
    endifCnt = p->endifCnt;

    if (Input.importRbrace) {
     
        if ('}' == *CURRENT) {

            CURRENT++;
            Input.importRbrace -= 1;
        } else
            ImportLoadFile ();
    }

    return GetNextToken ();
}

/* 处理++/+=/+ 操作 */
static int ScanPlus (void) 
{
    if ('+' == *++CURRENT) {

        CURRENT++;
        return TK_INC;
    } else if ('=' == *CURRENT) {

        CURRENT++;
        return TK_ADD_ASSIGN;
    } else return TK_ADD;
}

/* 处理--/-=/-/-> 操作 */
static int ScanMius (void) 
{
    if ('-' == *++CURRENT) {
        CURRENT++;
        return TK_DEC;
    } else if ('=' == *CURRENT) {
        CURRENT++;
        return TK_SUB_ASSIGN;
    } else if ('>' == *CURRENT) {
        CURRENT++;
        return TK_POINTER;
    } else return TK_ADD;
}
  
/* 处理*=,* 操作 */
static int ScanStar (void) 
{
    if ('=' == *++CURRENT) {
        CURRENT++;
        return TK_MUL_ASSIGN;
    } else return TK_MUL;
}

/* 处理/=,/ 操作 */
static int ScanSlash (void) 
{
    if ('=' == *++CURRENT) {
        CURRENT++;
        return TK_DIV_ASSIGN;
    } else return TK_DIV;
}

/* 处理%=,% 操作 */
static int ScanPercent (void) 
{
    if ('=' == *++CURRENT) {
        CURRENT++;
        return TK_MOD_ASSIGN;
    } else return TK_MOD;
}

/* 处理<<=/<</<=/< 操作 */
static int ScanLess (void) 
{
    if ('<' == *++CURRENT) {

        if ('=' == *++CURRENT) {

            CURRENT++;
            return TK_LSHIFT_ASSIGN;
        } return TK_LSHIFT;
    } else if ('=' == *CURRENT){

        CURRENT++;
        return TK_LESS_EQ;
    } else return TK_LESS;
}

/* 处理>>=/>>/>=/> 操作 */
static int ScanGreat (void) 
{
    if ('>' == *++CURRENT) {

        if ('=' == *++CURRENT) {
            
            CURRENT++;
            return TK_RSHIFT_ASSIGN;
        } return TK_RSHIFT;
    } else if ('=' == *CURRENT) {

        CURRENT++;
        return TK_GREAT_EQ;
    }else return TK_GREAT;
}

/* 处理!=/! 操作 */
static int ScanExclamation (void) 
{
    if ('=' == *++CURRENT) {
        CURRENT++;
        return TK_UNEQUAL;
    } else return TK_NOT;
}

/* 处理==/= 操作 */
static int ScanEqual (void) 
{
    if ('=' == *++CURRENT) {
        CURRENT++;
        return TK_EQUAL;
    } else return TK_ASSIGN;
}

/* 处理||/|=/| 操作 */
static int ScanBar(void)
{
	if ('|' == *++CURRENT) {
		CURRENT++;
		return TK_OR;
	} else if ('=' == *CURRENT) {
		CURRENT++;
		return TK_BITOR_ASSIGN;
	} else return TK_BITOR;
}

/* 处理&&/&=/& 操作 */
static int ScanAmpersand(void)
{
	if ('&' == *++CURRENT) {
		CURRENT++;
		return TK_AND;
	} else if ('=' == *CURRENT) {
		CURRENT++;
		return TK_BITAND_ASSIGN;
	} else return TK_BITAND;
}

/* 处理^=/^ 操作 */
static int ScanCaret(void)
{
	if ('=' == *++CURRENT) {
		CURRENT++;
		return TK_BITXOR_ASSIGN;
	} else return TK_BITXOR;
}

/* 处理浮点常量和.../. 操作 */
static int ScanDot(void)
{
	if (IsDigit(CURRENT[1])) {

		return ScanFloatLiteral(CURRENT);
	} else if (CURRENT[1] == '.' && CURRENT[2] == '.') {

		CURRENT += 3;
		return TK_ELLIPSE;
	} else {

		CURRENT++;
		return TK_DOT;
	}
}

static int ScanBacklash (void)
{
    if (CURRENT[1] == '\n') {

        TokenCoord.ppline++;
        LINE++;
        CURRENT  += 2;
        LINEHEAD = CURRENT;
    } else {
        
        Error (&TokenCoord, "\'\\\' error");
        CURRENT++;
    }

    return GetNextToken ();
}

/* 几个特殊符号处理函数类似,一并处理 */
#define SINGLE_CHAR_SCANNER(t)  \
    static int Scan ##t(void)   \
    {                           \
        CURRENT++;              \
        return TK_ ##t;         \
    }
SINGLE_CHAR_SCANNER (LBRACE)
SINGLE_CHAR_SCANNER (RBRACE)
SINGLE_CHAR_SCANNER (LBRACKET)
SINGLE_CHAR_SCANNER (RBRACKET)
SINGLE_CHAR_SCANNER (LPAREN)
SINGLE_CHAR_SCANNER (RPAREN)
SINGLE_CHAR_SCANNER (COMMA)
SINGLE_CHAR_SCANNER (SEMICOLON)
SINGLE_CHAR_SCANNER (COMP)
SINGLE_CHAR_SCANNER (QUESTION)
SINGLE_CHAR_SCANNER (COLON)

/* 初始化词法分析用到的数据 */
void SetupLexer (void) 
{
    int i;

    InLink = CreateVector (1);
    for (i = 0; i < END_OF_FILE + 1; i++) {
    
        if ( IsLetter (i) ) {

            /* 在字母开始的位置挂载标示符处理函数 */
            Scanners[i] = ScanIdentifier;
        } else if ( IsDigit (i) ) {

            /* 在数字或点开始的位置挂载常数处理函数 */
            Scanners[i] = ScanNumericLiteral;
        } else {

            /* 挂载处理处理非法字符的函数 */
            Scanners[i] = ScanBadChar;
        }
    }
    /* 下边覆盖上边部分挂在的错误处理函数 */
    /* 挂在文件扫描结束时的处理函数 */
    Scanners[END_OF_FILE] = ScanEOF;
    Scanners['\''] = ScanCharLiteral;
    Scanners['"'] = ScanStringLiteral;
    Scanners['+'] = ScanPlus;
    Scanners['-'] = ScanMius;
    Scanners['*'] = ScanStar;
    Scanners['/'] = ScanSlash;
    Scanners['%'] = ScanPercent;
    Scanners['<'] = ScanLess;
    Scanners['>'] = ScanGreat;
    Scanners['!'] = ScanExclamation;
    Scanners['='] = ScanEqual;
    Scanners['|'] = ScanBar;
    Scanners['&'] = ScanAmpersand;
    Scanners['^'] = ScanCaret;
    Scanners['.'] = ScanDot;
    Scanners['{'] = ScanLBRACE;
    Scanners['}'] = ScanRBRACE;
    Scanners['['] = ScanLBRACKET;
    Scanners[']'] = ScanRBRACKET;
    Scanners['('] = ScanLPAREN;
    Scanners[')'] = ScanRPAREN;
    Scanners[','] = ScanCOMMA;
    Scanners[';'] = ScanSEMICOLON;
    Scanners['~'] = ScanCOMP;
    Scanners['?'] = ScanQUESTION;
    Scanners[':'] = ScanCOLON;
    Scanners['\\'] = ScanBacklash;

    /* 外部关键字关键字 */
    if (ExtraKeywords) {

        char *str;
        struct keyword *p;
        FOR_EACH_ITEM (char*, str, ExtraKeywords) 
            for (p = keywords_; p->name; p++) {
            
                if (!strcmp (str, p->name)) {

                    p->len = strlen (str);
                    break;
                }
            }
        ENDFOR
    }
}

/* 保存当前扫描的词法单元 */
void BeginPeekToken (void)
{
    PeekPoint = CURRENT;
    PeekValue = TokenValue;
    PeekCoord = TokenCoord;
}

/* 恢复之前的位置 */
void EndPeekToken (void) 
{
    CURRENT = PeekPoint;
    TokenValue = PeekValue;
    TokenCoord = PeekCoord;
}


void LexTest (char *filename)
{
    ReadSourceFile (filename);

    PRINT ("LexTest start... \n");
    SetupLexer ();
    int tok = 0;

    SkipWhiteSpace ();
    while (1) {

        tok = Scanners[*CURRENT]();
        if (TK_END == tok) break;
        
            PRINT ("%-8s|(%d, %d) ", TokenString[tok], 
            TokenCoord.line, TokenCoord.col);
            if (TK_ID == tok) 
                PRINT ("%s", (char*)TokenValue.p);
            PRINT ("\n");
    }

    PRINT ("词法分析结束\n");
    PRINT ("LexTest end... \n");

    CloseSourceFile ();
}
