### **解决回文串**(力扣第5题)
#### 1.函数签名和边界处理
```c
char* longestPalindrome(char* s) {
int n = strlen(s);//首先获取原字符串长度 n。
//strlen(s) 是 C 语言标准库 **<string.h>** 中提供的一个函数，用于计算字符串的长度。
    if (n <= 1) return s;
    //如果长度小于等于 1，那么它本身就是回文，直接返回原指针
```
#### 2. 构造预处理字符串 *t*
```c
int t_len = 2 * n + 3;//变成奇数长度。
char* t = (char*)malloc(t_len + 1);// +1 用于存放 '\0'
if (t == NULL) return NULL;
t[0] = '^';
t[1] = '#';
for (int i = 0; i < n; i++) {
    t[2 * i + 2] = s[i];
    t[2 * i + 3] = '#';
}
t[t_len - 1] = '$';
t[t_len] = '\0';
```
* 所有回文子串在 t 中都变成奇数长度（中心要么是原字符，要么是 #）
* 插入#，在每个数据中。

#### 3. Manacher 核心变量
```c
int* p = (int*)calloc(t_len, sizeof(int)); // p[i] 为回文半径（包括中心）
if (p == NULL) {
   free(t);//调用 free(p) 释放内存，否则会造成内存泄漏。
    return NULL;
}
int c = 0, r = 0;       // r 为最右回文边界的下一个位置，c 为其中心
int max_r = 0, center = 0;// 记录最大半径及其中心
```