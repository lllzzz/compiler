#include<stdio.h>
#include<stdlib.h>
#include<memory.h>
#include<string.h>

#define DEBUG

int token; // 当前token
char * src, * old_src; // 指向代码String的指针
int poolsize;
int line;

int * text, // 代码段
    * old_text,
    * stack; // 栈
char * data; // 数据段

// 寄存器相关变量
int * pc, // 程序计数器，该地址存放下一条要执行的计算机指令
    * bp, // 指针寄存器，指向栈顶
    * sp, // 基址指针
    ax,   // 通用寄存器，存放一条指令执行后的结果
    cycle;

// 指令集
enum {
    IMM, LI, LC, SI, SC, // 0 1 2 3 4
    PUSH, // 5
    JMP, // 6
    JZ, JNZ, // 7 8
    CALL, // 9
    ENT, // 10
    ADJ, // 11
    LEV, // 12
    LEA, // 13
    OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD, // 14 ~ 29
    OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT // 30 ~
};

// Token
enum {
    Num = 128, Fun, Sys, Glo, Loc, Id, // 128 ~ 133
    Char, Else, Enum, If, Int, Return, Sizeof, While, // 134 ~ 141
    Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

int token_val; // 数字，字符串的值
int * current_id, // 当前标示
    * symbols; // 保存全部标示结构（变量、关键字）

enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};  // 标志的字段

enum { CHAR, INT, PTR }; // 支持的类型
int *idmain; // main入口



void debug()
{
#ifndef DEBUG
    return;
#endif
    puts(">>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    // printf("ac: %d, %p\n", ax, ax);
    // printf("pc: %d, %p\n", *pc, *pc);
    // int * s = sp;
    // printf("%d, %p\n", *s, *s);
    // puts("stack:");
    // while (*s) {
    //     printf("\t%d, %p\n", *s, *s);
    //     s++;
    // }
    puts("current_id:");
    int i = 0;
    char * title[] = {"Token", "Hash", "Name", "Type", "Class", "Value", "BType", "BClass", "BValue"};
    for (i = 0; i < IdSize; i++) {
        printf("%s:\t %d", title[i], current_id[i]);
        if (i == 2) printf(", %s", current_id[i]);
        printf("\n");
    }
}


/**
 * 用户词法分析，获取下一个标记
 */
void next()
{
    char * last_pos;
    int hash;

    while (token = *src) {
        ++src;

        // 处理换行
        if (token == "\n") {
            ++line;
        }
        // 处理宏，直接略过
        else if (token == '#') {
            while ( *src != 0 && *src != '\n') {
                src++;
            }
        }
        // 处理变量
        else if ((token >= 'a' && token <= 'z') ||
                 (token >= 'A' && token <= 'Z') ||
                 (token == '_'))
        {
            last_pos = src - 1;
            hash = token;

            while ((*src >= 'a' && *src <= 'z') ||
                   (*src >= 'A' && *src <= 'Z') ||
                   (*src >= '0' && *src <= '9') ||
                   (*src == '_'))
            {
                hash = hash * 147 + *src;
                src++;
            }

            current_id = symbols;
            while (current_id[Token]) {
                if (current_id[Hash] == hash && !memcmp((char *)current_id[Name], last_pos, src - last_pos)) {
                    token = current_id[Token];
                    return;
                }
                current_id = current_id + IdSize;
            }

            current_id[Name] = (int)last_pos;
            current_id[Hash] = hash;
            token = current_id[Token] = Id;
            return;
        }
        // 处理数字
        else if (token >= '0' && token <= '9') {
            token_val = token - '0';
            if (token_val > 0) {
                while (*src >= '0' && *src <= '9') {
                    token_val = token_val * 10 + *src++ - '0';
                }
            } else {
                if (*src == 'x' || *src == 'X') {
                    token = *++src;
                    while ((token >= '0' && token <= '9') ||
                           (token >= 'a' && token <= 'f') ||
                           (token >= 'A' && token <= 'F'))
                    {
                        token_val = token_val * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
                        token = *++src;
                    }
                } else {
                    while (*src >= '0' && *src <= '7') {
                        token_val = token_val * 8 + *src++ - '0';
                    }
                }
            }
            token = Num;
            return;
        }
        // 处理字符串
        else if (token == '"' || token == '\'') {
            last_pos = data;
            while (*src != 0 && *src != token) {
                token_val = *src++;
                if (token_val == '\\') {
                    token_val = *src++;
                    if (token_val == 'n') {
                        token_val = '\n';
                    }
                }

                if (token == '"') {
                    *data++ = token_val;
                }
            }

            src++;
            if (token == '"') {
                token_val = (int)last_pos;
            } else {
                token = Num;
            }

            return;
        }
        // 处理注释
        else if (token == '/') {
            if (*src == '/') {
                while (*src != 0 && *src != '\n') {
                    ++src;
                }
            } else {
                token = Div;
                return;
            }
        }
        // 其他字符
        else if (token == '=') {
            // parse '==' and '='
            if (*src == '=') {
                src ++;
                token = Eq;
            } else {
                token = Assign;
            }
            return;
        }
        else if (token == '+') {
            // parse '+' and '++'
            if (*src == '+') {
                src ++;
                token = Inc;
            } else {
                token = Add;
            }
            return;
        }
        else if (token == '-') {
            // parse '-' and '--'
            if (*src == '-') {
                src ++;
                token = Dec;
            } else {
                token = Sub;
            }
            return;
        }
        else if (token == '!') {
            // parse '!='
            if (*src == '=') {
                src++;
                token = Ne;
            }
            return;
        }
        else if (token == '<') {
            // parse '<=', '<<' or '<'
            if (*src == '=') {
                src ++;
                token = Le;
            } else if (*src == '<') {
                src ++;
                token = Shl;
            } else {
                token = Lt;
            }
            return;
        }
        else if (token == '>') {
            // parse '>=', '>>' or '>'
            if (*src == '=') {
                src ++;
                token = Ge;
            } else if (*src == '>') {
                src ++;
                token = Shr;
            } else {
                token = Gt;
            }
            return;
        }
        else if (token == '|') {
            // parse '|' or '||'
            if (*src == '|') {
                src ++;
                token = Lor;
            } else {
                token = Or;
            }
            return;
        }
        else if (token == '&') {
            // parse '&' and '&&'
            if (*src == '&') {
                src ++;
                token = Lan;
            } else {
                token = And;
            }
            return;
        }
        else if (token == '^') {
            token = Xor;
            return;
        }
        else if (token == '%') {
            token = Mod;
            return;
        }
        else if (token == '*') {
            token = Mul;
            return;
        }
        else if (token == '[') {
            token = Brak;
            return;
        }
        else if (token == '?') {
            token = Cond;
            return;
        }
        else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') {
            // directly return the character as token;
            return;
        }
    }
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
        global_declaration();
    }
}

