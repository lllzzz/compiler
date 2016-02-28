#include<stdio.h>
#include<stdlib.h>
#include<memory.h>
#include<string.h>

int token; // 当前token
char * src, * old_src; // 指向代码String的指针
int poolsize; 
int line;

int * text, // 代码段
    * old_text, 
    * stack; // 栈
char * data; // 数据段

void next();
void expression(int);
void program();
int eval();

int main(int argc, char **argv)
{
    int i, fd;
    argc--;
    argv++;
    poolsize = 256 * 1024;
    line = 1;

    if ((fd = open(*argv, 0)) < 0) {
        printf("could not open (%s)\n", *argv);
        return -1;
    }

    if (!(src = old_src = malloc(poolsize))) {
        printf("could not malloc (%d) for source area\n", poolsize);
        return -1;
    } 

    if ((i = read(fd, src, poolsize - 1)) <= 0) {
        printf("read() returned %d\n", i);
        return -1;
    }
    src[i] = 0;
    close(fd);

    // 为虚拟机分配内存
    if (!(text = old_text = malloc(poolsize))) {
        printf("could not malloc(%d) for text area\n", poolsize);
        return -1;
    }

    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%d) for data area\n", poolsize);
        return -1;
    }

    if (!(stack = malloc(poolsize))) {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }

    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);
    // 分配内存结束

    program();
    return eval();    
}

/**
 * 用户词法分析，获取下一个标记
 */
void next() 
{
    token = *src ++;
    return;
}

/**
 * 用于解析表达式
 * @param level 
 */
void expression(int level)
{

}

/**
 * 语法分析入口
 */
void program() 
{
    next();
    while (token > 0) {
        printf("token is: %c\n", token);
        next();
    }
}

/**
 * 虚拟机入口，用于解释目标代码
 * @return [description]
 */
int eval()
{
    return 0; 
}