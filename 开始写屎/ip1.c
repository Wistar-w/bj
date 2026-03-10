#include<stdio.h>
void swap(int*a,int*b)
{
int temp = *a;   // *a 表示通过纸条 a 找到它指向的房间，把里面的数字取出来存到 temp
    *a = *b;         // 把纸条 b 指向的房间里的数字，放到纸条 a 指向的房间里
    *b = temp;       // 把 temp 里的数字放到纸条 b 指向的房间里
}
int main()
{
 int x, y;
    printf("请输入两个整数（空格分隔）：");
    scanf("%d %d", &x, &y);
    printf("交换前: x = %d, y = %d\n", x, y);
    // 调用 swap 函数，传入 x 和 y 的地址
    swap(&x, &y);
    printf("交换后: x = %d, y = %d\n", x, y);
    return 0;
}
