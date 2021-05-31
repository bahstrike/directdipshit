#include "dipshit.h"

const char* UnsafeTmpMBS(const wchar_t* wcs)
{
	static char sz[1024];

	int len = wcslen(wcs);
	wcstombs(sz, wcs, len);
	sz[len] = 0;

	return sz;
}