#include "stdlib.h"
#include "string.h"

int atoi(const char *str) {
    int value = 0;
    bool negative = false;
    if (str[0] == '-') {
        negative = true;
        str++;
    }

    for (size_t i = 0; i < strlen(str); i++) {
        value *= 10;
        value += str[i] - '0';
    }

    return (negative ? -value : value);
}

bool isalnum(char c) {
    return isalpha(c) || isdigit(c);
}

bool isalpha(char c) {
    return isupper(c) || islower(c);
}

bool iscntrl(char c) {
    return c < 32 || c == 127;
}

bool isdigit(char c) {
    return c >= '0' && c <= '9';
}

bool islower(char c) {
    return c >= 'a' && c <= 'z';
}

bool isprint(char c) {
    return !iscntrl(c);
}

bool ispunct(char c) {
    return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

bool isspace(char c) {
    return c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c == ' ';
}

bool isupper(char c) {
    return c >= 'A' && c <= 'Z';
}

char tolower(char c) {
    if (!isalpha(c)) return c;
    if (islower(c)) return c;
    return c + 32;
}

char toupper(char c) {
    if (!isalpha(c)) return c;
    if (isupper(c)) return c;
    return c - 32;
}
