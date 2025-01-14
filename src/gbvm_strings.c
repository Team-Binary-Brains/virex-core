#include "gbvm_strings.h"

String ltrim(String s)
{
    size_t i = 0;
    while (i < s.length && isspace(s.data[i])) {
        i++;
    }
    return (String) {
        .length = s.length - i,
        .data = s.data + i
    };
}

String rtrim(String s)
{
    size_t i = 0;
    while (i < s.length && isspace(s.data[s.length - i - 1])) {
        i++;
    }
    return (String) {
        .length = s.length - i,
        .data = s.data
    };
}

String trim(String s)
{
    return rtrim(ltrim(s));
}

String splitStr(String* s, char c)
{
    size_t i = 0;
    while (i < s->length && s->data[i] != c) {
        i++;
    }
    String res = {
        .length = i,
        .data = s->data
    };

    s->length -= i;
    s->data += i;

    if (i < s->length) {
        s->length -= 1;
        s->data += 1;
    }

    return res;
}

Word strToWord(String s)
{
    Word val = 0;
    size_t i = 0;

    while (i < s.length && isdigit(s.data[i])) {
        val = val * 10 + s.data[i] - '0';
        i++;
    }
    return val;
}

void printString(String s)
{
    printf("%.*s\n", (int)(s.length), s.data);
}