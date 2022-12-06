# mjson - a JSON parser + emitter

[![Build Status]( https://github.com/cesanta/mjson/workflows/build/badge.svg)](https://github.com/cesanta/mjson/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)
[![Code Coverage](https://codecov.io/gh/cesanta/mjson/branch/master/graph/badge.svg)](https://codecov.io/gh/cesanta/mjson)

# Features

- Small, ~1k lines of code, embedded-friendly
- No dependencies
- State machine parser, no allocations, no recursion
- High level API - fetch from JSON directly into C/C++ by
    [jsonpath](https://github.com/json-path/JsonPath)
- Low level SAX API
- Flexible JSON generation API - print to buffer, file, socket, etc

## Parsing example

```c
const char *s = "{\"a\":1,\"b\":[2,false]}";  // {"a":1,"b":[2,false]}

double val;                                       // Get `a` attribute
if (mjson_get_number(s, strlen(s), "$.a", &val))  // into C variable `val`
  printf("a: %g\n", val);                         // a: 1

const char *buf;  // Get `b` sub-object
int len;          // into C variables `buf,len`
if (mjson_find(s, strlen(s), "$.b", &buf, &len))  // And print it
  printf("%.*s\n", len, buf);                     // [2,false]

int v;                                           // Extract `false`
if (mjson_get_bool(s, strlen(s), "$.b[1]", &v))  // into C variable `v`
  printf("boolean: %d\n", v);                    // boolean: 0
```

## Printing example

```c
// Print into a statically allocated buffer
char buf[100];
mjson_snprintf(buf, sizeof(buf), "{%Q:%d}", "a", 123);
printf("%s\n", buf);  // {"a":123}

// Print into a dynamically allocated string
char *s = mjson_aprintf("{%Q:%g}", "a", 3.1415);
printf("%s\n", s);  // {"a":3.1415}
free(s);            // Don't forget to free an allocated string
```

# Build options

- `-D MJSON_ENABLE_PRINT=0` disable emitting functionality, default: enabled
- `-D MJSON_MAX_DEPTH=30` define max object depth, default: 20
- `-D MJSON_ENABLE_BASE64=0` disable base64 parsing/printing, default: enabled
- `-D MJSON_ENABLE_RPC=0` disable RPC functionality, default: enabled
- `-D MJSON_DYNBUF_CHUNK=256` sets the allocation granularity of `mjson_print_dynamic_buf`
- `-D MJSON_ENABLE_PRETTY=0` disable `mjson_pretty()`, default: enabled
- `-D MJSON_ENABLE_MERGE=0` disable `mjson_merge()`, default: enabled
- `-D MJSON_ENABLE_NEXT=0` disable `mjson_next()`, default: enabled
- `-D MJSON_REALLOC=my_realloc` redefine realloc() used by `mjson_print_dynamic_buf()`, default: realloc


# Parsing API

## mjson_find()

```c
int mjson_find(const char *s, int len, const char *path, const char **tokptr, int *toklen);
```

In a JSON string `s`, `len`, find an element by its JSONPATH `path`.
Save found element in `tokptr`, `toklen`.
If not found, return `JSON_TOK_INVALID`. If found, return one of:
`MJSON_TOK_STRING`, `MJSON_TOK_NUMBER`, `MJSON_TOK_TRUE`, `MJSON_TOK_FALSE`,
`MJSON_TOK_NULL`, `MJSON_TOK_ARRAY`, `MJSON_TOK_OBJECT`. If a searched key
contains `.`, `[` or `]` characters, they should be escaped by a backslash.

Example:

```c
// s, len is a JSON string: {"foo": { "bar": [ 1, 2, 3] }, "b.az": true} 
char *p;
int n;
assert(mjson_find(s, len, "$.foo.bar[1]", &p, &n) == MJSON_TOK_NUMBER);
assert(mjson_find(s, len, "$.b\\.az", &p, &n) == MJSON_TOK_TRUE);
assert(mjson_find(s, len, "$", &p, &n) == MJSON_TOK_OBJECT);
```

## mjson_get_number()

```c
int mjson_get_number(const char *s, int len, const char *path, double *v);
```

In a JSON string `s`, `len`, find a number value by its JSONPATH `path` and
store into `v`. Return 0 if the value was not found, non-0 if found and stored.
Example:

```c
// s, len is a JSON string: {"foo": { "bar": [ 1, 2, 3] }, "baz": true} 
double v = 0;
mjson_get_number(s, len, "$.foo.bar[1]", &v);  // v now holds 2
```

## mjson_get_bool()

```c
int mjson_get_bool(const char *s, int len, const char *path, int *v);
```

In a JSON string `s`, `len`, store value of a boolean by its JSONPATH `path`
into a variable `v`. Return 0 if not found, non-0 otherwise. Example:

```c
// s, len is a JSON string: {"foo": { "bar": [ 1, 2, 3] }, "baz": true} 
bool v = mjson_get_bool(s, len, "$.baz", false);   // Assigns to true
```

## mjson_get_string()

```c
int mjson_get_string(const char *s, int len, const char *path, char *to, int sz);
```
In a JSON string `s`, `len`, find a string by its JSONPATH `path` and unescape
it into a buffer `to`, `sz` with terminating `\0`.
If a string is not found, return -1.
If a string is found, return the length of unescaped string. Example:

```c
// s, len is a JSON string [ "abc", "de\r\n" ]
char buf[100];
int n = mjson_get_string(s, len, "$[1]", buf, sizeof(buf));  // Assigns to 4
```

## mjson_get_hex()

```c
int mjson_get_hex(const char *s, int len, const char *path, char *to, int sz);
```

In a JSON string `s`, `len`, find a string by its JSONPATH `path` and
hex decode it into a buffer `to`, `sz` with terminating `\0`.
If a string is not found, return -1.
If a string is found, return the length of decoded string.
The hex string should be lowercase, e.g. string `Hello` is hex-encoded as
`"48656c6c6f"`.  Example:

```c
// s, len is a JSON string [ "48656c6c6f" ]
char buf[100];
int n = mjson_get_hex(s, len, "$[0]", buf, sizeof(buf));  // Assigns to 5
```



## mjson_get_base64()

```c
int mjson_get_base64(const char *s, int len, const char *path, char *to, int sz);
```

In a JSON string `s`, `len`, find a string by its JSONPATH `path` and
base64 decode it into a buffer `to`, `sz` with terminating `\0`.
If a string is not found, return 0.
If a string is found, return the length of decoded string. Example:

```c
// s, len is a JSON string [ "MA==" ]
char buf[100];
int n = mjson_get_base64(s, len, "$[0]", buf, sizeof(buf));  // Assigns to 1
```


## mjson()

```c
int mjson(const char *s, int len, mjson_cb_t cb, void *cbdata);
```

Parse JSON string `s`, `len`, calling callback `cb` for each token. This
is a low-level SAX API, intended for fancy stuff like pretty printing, etc.


## mjson_next()

```c
int mjson_next(const char *s, int n, int off, int *koff, int *klen, int *voff,
               int *vlen, int *vtype);
```

Assuming that JSON string `s`, `n` contains JSON object or JSON array,
return the next key/value pair starting from offset `off`.
key is returned as  `koff` (key offset), `klen` (key length), value is returned as `voff` (value offset),
`vlen` (value length), `vtype` (value type). Pointers could be NULL.
Return next offset. When iterating over the array, `koff` will hold value
index inside an array, and `klen` will be `0`. Therefore, if `klen` holds
`0`, we're iterating over an array, otherwise over an object.
Note: initial offset should be 0.

Usage example:

```c
const char *s = "{\"a\":123,\"b\":[1,2,3,{\"c\":1}],\"d\":null}";
int koff, klen, voff, vlen, vtype, off;

for (off = 0; (off = mjson_next(s, strlen(s), off, &koff, &klen,
																&voff, &vlen, &vtype)) != 0; ) {
	printf("key: %.*s, value: %.*s\n", klen, s + koff, vlen, s + voff);
}
```


# Emitting API


The emitting API is flexible and can print to anything: fixed buffer,
dynamic growing buffer, FILE *, network socket, etc etc. The printer function
gets the pointer to the buffer to print, and a user-specified data:

```c
typedef int (*mjson_print_fn_t)(const char *buf, int len, void *userdata);
```

mjson library defines the following built-in printer functions:

```c
struct mjson_fixedbuf {
  char *ptr;
  int size, len;
};
int mjson_print_fixed_buf(const char *ptr, int len, void *userdata);

int mjson_print_file(const char *ptr, int len, void *userdata);
int mjson_print_dynamic_buf(const char *ptr, int len, void *userdata);
```

If you want to print to something else, for example to a network socket,
define your own printing function. If you want to see usage examples
for the built-in printing functions, see `unit_test.c` file.

## mjson_printf()

```c
int mjson_vprintf(mjson_print_fn_t, void *, const char *fmt, va_list ap);
int mjson_printf(mjson_print_fn_t, void *, const char *fmt, ...);
```

Print using `printf()`-like format string. Supported specifiers are:

- `%Q` print quoted escaped string. Expect NUL-terminated `char *`
- `%.*Q` print quoted escaped string. Expect `int, char *`
- `%s` print string as is. Expect NUL-terminated `char *`
- `%.*s` print string as is. Expect `int, char *`
- `%g`, print floating point number, precision is set to 6. Expect `double`
- `%.*g`, print floating point number with given precision. Expect `int, double`
- `%d`, `%u` print signed/unsigned integer. Expect `int`
- `%ld`, `%lu` print signed/unsigned long integer. Expect `long`
- `%B` print `true` or `false`. Expect `int`
- `%V` print quoted base64-encoded string. Expect `int, char *`
- `%H` print quoted hex-encoded string. Expect `int, char *`
- `%M` print using custom print function. Expect `int (*)(mjson_print_fn_t, void *, va_list *)`

The following example produces `{"a":1, "b":[1234]}` into the
dynamically-growing string `s`.
Note that the array is printed using a custom printer function:

```c
static int m_printer(mjson_print_fn_t fn, void *fndata, va_list *ap) {
  int value = va_arg(*ap, int);
  return mjson_printf(fn, fndata, "[%d]", value);
}

...
char *s = NULL;
mjson_printf(&mjson_print_dynamic_buf, &s, "{%Q:%d, %Q:%M}", "a", 1, "b", m_printer, 1234);
/* At this point `s` contains: {"a":1, "b":[1234]}  */
free(s);
```

## mjson_snprintf()

```c
int mjson_snprintf(char *buf, size_t len, const char *fmt, ...);
```

A convenience function that prints into a given string.

## mjson_aprintf()

```c
char *mjson_aprintf(const char *fmt, ...);
```

A convenience function that prints into an allocated string. A returned
pointer must be `free()`-ed by a caller.

## mjson_pretty()

```c
int mjson_pretty(const char *s, int n, const char *pad,
                 mjson_print_fn_t fn, void *userdata);
```

Pretty-print JSON string `s`, `n` using padding `pad`. If `pad` is `""`,
then a resulting string is terse one-line. Return length of the printed string.


## mjson_merge()

```c
int mjson_merge(const char *s, int n, const char *s2, int n2,
                mjson_print_fn_t fn, void *fndata);
```

Merge JSON string `s2`,`n2` into the original string `s`,`n`. Both strings
are assumed to hold objects. The result is printed using `fn`,`fndata`.
Return value: number of bytes printed.

In order to delete the key in the original string, set that key to `null`
in the `s2`,`n2`.
NOTE: both strings must not contain arrays, as merging arrays is not supported.

# Example - connect Arduino Uno to AWS IoT device shadow

[![](http://i3.ytimg.com/vi/od1rsIrvwrM/hqdefault.jpg)](https://www.youtube.com/watch?v=od1rsIrvwrM)

See https://vcon.io for more information.

# Contact

Please visit https://vcon.io/contact.html
