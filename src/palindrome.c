#include<stdio.h>
int main()
{
	int n;
	scanf("%d",&n);
	int k = 1;
	int a[6];
	int i;
	if (n >= 10) k++;
	if (n >= 100) k++;
	if (n >= 1000) k++;
	if (n >= 10000) k++;
	for (i = 0; i < k; i++) {
		a[i] = n % 10;
		n /= 10;
	}
	for (i = 0; i < k; i++)
		if (a[i] != a[k - i - 1]) break;




	if(i == k){
		printf("Y");
	}else{
		printf("N");
	}
	return 0;
}
