#include <stdio.h>
#include <cpuid.h>

int main() {
    unsigned int ret=0;
    unsigned int eax, ebx, ecx, edx;

    // 1. v2 v2 (req: SSE4.2 и POPCNT)
    // call CPUID with EAX=1
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);

    int has_sse42  = (ecx & (1 << 20)) != 0;
    int has_popcnt = (ecx & (1 << 23)) != 0;
    int has_ssse3  = (ecx & (1 <<  9)) != 0;

    // v3 (req: AVX2, BMI1, BMI2)
    // call CPUID with EAX=7, ECX=0
    unsigned int eax7, ebx7, ecx7, edx7;
    __cpuid_count(7, 0, eax7, ebx7, ecx7, edx7);

    int has_avx2 = (ebx7 & (1 << 5)) != 0;
    int has_bmi1 = (ebx7 & (1 << 3)) != 0;
    int has_bmi2 = (ebx7 & (1 << 8)) != 0;

    printf("--- Check CPU capabilities ---\n");
    printf("SSE4.2: %s\n", has_sse42 ? "OK" : "Missing");
    printf("POPCNT: %s\n", has_popcnt ? "OK" : "Missing");
    printf("AVX2:   %s\n", has_avx2 ? "OK" : "Missing");
    printf("BMI2:   %s\n", has_bmi2 ? "OK" : "Missing");
    printf("Summary: ");
    if (has_sse42 && has_popcnt && has_ssse3) {
        if (has_avx2 && has_bmi1 && has_bmi2) {
            printf("Your CPU supports x86-64-v3 (compatible with modern distro builds).\n");
            ret=0;
        } else {
            printf("Your CPU only supports x86-64-v2 (you might face issues with modern distro builds).\n");
            ret=1;
        }
    } else {
        printf("Your CPU is below x86-64-v2 (hypervisor performance issues are likely).\n");
        ret=1;
    }

    return ret;
}
