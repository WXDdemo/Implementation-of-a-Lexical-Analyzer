// Lexical_Analysis.cpp : 完全修正版 - 正确跳过预处理指令，避免误识标识符
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "iostream"
using namespace std;

// 全局变量，保留字表
static char reserveWord[32][20] = {
    "auto", "break", "case", "char", "const", "continue",
    "default", "do", "double", "else", "enum", "extern",
    "float", "for", "goto", "if", "int", "long",
    "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while"
};

// 界符运算符表
static char operatorOrDelimiter[36][10] = {
    "+", "-", "*", "/", "<", "<=", ">", ">=", "=", "==",
    "!=", ";", "(", ")", "^", ",", "\"", "\'", "#", "&",
    "&&", "|", "||", "%", "~", "<<", ">>", "[", "]", "{",
    "}", "\\", ".", "\?", ":", "!"
};

static char IDentifierTbl[1000][50] = { "" };

/********查找是否是保留字*****************/
int searchReserve(char reserveWord[][20], char s[])
{
    for(int i = 0; i < 32; i++) {
        if(strcmp(reserveWord[i], s) == 0) {
            return i + 1;
        }
    }
    return -1;
}

/*********************判断是否为字母********************/
bool IsLetter(char letter)
{
    if ( (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z') || letter=='_' )
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*****************判断是否为数字************************/
bool IsDigit(char digit)
{
    if (digit >= '0' && digit <= '9')
    {
        return true;
    }
    else
    {
        return false;
    }
}

/********************编译预处理，取出无用的字符和注释**********************/
void filterResource(char r[], int pProject)
{
    char tempString[10000];
    int count = 0;
    for (int i = 0; i <= pProject && r[i] != '\0'; i++)
    {
        if(r[i] == '\t' || r[i] == '\r') {
            continue;
        }
        
        // 处理单行注释 //
        if(r[i] == '/' && r[i+1] == '/') {
            i += 2;
            while(i <= pProject && r[i] != '\n' && r[i] != '\0') {
                i++;
            }
            if(i <= pProject && r[i] == '\n') {
                tempString[count++] = r[i];
            }
            continue;
        }
        
        // 处理多行注释 /* */
        if(r[i] == '/' && r[i+1] == '*') {
            i += 2;
            while(i <= pProject && r[i] != '\0') {
                if(r[i] == '*' && r[i+1] == '/') {
                    i += 2;
                    break;
                }
                i++;
            }
            continue;
        }
        
        tempString[count++] = r[i];
    }
    tempString[count] = '\0';
    strcpy(r, tempString);
}

/*
  GetToken 修改说明：
  - 预处理指令 #... 整行跳过，不解析内部
  - 错误标志: syn == -1 非法字符，syn == -3 非法数字常量
*/
void GetToken(int &syn, char resourceProject[], char token[], int &pProject)
{
    int i, count = 0;
    char ch = resourceProject[pProject];
    
    // 跳过空格和换行
    while (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r') {
        pProject++;
        ch = resourceProject[pProject];
    }
    
    // 清空token
    for (i = 0; i < 200; i++) {
        token[i] = '\0';
    }
    
    // 文件结束
    if (ch == '\0') {
        syn = 0;
        return;
    }
    
    // 预处理指令：# 开头，整行跳过（推荐做法）
    if (ch == '#') {
        token[count++] = resourceProject[pProject++]; // #
        while (resourceProject[pProject] != '\0' && resourceProject[pProject] != '\n') {
            token[count++] = resourceProject[pProject++]; // 收集整行用于输出
        }
        token[count] = '\0';
        syn = 200; // 预处理指令
        if (resourceProject[pProject] == '\n') {
            pProject++; // 跳到下一行
        }
        return;
    }
    
    // 字符串常量
    if (ch == '\"') {
        token[count++] = resourceProject[pProject++]; // "
        while(resourceProject[pProject] != '\0' && resourceProject[pProject] != '\"') {
            if (resourceProject[pProject] == '\\' && resourceProject[pProject+1] != '\0') {
                token[count++] = resourceProject[pProject++]; // \
                token[count++] = resourceProject[pProject++]; // escaped
                continue;
            }
            token[count++] = resourceProject[pProject++];
        }
        if(resourceProject[pProject] == '\"') {
            token[count++] = resourceProject[pProject++]; // closing "
        }
        token[count] = '\0';
        syn = 102;
        return;
    }
    
    // 字符常量
    if (ch == '\'') {
        token[count++] = resourceProject[pProject++]; // '
        if (resourceProject[pProject] == '\\' && resourceProject[pProject+1] != '\0') {
            token[count++] = resourceProject[pProject++]; // \
            token[count++] = resourceProject[pProject++]; // escaped
        } else if (resourceProject[pProject] != '\0' && resourceProject[pProject] != '\'') {
            token[count++] = resourceProject[pProject++]; // char
        }
        if (resourceProject[pProject] == '\'') {
            token[count++] = resourceProject[pProject++]; // '
        }
        token[count] = '\0';
        syn = 105;
        return;
    }
    
    // 标识符或关键字
    if (IsLetter(resourceProject[pProject]))
    {	
        while(IsLetter(resourceProject[pProject]) || IsDigit(resourceProject[pProject])) {
            token[count++] = resourceProject[pProject++];
        }
        token[count] = '\0';
        
        syn = searchReserve(reserveWord, token);
        if (syn == -1) {
            syn = 100;
        }
        return;
    }
    
    // 数字常量（支持十六进制、浮点、非法后缀检测）
    if (IsDigit(resourceProject[pProject]))
    {
        int start = pProject;
        
        // 十六进制: 0x / 0X
        if (resourceProject[pProject] == '0' && (resourceProject[pProject+1] == 'x' || resourceProject[pProject+1] == 'X')) {
            token[count++] = resourceProject[pProject++]; // '0'
            token[count++] = resourceProject[pProject++]; // 'x'/'X'
            int hexDigits = 0;
            while ( (resourceProject[pProject] >= '0' && resourceProject[pProject] <= '9') ||
                    (resourceProject[pProject] >= 'a' && resourceProject[pProject] <= 'f') ||
                    (resourceProject[pProject] >= 'A' && resourceProject[pProject] <= 'F') ) {
                token[count++] = resourceProject[pProject++];
                hexDigits++;
            }
            if (hexDigits == 0 || (IsLetter(resourceProject[pProject]) || IsDigit(resourceProject[pProject]))) {
                while (IsLetter(resourceProject[pProject]) || IsDigit(resourceProject[pProject])) {
                    token[count++] = resourceProject[pProject++];
                }
                token[count] = '\0';
                syn = -3;
                return;
            }
            token[count] = '\0';
            syn = 104;
            return;
        }
        
        // 以0开头：八进制（但含8/9非法）
        if (resourceProject[pProject] == '0') {
            token[count++] = resourceProject[pProject++];
            bool has8or9 = false;
            while (IsDigit(resourceProject[pProject])) {
                if (resourceProject[pProject] == '8' || resourceProject[pProject] == '9') has8or9 = true;
                token[count++] = resourceProject[pProject++];
            }
            if (IsLetter(resourceProject[pProject])) {
                while (IsLetter(resourceProject[pProject]) || IsDigit(resourceProject[pProject])) {
                    token[count++] = resourceProject[pProject++];
                }
                token[count] = '\0';
                syn = -3;
                return;
            }
            if (has8or9) {
                token[count] = '\0';
                syn = -3;
                return;
            }
            token[count] = '\0';
            syn = 103;
            return;
        }
        
        // 十进制整数或浮点
        while (IsDigit(resourceProject[pProject])) {
            token[count++] = resourceProject[pProject++];
        }
        if (IsLetter(resourceProject[pProject])) {
            while (IsLetter(resourceProject[pProject]) || IsDigit(resourceProject[pProject])) {
                token[count++] = resourceProject[pProject++];
            }
            token[count] = '\0';
            syn = -3;
            return;
        }
        if (resourceProject[pProject] == '.') {
            token[count++] = resourceProject[pProject++];
            while (IsDigit(resourceProject[pProject])) {
                token[count++] = resourceProject[pProject++];
            }
            if (resourceProject[pProject] == 'e' || resourceProject[pProject] == 'E') {
                token[count++] = resourceProject[pProject++];
                if (resourceProject[pProject] == '+' || resourceProject[pProject] == '-') {
                    token[count++] = resourceProject[pProject++];
                }
                if (!IsDigit(resourceProject[pProject])) {
                    while (IsLetter(resourceProject[pProject]) || IsDigit(resourceProject[pProject])) {
                        token[count++] = resourceProject[pProject++];
                    }
                    token[count] = '\0';
                    syn = -3;
                    return;
                }
                while (IsDigit(resourceProject[pProject])) {
                    token[count++] = resourceProject[pProject++];
                }
            }
            token[count] = '\0';
            syn = 101;
            return;
        }
        token[count] = '\0';
        syn = 99;
        return;
    }
    
    // 单个字符运算符
    if (strchr("+-*/;()^,~%[]{} \\.?:", ch)) {
        token[0] = resourceProject[pProject++];
        token[1] = '\0';
        for (i = 0; i < 36; i++) {
            if (strcmp(token, operatorOrDelimiter[i]) == 0) {
                syn = 33 + i;
                break;
            }
        }
        if (i < 36) return;
    }
    
    // 复合运算符
    if (resourceProject[pProject] == '<') {
        pProject++;
        if (resourceProject[pProject] == '=') { token[0]='<'; token[1]='='; token[2]='\0'; pProject++; syn=38; }
        else if (resourceProject[pProject] == '<') { token[0]='<'; token[1]='<'; token[2]='\0'; pProject++; syn=58; }
        else { token[0]='<'; token[1]='\0'; syn=37; }
        return;
    }
    if (resourceProject[pProject] == '>') {
        pProject++;
        if (resourceProject[pProject] == '=') { token[0]='>'; token[1]='='; token[2]='\0'; pProject++; syn=40; }
        else if (resourceProject[pProject] == '>') { token[0]='>'; token[1]='>'; token[2]='\0'; pProject++; syn=59; }
        else { token[0]='>'; token[1]='\0'; syn=39; }
        return;
    }
    if (resourceProject[pProject] == '=') {
        pProject++;
        if (resourceProject[pProject] == '=') { token[0]='='; token[1]='='; token[2]='\0'; pProject++; syn=42; }
        else { token[0]='='; token[1]='\0'; syn=41; }
        return;
    }
    if (resourceProject[pProject] == '!') {
        pProject++;
        if (resourceProject[pProject] == '=') { token[0]='!'; token[1]='='; token[2]='\0'; pProject++; syn=43; }
        else { token[0]='!'; token[1]='\0'; syn=68; }
        return;
    }
    if (resourceProject[pProject] == '&') {
        pProject++;
        if (resourceProject[pProject] == '&') { token[0]='&'; token[1]='&'; token[2]='\0'; pProject++; syn=53; }
        else { token[0]='&'; token[1]='\0'; syn=52; }
        return;
    }
    if (resourceProject[pProject] == '|') {
        pProject++;
        if (resourceProject[pProject] == '|') { token[0]='|'; token[1]='|'; token[2]='\0'; pProject++; syn=55; }
        else { token[0]='|'; token[1]='\0'; syn=54; }
        return;
    }
    
    // 非法字符
    printf("错误字符: %c\n", ch);
    token[0] = ch; token[1] = '\0';
    pProject++;
    syn = -1;
}

int main()
{
    char resourceProject[10000];
    char token[200] = { 0 };
    int syn = -1, i;
    int pProject = 0;
    int identifierCount = 0;
    FILE *fp, *fp1;
    
    if ((fp = fopen("testHard.c", "r")) == NULL) {
        cout << "无法打开源文件 testHard.c" << endl;
        return -1;
    }
    
    // 读取文件
    pProject = 0;
    int ch;
    while ((ch = fgetc(fp)) != EOF && pProject < 9998) {
        resourceProject[pProject++] = (char)ch;
    }
    resourceProject[pProject] = '\0';
    fclose(fp);
    
    cout << "源程序为:" << endl;
    cout << resourceProject << endl;
    
    // 过滤注释
    filterResource(resourceProject, pProject);
    cout << endl << "过滤之后的程序:" << endl;
    cout << resourceProject << endl;
    
    pProject = 0;
    
    if ((fp1 = fopen("lex_out.txt", "w")) == NULL) {
        cout << "无法创建输出文件" << endl;
        return -1;
    }
    
    fprintf(fp1, "词法分析结果:\n");
    
    // 词法分析主循环
    while (true) {
        GetToken(syn, resourceProject, token, pProject);
        
        if (syn == 0) break;                    // 文件结束
        if (syn == -1) continue;                // 跳过非法字符
        if (syn == -3) {                        // 非法数字
            printf("词法错误：非法数字常量或数字后缀 -> %s\n", token);
            fprintf(fp1, "词法错误：非法数字常量或数字后缀 -> %s\n", token);
            continue;
        }
        if (syn == 200) {                       // 预处理指令：输出但不处理内部
            printf("(预处理指令, %s)\n", token);
            fprintf(fp1, "(预处理指令, %s)\n", token);
            continue;
        }
        
        // 正常 token 输出
        if (syn == 100) {
            int found = 0;
            for(i = 0; i < identifierCount; i++) {
                if(strcmp(IDentifierTbl[i], token) == 0) {
                    found = 1; break;
                }
            }
            if(!found && identifierCount < 1000) {
                strcpy(IDentifierTbl[identifierCount++], token);
            }
            printf("(标识符, %s)\n", token);
            fprintf(fp1, "(标识符, %s)\n", token);
        }
        else if (syn >= 1 && syn <= 32) {
            printf("(关键字, %s)\n", token);
            fprintf(fp1, "(关键字, %s)\n", token);
        }
        else if (syn == 99)  { printf("(十进制整数, %s)\n", token); fprintf(fp1, "(十进制整数, %s)\n", token); }
        else if (syn == 101) { printf("(浮点数, %s)\n", token); fprintf(fp1, "(浮点数, %s)\n", token); }
        else if (syn == 102) { printf("(字符串常量, %s)\n", token); fprintf(fp1, "(字符串常量, %s)\n", token); }
        else if (syn == 103) { printf("(八进制数, %s)\n", token); fprintf(fp1, "(八进制数, %s)\n", token); }
        else if (syn == 104) { printf("(十六进制数, %s)\n", token); fprintf(fp1, "(十六进制数, %s)\n", token); }
        else if (syn >= 33 && syn <= 68) {
            printf("(运算符, %s)\n", token);
            fprintf(fp1, "(运算符, %s)\n", token);
        }
    }
    
    // 输出标识符表
    fprintf(fp1, "\n标识符表:\n");
    for (i = 0; i < identifierCount; i++) {
        printf("第%d个标识符: %s\n", i + 1, IDentifierTbl[i]);
        fprintf(fp1, "第%d个标识符: %s\n", i + 1, IDentifierTbl[i]);
    }
    
    fclose(fp1);
    cout << "词法分析完成！结果已保存到 lex_out.txt" << endl;
    return 0;
}