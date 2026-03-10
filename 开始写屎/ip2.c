#include<stdio.h>
void sum_diff(int a,int b,int *sum,int *diff)
{
    *sum=a+b;//sum指向a,b之和。
    *diff=a-b;//diff指向a,b之减。
}
int main()
{
    int x,y;
    int s,d;
    printf("请输入2个整数,并用空格隔开");
    scanf("%d %d",&x,&y);
sum_diff(x,y,&s,&d);
printf("和为:%d 差为:%d",s,d);
return 0;
}