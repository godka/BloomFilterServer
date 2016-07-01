#include <memory.h>
#include <stdlib.h>
typedef struct BloomFilterType
{
	char* buf;
	int len;
	int k;
}BloomFilterType;
void* BF_Create(int num);
void BF_Add(void* BF,const char* str,int len);
int BF_Contains(void* BF, const char* str, int len);
int BF_Free(void* BF);