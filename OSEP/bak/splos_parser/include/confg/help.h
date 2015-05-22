/*************************************************************************
	> File Name: help.h
	> Author: 
	> Mail: 
	> Created Time: 2015年05月17日 星期日 14时54分13秒
 ************************************************************************/

#ifdef HELP 
HELP ("splos_parser 用法:")
HELP ("  --dump-ast 打印语法树")
HELP ("  --test-lex 测试词法分析")
HELP ("  -I         设置加载引入文件时扫描的路径")
HELP ("             用法: -I./test/test.h")
HELP ("  -VARINFO   打印日志, 日志最高级别7级")
HELP ("             1-4, 7 暂时没用")
HELP ("             4. 打印加载的文件")
HELP ("             5. 打印define 定义")
HELP ("             6. 打印使用变量的类型")
HELP ("  --help     显示此帮助")
HELP ("  --version")
#endif
