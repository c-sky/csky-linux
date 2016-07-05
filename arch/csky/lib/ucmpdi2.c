#include <linux/module.h>

typedef int word_type __attribute__ ((mode (__word__)));

#ifdef __BIG_ENDIAN
struct DWstruct {
	int high, low;
};
#elif defined(__LITTLE_ENDIAN)
struct DWstruct {
	int low, high;
};
#else
#error I feel sick.
#endif

typedef union {
	struct DWstruct s;
	long long ll;
} DWunion;


word_type __ucmpdi2(unsigned long long a, unsigned long long b)
{
	const DWunion au = {.ll = a};
	const DWunion bu = {.ll = b};

	if ((unsigned int) au.s.high < (unsigned int) bu.s.high)
		return 0;
	else if ((unsigned int) au.s.high > (unsigned int) bu.s.high)
		return 2;
	if ((unsigned int) au.s.low < (unsigned int) bu.s.low)
		return 0;
	else if ((unsigned int) au.s.low > (unsigned int) bu.s.low)
		return 2;
	return 1;
}

EXPORT_SYMBOL(__ucmpdi2);