void match(int tk) {
    if (token == tk) {
        next();
    } else {
        printf("%d: expected token: %d\n", line, tk);
        exit(-1);
    }
}

void enum_declaration()
{
    int i;
    i = 0;
    while (token != '}') {
        if (token != Id) {
            printf("Line %d: bad enum identifier %d\n", line, token);
            exit(-1);
        }
        next();
        if (token == Assign) {
            next();
            if (token != Num) {
                printf("Line %d: bad enum initializer\n", line);
                exit(-1);
            }
            i = token_val;
            next();
        }

        current_id[Class] = Num;
        current_id[Type] = INT;
        current_id[Value] = i++;

        if (token == ',') {
            next();
        }
    }
}

int index_of_bp;

void function_parameter()
{
    int type;
    int params;
    params = 0;
    while (token != ')') {
        type = INT;
        if (token == Int) {
            match(Int);
        } else if (token == Char) {
            type = CHAR;
            match(Char);
        }

        while (token == Mul) {
            match(Mul);
            type = type + PTR;
        }

        if (token != Id) {
            printf("Line %d: bad parameter declaration\n", line);
            exit(-1);
        }

        if (current_id[Class] == Loc) {
            printf("Line %d: duplicate parameter declaration\n", line);
            exit(-1);
        }
        match(Id);

        current_id[BClass] = current_id[Class];
        current_id[Class] = Loc;
        current_id[BType] = current_id[Type];
        current_id[Type] = type;
        current_id[BValue] = current_id[Value];
        current_id[Value] = params;

        if (token == ',') {
            match(',');
        }
    }
    index_of_bp = params + 1;
}

void function_body()
{
    int pos_local;
    int type;
    pos_local = index_of_bp;

    while (token == Int || token == Char) {
        basetype = (token == Int) ? INT : CHAR;
        match(token);

        while (token != ';') {
            type = basetype;
            while (token == Mul) {
                match(Mul);
                type = type + PTR;
            }

            if (token != Id) {
                printf("Line %d: bad local declaration\n", line);
                exit(-1);
            }

            if (current_id[Class] == Loc) {
                printf("Line %d: duplicate local declaration\n", line);
                exit(-1);
            }
            match(Id);

            current_id[BClass] = current_id[Class];
            current_id[Class] = Loc;
            current_id[BType] = current_id[Type];
            current_id[Type] = type;
            current_id[BValue] = current_id[Value];
            current_id[Value] = ++pos_local;

            if (token == ',') {
                match(',');
            }
        }
        match(';');
    }

    *++text = ENT;
    *++text = pos_local - index_of_bp;

    while (token != '}') {
        statement();
    }

    *++text = LEV;
}

void function_declaration()
{
    match('(');
    function_parameter();
    match(')');
    match('{');
    function_body();

    current_id = symbols;
    while (current_id[Token]) {
        if (current_id[Class] == Loc) {
            current_id[Class] = current_id[BClass];
            current_id[Type] = current_id[BType];
            current_id[Value] = current_id[BValue];
        }
        current_id = current_id + IdSize;
    }
}

