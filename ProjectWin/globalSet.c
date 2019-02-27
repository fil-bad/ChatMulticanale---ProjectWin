#include "globalSet.h"

/* REALLOCARRAY */

void *reallocarray(void *optr, size_t nmemb, size_t size) {
	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
		nmemb > 0 && SIZE_MAX / nmemb < size) {
		errno = ENOMEM;
		return NULL;
	}
	void* ptr = malloc(size * nmemb);
	if (optr) {
		memcpy(ptr, optr, size * nmemb);
		free(optr);
	}
	return ptr;
}