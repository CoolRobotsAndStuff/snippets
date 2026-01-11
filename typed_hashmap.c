#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define HASH_MAP_INIT_COUNT 64
#define HASH_MAP_TOMBSTONE 69
#define HASH_MAP_MAX_FILL_PERCENT 80

#define TYPED_HASH_MAP(struct_name, func_prefix, key_type, val_type, hash_func, equals_func) \
                                                                           \
typedef struct struct_name {                                               \
    key_type* keys;                                                        \
    val_type* vals;                                                        \
    key_type (*key_new)(key_type);                                         \
    val_type (*val_new)(val_type);                                         \
    void (*key_destr)(key_type);                                           \
    void (*val_destr)(val_type);                                           \
    size_t capacity;                                                       \
    size_t count;                                                          \
} struct_name;                                                             \
                                                                           \
struct_name func_prefix##_new() {                                          \
    struct_name ret = {0};                                                 \
    ret.keys = malloc(HASH_MAP_INIT_COUNT * sizeof(key_type));             \
    ret.vals = malloc(HASH_MAP_INIT_COUNT * sizeof(val_type));             \
    memset(ret.keys, 0, HASH_MAP_INIT_COUNT * sizeof(key_type));           \
    memset(ret.vals, 0, HASH_MAP_INIT_COUNT * sizeof(val_type));           \
    ret.capacity = HASH_MAP_INIT_COUNT;                                    \
    ret.count = 0;                                                         \
    return ret;                                                            \
}                                                                          \
                                                                           \
struct_name func_prefix##_new_managed(key_type (*key_new)(key_type),       \
                                      void (*key_destr)(key_type),         \
                                      val_type (*val_new)(val_type),       \
                                      void (*val_destr)(val_type)){        \
    struct_name ret = {0};                                                 \
    ret.key_new   = key_new;                                               \
    ret.key_destr = key_destr;                                             \
    ret.val_new   = val_new;                                               \
    ret.val_destr = val_destr;                                             \
    ret.keys = malloc(HASH_MAP_INIT_COUNT * sizeof(key_type));             \
    ret.vals = malloc(HASH_MAP_INIT_COUNT * sizeof(val_type));             \
    memset(ret.keys, 0, HASH_MAP_INIT_COUNT * sizeof(key_type));           \
    memset(ret.vals, 0, HASH_MAP_INIT_COUNT * sizeof(val_type));           \
    ret.capacity = HASH_MAP_INIT_COUNT;                                    \
    return ret;                                                            \
}                                                                          \
                                                                           \
void func_prefix##_free(struct_name* hs) {                                 \
    for (size_t i = 0; func_prefix##_next(*hs, &i); ++i) {                 \
        if (hs->key_destr != NULL) hs->key_destr(hs->keys[i]);             \
        if (hs->val_destr != NULL) hs->val_destr(hs->vals[i]);             \
    }                                                                      \
    free(hs->keys);                                                        \
    free(hs->vals);                                                        \
    hs->keys = NULL;                                                       \
    hs->vals = NULL;                                                       \
    hs->capacity = 0;                                                      \
    hs->count = 0;                                                         \
}                                                                          \
                                                                           \
ssize_t func_prefix##_find(struct_name* hs, key_type key) {                \
    assert(hs->capacity > 0);                                              \
    size_t index = hash_func(hs->capacity, key);                           \
    size_t init_index = index;                                             \
    assert(index < hs->capacity);                                          \
    while (1) {                                                            \
        if (hs->keys[index] == (key_type){0}) {                            \
            if (*(char*)(&hs->vals[index]) == (char)HASH_MAP_TOMBSTONE) {  \
                index = (index + 1)%hs->capacity;                          \
                if (index == init_index) return -1;                        \
                continue;                                                  \
            }                                                              \
            return -1;                                                     \
        }                                                                  \
        if (equals_func(hs->keys[index], key)) return index;               \
        index = (index + 1)%hs->capacity;                                  \
        if (index == init_index) return -1;                                \
    }                                                                      \
}                                                                          \
                                                                           \
bool func_prefix##_next(struct_name hs, size_t* i) {                       \
    for (; (*i) < hs.capacity; (*i)++) {                                   \
        if (hs.keys[*i] != (key_type){0}) return true;                     \
    }                                                                      \
    return false;                                                          \
}                                                                          \
                                                                           \
void func_prefix##_set(struct_name* hs, key_type key, val_type val) {      \
    assert(hs->capacity > 0);                                              \
    hs->count++;                                                           \
    if (hs->count > (hs->capacity*HASH_MAP_MAX_FILL_PERCENT/100)) {        \
        size_t new_cap = hs->capacity*2;                                   \
        key_type* new_keys = malloc(new_cap * sizeof(key_type));           \
        val_type* new_vals = malloc(new_cap * sizeof(val_type));           \
        memset(new_keys, 0, new_cap * sizeof(key_type));                   \
        memset(new_vals, 0, new_cap * sizeof(val_type));                   \
        for (size_t i = 0; func_prefix##_next(*hs, &i); ++i) {             \
            size_t new_index = hash_func(new_cap, hs->keys[i]);            \
            assert(new_index < new_cap);                                   \
            while (new_keys[new_index] != NULL)                            \
                new_index = (new_index + 1)%new_cap;                       \
            new_keys[new_index] = hs->keys[i];                             \
            new_vals[new_index] = hs->vals[i];                             \
        }                                                                  \
        free(hs->keys);                                                    \
        free(hs->vals);                                                    \
        hs->keys = new_keys;                                               \
        hs->vals = new_vals;                                               \
        hs->capacity = new_cap;                                            \
    }                                                                      \
    ssize_t index = func_prefix##_find(hs, key);                           \
    if (index < 0) {                                                       \
        index = hash_func(hs->capacity, key);                              \
        assert(index < hs->capacity);                                      \
        while (hs->keys[index] != NULL)                                    \
            index = (index + 1)%hs->capacity;                              \
    }                                                                      \
    hs->keys[index] = hs->key_new == NULL ? key : hs->key_new(key);        \
    hs->vals[index] = hs->val_new == NULL ? val : hs->val_new(val);        \
}                                                                          \
                                                                           \
                                                                           \
