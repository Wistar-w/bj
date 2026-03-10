/*解法1*/
#include <stdbool.h>  // 使用bool类型需要此头文件
#define MAX(a, b) ((b) > (a) ? (b) : (a))
/* #define - 预处理指令，定义宏
MAX - 宏的名称
使用三元条件运算符 ? 
逻辑：如果 b > a，则返回 b，否则返回 a
*/

int lengthOfLongestSubstring(char* s) {
//函数名：lengthOfLongestSubstring
//参数：char* s - 指向字符串的指针
//返回值：int - 最长无重复字符子串的长度
    int ans = 0, left = 0;
    bool has[128] = {0}; //创建一个布尔数组，大小为128
//用字符的ASCII值直接作为下标，在 O(1) 时间内判断该字符是否出现过。
// 也可以用哈希集合，这里为了效率用的数组
    for (int right = 0; s[right]; right++) {//滑动窗口
//从字符串开头遍历到结尾（当s[right]为'\0'时结束）
//s[right] 是当前遍历到的字符（char 类型）
//如果 s 指向字符串 "hello"，s[5] 是 '\0'
       unsigned char c = (unsigned char)s[right]; // 转换为无符号
         while (has[c]) {//是否重复
            has[(unsigned char)s[left]] = false;
            left++;//移出去
        }
        has[c] = true; // 加入 c
        ans = MAX(ans, right - left + 1); // 更新窗口长度最大值
    }
    return ans;
}
/*解2*/
 
#define MAX(a, b) ((b) > (a) ? (b) : (a))
 int lengthOfLongestSubstring(char* s) {
    int lastIndex[128];  // 记录字符最后出现的位置
    memset(lastIndex, -1, sizeof(lastIndex));  // 初始化为-1
    
    int ans = 0, left = 0;
    for (int right = 0; s[right]; right++) {
        char c = s[right];
        // 如果字符c在left之后出现过，直接跳到c上次出现位置的下一位
        if (lastIndex[c] >= left) {
            left = lastIndex[c] + 1;
        }
        lastIndex[c] = right;  // 更新c的最后出现位置
        ans = MAX(ans, right - left + 1);
    }
    return ans;
}