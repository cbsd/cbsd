#include <stdio.h>
#include <smmintrin.h>

int main(void)
{
    int pop = _mm_popcnt_u32(0xf0f0f0f0ULL);
    printf("pop = %d\n", pop);
    return 0;
}

