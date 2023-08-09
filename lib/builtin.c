typedef _Bool bool;
typedef unsigned size_t;

#define __MX_BUFFER_SIZE__ 4096

// Necessary functions in libc

int printf(const char *pattern, ...);
int sprintf(char *dest, const char *pattern, ...);
int scanf(const char *pattern, ...);
int sscanf(const char *src, const char *pattern, ...);
size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
void *memcpy(void *dest, const void *src, size_t n);
void *malloc(size_t n);


char *__String_substring__(char *str,int l,int r) {
    int len = r - l;
    char *buf = malloc(len + 1);
    memcpy(buf, str + l, len);
    buf[len] = '\0';
    return buf;
}

int __String_parseInt__(char *str) {
    int ret;
    sscanf(str, "%d", &ret);
    return ret;
}

int __String_ord__(char *str,int n) {
    return str[n];
}

void __print__(char *str) {
    printf("%s", str);
}

void __println__(char *str) {
    printf("%s\n", str);
}

void __printInt__(int n) {
    printf("%d", n);
}

void __printlnInt__(int n) {
    printf("%d\n", n);
}

char *__getString__() {
    char *buf = malloc(__MX_BUFFER_SIZE__);
    scanf("%s", buf);
    return buf;
}

int __getInt__() {
    int ret;
    scanf("%d", &ret);
    return ret;
}

char *__toString__(int n) {
    /* Align to 16 */
    char *buf = malloc(16);
    sprintf(buf, "%d", n);
    return buf;
}

char *__string_add__(char *lhs,char *rhs) {
    int len1 = strlen(lhs);
    int len2 = strlen(rhs);
    char *buf = malloc(len1 + len2 + 1);
    memcpy(buf, lhs, len1);
    memcpy(buf + len1, rhs, len2 + 1);
    return buf;
}

bool __string_eq__(char *lhs,char *rhs) {
    return strcmp(lhs, rhs) == 0;
}

bool __string_ne__(char *lhs,char *rhs) {
    return strcmp(lhs, rhs) != 0;
}

bool __string_gt__(char *lhs,char *rhs) {
    return strcmp(lhs, rhs) > 0;
}

bool __string_ge__(char *lhs,char *rhs) {
    return strcmp(lhs, rhs) >= 0;
}

bool __string_lt__(char *lhs,char *rhs) {
    return strcmp(lhs, rhs) < 0;
}

bool __string_le__(char *lhs,char *rhs) {
    return strcmp(lhs, rhs) <= 0;
}

int __Array_size(void *array) {
    return *(((int *)array) - 1);
}

void *__new_array4__(int len) {
    int *buf = (int *)malloc((len + 1) << 2);
    *buf = len;
    return buf + 1;
}

void *__new_array1__(int len) {
    int *buf = (int *)malloc(len + 4);
    *buf = len;
    return buf + 1;
}
