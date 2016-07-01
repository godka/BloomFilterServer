#include "BloomFliter.h"
#define MY_RAND(x) rand() % x
static int RSHash(const char* str, int len)
{
	int b = 378551;
	int a = 63689;
	int hash = 0;
	int i;
	for (i = 0; i < len; i++)
	{
		hash = hash * a + str[i];
		a = a * b;
	}
	return (hash & 0x7FFFFFFF);
}
void* BF_Create(int n){
	struct BloomFilterType* bf = (BloomFilterType*) malloc(sizeof(BloomFilterType));
	bf->buf = (char*) malloc(sizeof(char) * n * 10);
	bf->len = n * 10;
	memset(bf->buf, 0, bf->len);
	bf->k = 7;
	return bf;
}


void BF_Add(void* BF, const char* str, int len){
	int i;
	if (!BF)
		return;
	BloomFilterType* bft = (BloomFilterType*) BF;
	srand(RSHash(str, len));
	for (i = 0; i < bft->k; i++){
		bft->buf[MY_RAND(bft->len)] = 1;
	}
}

int BF_Contains(void* BF, const char* str, int len){
	int i;
	if (!BF)
		return NULL;
	BloomFilterType* bft = (BloomFilterType*) BF;
	srand(RSHash(str, len));
	for (i = 0; i < bft->k; i++){
		if (bft->buf[MY_RAND(bft->len)] == 0)
			return 0;
	}
	return 1;
}

int BF_Free(void* BF){
	if (BF){
		BloomFilterType* bft = (BloomFilterType*) BF;
		free(bft->buf);
		free(bft);
		bft = NULL;
	}
	BF = NULL;
}