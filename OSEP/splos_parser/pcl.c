/*************************************************************************
	> File Name: ucl.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月14日 星期五 16时17分24秒

	> Description: 
 ************************************************************************/

#include "pcl.h"
#include "ast.h"

#define VERSION "1.0.0"

/* 标志是否要生成语法树 */
static int DumpAST;
/* 标志是否要生成中间代码 */
static int DumpIR;


/* 保存语法树的文件 */
FILE *ASTFile;
/* 中间代码文件 */
FILE *IRFile;
/* 生成的asm文件 */
FILE *ASMFile;

/* 汇编文件的扩展名 */
char *ExtName = ".s";

/* ALLOC 默认在CurrentHeap 中分配内存 */
Heap CurrentHeap;
/* 为文件创建的堆 */
HEAP (FileHeap);
/* 所有字符串逗存放在次堆中 */
HEAP (StringHeap);
/* 进程一直都存在的堆 */
HEAP (ProgramHeap);
/* 文件中的警告,错误数 */
int WarningCount;
int ErrorCount;
/* 存储用户自定义空白符的集合 */
Vector ExtraWhiteSpace;
/* 存储用户自定义关键字的集合 */
Vector ExtraKeywords;

/* 测试词法分析标志位 */
unsigned char test_lex = 0;

/* 用户自定义关键字 */
#define AddKeyword(str) \
            AddItemToVector (&ExtraKeywords, str) 
/* 用户自定义空白符 */
#define AddWhiteSpace(str) \
            AddItemToVector (&ExtraWhiteSpace, str)


/* 初始化 */
static void Initialize (void) 
{
    /* 用文件的堆初始化当前堆 */
    CurrentHeap = &FileHeap;
    /* 初始化警告,错误数目 */
    WarningCount = ErrorCount = 0;
    /* 初始化符号表 */
    InitSymbolTable ();
    /* 几种文件描述符 */
    ASTFile = IRFile = ASMFile = NULL;
}

/* 结束 */
static void Finalize (void) 
{
    /* 释放文件堆 */
    FreeHeap (&FileHeap);
}

/* 编译 */
static void Compile (char *file) 
{
    AstTranslationUnit transUnit;

    Initialize ();

    if ( test_lex ) {

        LexTest (file);
        goto exit;
    }

    /* 处理C文件,产生语法树所需的节点 */
    transUnit = ParseTranslationUnit (file);

    /* 检查节点的语义 */
    CheckTranslationUnit (transUnit);

    if ( ErrorCount ) 
        goto exit;

    if ( DumpAST ) {
        /* 生成语法树 */
        DumpTranslationUnit (transUnit);
    }

#if 0
    /* 语句翻译 */
    Translate (transUnit);

    if (DumpIR) {
        
        /* 中间代码输出 */
        DAssemTranslationUnit (transUnit);
    }

    /* 目标代码生成 */
    //EmitTranslationUnit (transUnit);

#endif

exit:
    Finalize ();
}

/* 添加item 到v集合中 
 * str: 串如　abc,bcd,cde 用逗号分隔 */
static void AddItemToVector (Vector *v, char *str)
{
    char *p = str, *q;

    /* 如果data 为空则初始化 */
    if (NULL == *v)
        *v = CreateVector (1);

    /* 将元素添加到v 集合中 */
    while ((q = strchr (p, ','))) {
    
        *q = 0;
        INSERT_ITEM (*v, p);
        p = q + 1;
    }
    INSERT_ITEM (*v, p);
}

/* 在参数输入时,以下命令必须在最前边,源文件名在后边 */
static int ParseCommandLine (int argc, char *argv[])
{
    int i;
    
    UserIncPath = CreateVector (1);
    for (i = 0; i < argc; i++) {

        if (!strncmp (argv[i], "-ext:", 5)) {
            /* 设置汇编文件的扩展名 */
            ExtName = argv[i] + 5;
        } else if (!strcmp (argv[i], "-ignore")) {
            /* 添加要当成空白符处理的字符串 */
            AddWhiteSpace (argv[++i]);
        } else if (!strcmp (argv[i], "-keyword")) {
            /* 添加用户自定义关键字 */
            AddKeyword (argv[++i]);
        } else if (!strcmp (argv[i], "--dump-ast")) {
            /* 标志生成语法树 */
            DumpAST = 1;
        } else if (!strcmp (argv[i], "--dump-IR")) {
            /* 标志是否要生成中间代码 */
            DumpIR = 1;
        } else if (!strcmp (argv[i], "--test-lex")) {
            
            test_lex = 1;
        } else if (!strncmp (argv[i], "-I", 2)) {
            
            char *path;
            
            path = calloc (strlen (argv[i]+2) + 1, sizeof (char));
            strcpy (path, argv[i] + 2);
            INSERT_ITEM (UserIncPath, path);
        } else if (!strncmp (argv[i], "-VARINFO=", 9)) {
            
            VarInfo = atoi (argv[i] + 9);
            if (VarInfo > 7 || VarInfo < 0) 
                VarInfo = 0;
        } else if (!strcmp (argv[i], "--help")) { 
        
            #define HELP(help) printf ("%s\n", help);
            #include "include/confg/help.h"
            #undef  HELP
        } else if (!strcmp (argv[i], "--version")) {
            
            printf ("%s \n", VERSION);
        } else return i;
    } return i;
}

#include "../fs/init.h"

/* 编译从这开始 */
int main (int argc, char **argv)
{
    int i;

    CurrentHeap = &ProgramHeap;
    argc--; argv++;

#if defined(_LF)
    init_file_layout ();
    init_memory_layout ();
#endif

    CreateLogFile ();

#if 1
    i = ParseCommandLine (argc, argv);
    
    /* 初始化词法分析器 */
    SetupLexer ();
    /* 按照配置文件设置基本类型和默认函数的属性 */
    SetupTypeSystem ();

    for ( ; i < argc; i++ ) {
    
        Compile (argv[i]);
    }
#endif
    CloseLogFile ();
    return LogFileNo;
    //return (ErrorCount != 0);
}
