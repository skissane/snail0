#include <stdio.h>

int main(void) {
	int c;
	while((c = fgetc(stdin)) != EOF) {
		printf("'\\x%X',", (unsigned)c);
	}
	printf("'\\0'");
	return 0;
}
