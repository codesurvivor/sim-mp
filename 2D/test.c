#include<stdio.h>

void main()
{
 	unsigned long long int bit[4] = {0};
	int i = 0;
	bit[0] = 1<<127;
	for(i=0;i<4;i++)
		printf("bit %d is %llx\n", i, bit[i]);
	return;
}
