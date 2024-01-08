#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
int main(){
	FILE *file;
	file = fopen("test.txt", "w");
	fprintf(file, "Hello World!\n");
	fclose(file);
	exit(1);
}

