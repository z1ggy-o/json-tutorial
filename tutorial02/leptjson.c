#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <string.h>
#include <errno.h>  /* errno, ERANGE */
#include <math.h>  /* HUGE_VAL */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context *c, lept_value *v, const char *literal, int type)
{
    size_t i;  // for size, use size_t is better
    EXPECT(c, *literal);
    // int i, end, len;
    // len = strlen(literal);  Do not need to use length, use literal's last char
    for (i = 0; literal[i+1]; i++) {
        if (c->json[i] != literal[i+1])
            return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)  ((ch) >= '1' && (ch) <= '9')

static int is_valid_number(const char *s)
{
    const char *sp = s;

    if (*sp == '-') sp++;
    if (*sp == '0') sp++;
    else {
        if (!ISDIGIT1TO9(*sp)) return 0;

        #if 0
        Macro's misuse case !!!
         * here *++sp will be use twice. Do not use expression that has side effective in macro
        
        while(ISDIGIT(*++sp))
            ;        
        #endif

        for (sp++; ISDIGIT(*sp); sp++);
    }
    if (*sp == '.') {
        sp++;
        if (!ISDIGIT(*sp)) return 0;
        for (sp++; ISDIGIT(*sp); sp++);
    }
    if (*sp == 'e' || *sp == 'E') {
        sp++;
        if (*sp == '+' || *sp == '-') sp++;
        if (!ISDIGIT(*sp)) return 0;
        for (sp++; ISDIGIT(*sp); sp++);
    }
    return sp - s;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    int offset;
    if (!(offset = is_valid_number(c->json)))
        return LEPT_PARSE_INVALID_VALUE;

    errno = 0;  // If error occurs, macro will write error code into errno.
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    c->json += offset;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