int basetype;
int expr_type;

void global_declaration()
{
    int type;
    int i;

    basetype = INT;

    if (token == Enum) {
        match(Enum);
        if (token != '{') {
            match(Id);
        }
        if (token == '{') {
            match('{');
            enum_declaration();
            match('}');
        }
        match(';');
        return;
    }
    if (token == Int) {
        match(Int);
    }
    else if (token == Char) {
        match(Char);
        basetype = CHAR;
    }

    while (token != ';' && token != '}') {
        type = basetype;
        while (token == Mul) {
            match(Mul);
            type = type + PTR;
        }
        if (token != Id) {
            printf("Line %d: bad global declaration \n", line);
            exit(-1);
        }
        if (current_id[Class]) {
            printf("Line %d: duplicate global declaration\n", line);
            exit(-1);
        }
        match(Id);
        current_id[Type] = type;

        if (token == '(') {
            current_id[Class] = Fun;
            current_id[Value] = (int)(text + 1);
            function_declaration();
        } else {
            current_id[Class] = Glo;
            current_id[Value] = (int)data;
            data = data + sizeof(int);
        }

        if (token == ',') { match(','); }

    }
    next();
}

/**
 * 虚拟机入口，用于解释目标代码
 * @return [description]
 */
int eval()
{
    int op, * tmp;
    while (1) {
        op = *pc++;
        debug();
        if (op == IMM)          { ax = *pc++;}                          // load immediate value to ax
        else if (op == LC)      { ax = *(char *)ax; }                    // load character to ax, address in ax
        else if (op == LI)      { ax = *(int *)ax; }                     // load integer to ax, address in ax
        else if (op == SC)      { ax = *(char *)*sp++ = ax; }            // save character to address, value in ax, address on stack
        else if (op == SI)      { *(int *)*sp++ = ax; }                  // save integer to address, value in ax, address on stack
        else if (op == PUSH)    { *--sp = ax; }                          // push the value of ax onto the stack
        else if (op == JMP)     { pc = (int *)*pc; }                     // jump to the address
        else if (op == JZ)      { pc = ax ? pc + 1 : (int *)*pc; }       // jump if ax is zero
        else if (op == JNZ)     { pc = ax ? (int *)*pc : pc + 1; }       // jump if ax is not zero
        else if (op == CALL)    { *--sp = (int)(pc + 1); pc = (int *)*pc; }     // call subroutine
        else if (op == ENT)     { *--sp = (int)bp; bp = sp; sp = sp - *pc++; }  // make new stack frame
        else if (op == ADJ)     { sp = sp + *pc++; }
        else if (op == LEV)     { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; }
        else if (op == LEA)     { ax = (int)(bp + *pc++); }

        else if (op == OR)  ax = *sp++ | ax;
        else if (op == XOR) ax = *sp++ ^ ax;
        else if (op == AND) ax = *sp++ & ax;
        else if (op == EQ)  ax = *sp++ == ax;
        else if (op == NE)  ax = *sp++ != ax;
        else if (op == LT)  ax = *sp++ < ax;
        else if (op == LE)  ax = *sp++ <= ax;
        else if (op == GT)  ax = *sp++ >  ax;
        else if (op == GE)  ax = *sp++ >= ax;
        else if (op == SHL) ax = *sp++ << ax;
        else if (op == SHR) ax = *sp++ >> ax;
        else if (op == ADD) ax = *sp++ + ax;
        else if (op == SUB) ax = *sp++ - ax;
        else if (op == MUL) ax = *sp++ * ax;
        else if (op == DIV) ax = *sp++ / ax;
        else if (op == MOD) ax = *sp++ % ax;

        else if (op == EXIT) { printf("exit(%d)", *sp); return *sp;}
        else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
        else if (op == CLOS) { ax = close(*sp);}
        else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
        else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
        else if (op == MALC) { ax = (int)malloc(*sp);}
        else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp);}
        else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp);}

        else {
            printf("unknown instruction:%d\n", op);
            return -1;
        }

    }
    return 0;
}

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

    if (!(symbols = malloc(poolsize))) {
        printf("could not malloc(%d) for symbol table\n", poolsize);
        return -1;
    }

    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);
    memset(symbols, 0, poolsize);
    // 分配内存结束

    // 初始化寄存器
    // bp = sp = (int *)((int)stack + poolsize);
    bp = sp = stack + poolsize;
    ax = 0;
    // 初始化寄存器结束

    src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit void main";
    i = Char;
    while (i <= While) {
        next();
        current_id[Token] = i++;
    }

    i = OPEN;
    while (i <= EXIT) {
        next();
        current_id[Class] = Sys;
        current_id[Type] = INT;
        current_id[Value] = i++;
        debug();
    }

    next(); current_id[Token] = Char;
    next(); idmain = current_id;

    program();
    return eval();
}
