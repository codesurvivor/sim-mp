#include<stdio.h>

main()
{
unsigned int a;
unsigned long int b;
unsigned long long int c;

a = 0 -1;
b = 0 -1;
c = 0 -1;

printf("Int Max = %u \t Long Max = %lu \t Long Long Max = %llu\n", a, b, c);
printf("Int Max = %d \t Long Max = %d \t Long Long Max = %d\n", sizeof(a),sizeof(b),sizeof(c));
}

