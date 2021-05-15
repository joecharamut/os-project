#ifndef OS_ATTRIBUTES_H
#define OS_ATTRIBUTES_H

#define SECTION(x) __attribute__((section(x)))
#define ALIGN(x) __attribute__((aligned(x)))
#define NORETURN __attribute__((noreturn))
#define UNUSED __attribute__((unused))
#define USED __attribute__((used))

#endif //OS_ATTRIBUTES_H
