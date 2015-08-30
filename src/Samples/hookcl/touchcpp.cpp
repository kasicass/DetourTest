// see clcoffee.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

void TouchHelloCpp(const char* filename, unsigned char *xorvalue, size_t valuelen)
{
	const char src[] = "#include <iostream>\n"
		"int main() {\n"
		"  std::cout << \"Hello cl.exe\" << std::endl;\n"
		"  return 0;\n"
		"}\n";

	FILE *fp = fopen(filename, "wb");

	size_t j = 0;
	size_t n = strlen(src);
	for (size_t i = 0; i < n; ++i)
	{
		fputc(src[i] ^ xorvalue[j], fp);
		++j;
		if (j == valuelen) j = 0;
	}

	fclose(fp);
}

unsigned char hex2dec(char c)
{
	c = toupper(c);
	if (c > '9')
	{
		assert('A' <= c && c <= 'F');
		return c - 'A' + 10;
	}
	else
	{
		assert('0' <= c && c <= '9');
		return c - '0';
	}
}

int main()
{
	const char* xorvalueStr = getenv("CLCOFFEE_VALUE");
	const char* fileStr = getenv("CLCOFFEE_FILE");
	
	size_t len1 = strlen(xorvalueStr);
	size_t len2 = strlen(fileStr);
	assert(len1 >= 2 && len1%2 == 0);
	assert(len2 > 0);

	size_t valuelen = len1 / 2;
	unsigned char *xorvalue = (unsigned char*)malloc(valuelen);
	for (size_t i = 0; i < len1; i+=2)
		xorvalue[i/2] = hex2dec(xorvalueStr[i])*16 + hex2dec(xorvalueStr[i+1]);
	
	printf("CLCOFFEE_VALUE: ");
	for (size_t j = 0; j < valuelen; ++j)
	{
		printf("%d ", xorvalue[j]);
	}
	printf("\n");

	printf("CLCOFFEE_FILE: %s\n", fileStr);
	TouchHelloCpp(fileStr, xorvalue, valuelen);

	free(xorvalue);
	return 0;
}