bool func_prefix##_exists(struct_name* hs, key_type key) {                 \
    return func_prefix##_find(hs, key) >= 0;                               \
}                                                                          \
                                                                           \
val_type func_prefix##_get(struct_name* hs, key_type key) {                \
    ssize_t index = func_prefix##_find(hs, key);                           \
    if (index < 0) return (val_type){0};                                   \
    return hs->vals[index];                                                \
}                                                                          \
                                                                           \
void func_prefix##_del(struct_name* hs, key_type key) {                    \
    assert(hs->capacity > 0);                                              \
    ssize_t index = func_prefix##_find(hs, key);                           \
    if (index < 0) return;                                                 \
    hs->vals[index] = (val_type){0};                                       \
    char tmp = HASH_MAP_TOMBSTONE;                                         \
    memcpy(&(hs->vals[index]), &tmp, sizeof(char));                        \
    hs->keys[index] = (key_type){0};                                       \
}                                                                          \
                                                                           \
val_type func_prefix##_get_copy(struct_name* hs, key_type key) {           \
    assert(hs->val_new != NULL &&                                          \
           #func_prefix"_get_copy only available for managed hashmaps");   \
    return hs->val_new(func_prefix##_get(hs, key));                        \
}                                                                          \
                                                                           \

uint32_t FNV_1a(void *key, int length) {
    uint8_t *bytes = (uint8_t*)key;
    uint32_t hash = 2166136261u;

    for (int i = 0; i < length; i++) {
        hash ^= bytes[i];
        hash *= 16777619;
    }
    return hash;
}

uint32_t int_hash(size_t capacity, int data  ) { return ((uint32_t)data)%capacity;           }
uint32_t str_hash(size_t capacity, char* data) { return FNV_1a(data, strlen(data))%capacity; }

bool int_equals(int data1, int data2)     { return data1 == data2;            }
bool str_equals(char* data1, char* data2) { return strcmp(data1, data2) == 0; }
void strfree(char* str) { free(str); }

/* For greppabilty purposes
} Str2Str;
Str2Str str2str_new(
Str2Str str2str_new_managed(
char* str2str_get(
char* str2str_get_copy(
void str2str_set(
void str2str_del(
bool str2str_exists(
ssize_t str2str_find(
*/
TYPED_HASH_MAP(Str2Str, str2str, char*, char*, str_hash, str_equals)

typedef struct {
    double bar;
    int baz;
} Foo;


TYPED_HASH_MAP(Str2Foo, str2foo, char*, Foo, str_hash, str_equals)

int main() {
    // Stack allocated or manual memory management
    Str2Str hm1 = str2str_new();

    str2str_set(&hm1, "hello",  "world" );
    str2str_set(&hm1, "mother", "fucker");
    str2str_set(&hm1, "mother", "fucka" );

    // get reference
    printf("hello  = %s\n", str2str_get(&hm1, "hello" )); 
    printf("mother = %s\n", str2str_get(&hm1, "mother"));

    // Heap allocated, automatically managed
    Str2Str hm2 = str2str_new_managed(strdup, strfree, strdup, strfree);

    str2str_set(&hm2, "hello",  "world" );
    str2str_set(&hm2, "mother", "fucker");
    str2str_set(&hm2, "mother", "fucka" );

    // get reference
    printf("hello  = %s\n", str2str_get(&hm2, "hello" )); 
    printf("mother = %s\n", str2str_get(&hm2, "mother"));

    // make a copy of value with provided constructor
    printf("hello  = %s\n", str2str_get_copy(&hm2, "hello" )); 
    printf("mother = %s\n", str2str_get_copy(&hm2, "mother"));
    
    str2str_set(&hm2, "foo", "bar" );
    str2str_set(&hm2, "let's", "go" );

    printf("{");
    for (size_t i = 0; str2str_next(hm2, &i); ++i) {
        printf("\n    \"%s\": \"%s\",", hm2.keys[i], hm2.vals[i]);
    }
    printf("\b ");
    puts("\n}");

    // Struct type (stored as value)
    Str2Foo hm3 = str2foo_new();

    str2foo_set(&hm3, "cool",   (Foo){6.9, 420});
    str2foo_set(&hm3, "cringe", (Foo){6.7, 67 });

    // get reference
    printf("cool   = %lf, %d\n", str2foo_get(&hm3, "cool").bar, str2foo_get(&hm3, "cool").baz); 
    if (str2foo_exists(&hm3, "cringe")) {
        printf("cringe = %lf, %d\n", str2foo_get(&hm3, "cringe").bar, str2foo_get(&hm3, "cringe").baz);
    } else {
        puts("no cringe in this town");
    }

    str2foo_del(&hm3, "cringe");
    puts("deleting cringe");
    printf("cool   = %lf, %d\n", str2foo_get(&hm3, "cool").bar, str2foo_get(&hm3, "cool").baz); 
    if (str2foo_exists(&hm3, "cringe")) {
        printf("cringe = %lf, %d\n", str2foo_get(&hm3, "cringe").bar, str2foo_get(&hm3, "cringe").baz);
    } else {
        puts("no cringe in this town");
    }

    str2str_free(&hm1);
    str2str_free(&hm2);
    str2foo_free(&hm3);
}

