#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define HASH_MAP_INIT_COUNT 64

#define HASH_MAP_EMPTY 0
#define HASH_MAP_FULL 1
#define HASH_MAP_TOMBSTONE 2
#define HASH_MAP_MAX_FILL_PERCENT 80

#define TYPED_HASH_MAP(struct_name, func_prefix, key_type, val_type, hash_func, equals_func) \
                                                                           \
struct struct_name;                                                        \
                                                                           \
typedef struct struct_name {                                               \
    bool     (*next)(struct struct_name, size_t*);                         \
    ssize_t  (*find)(struct struct_name, key_type);                        \
    val_type (*get) (struct struct_name, key_type);                        \
    void     (*set) (struct struct_name*, key_type, val_type);             \
    key_type* keys;                                                        \
    val_type* vals;                                                        \
    unsigned char* stat;                                                   \
    key_type (*key_new)(key_type);                                         \
    val_type (*val_new)(val_type);                                         \
    void (*key_destr)(key_type);                                           \
    void (*val_destr)(val_type);                                           \
    size_t capacity;                                                       \
    size_t count;                                                          \
} struct_name;                                                             \
                                                                           \
bool func_prefix##_next(struct_name hs, size_t* i) {                       \
    for (; (*i) < hs.capacity; (*i)++) {                                   \
        if (hs.stat[*i] == HASH_MAP_FULL) return true;                     \
    }                                                                      \
    return false;                                                          \
}                                                                          \
                                                                           \
ssize_t func_prefix##_find(struct_name hm, key_type key) {                 \
    assert(hm.capacity > 0);                                               \
    size_t index = hash_func(hm.capacity, key);                            \
    size_t init_index = index;                                             \
    assert(index < hm.capacity);                                           \
    while (1) switch (hm.stat[index]) {                                    \
        case HASH_MAP_EMPTY: return -1;                                    \
        case HASH_MAP_FULL:                                                \
            if (equals_func(hm.keys[index], key)) return index;            \
        case HASH_MAP_TOMBSTONE:                                           \
            index = (index + 1)%hm.capacity;                               \
            if (index == init_index) return -1;                            \
            break;                                                         \
        default: assert(0 && "UNREACHABLE: invalid status.");              \
    }                                                                      \
}                                                                          \
                                                                           \
                                                                           \
void func_prefix##_set(struct_name* hm, key_type key, val_type val) {      \
    assert(hm->capacity > 0);                                              \
    if (hm->count >= (hm->capacity*HASH_MAP_MAX_FILL_PERCENT/100)) {       \
        size_t new_cap = hm->capacity*2;                                   \
        key_type* new_keys = malloc(new_cap * sizeof(key_type));           \
        val_type* new_vals = malloc(new_cap * sizeof(val_type));           \
        char*     new_stat = malloc(new_cap * sizeof(*new_stat));          \
        memset(new_keys, 0, new_cap * sizeof(key_type));                   \
        memset(new_vals, 0, new_cap * sizeof(val_type));                   \
        memset(new_stat, 0, new_cap * sizeof(*new_stat));                  \
        for (size_t i = 0; hm_next(*hm, &i); ++i) {                        \
            size_t new_index = hash_func(new_cap, hm->keys[i]);            \
            assert(new_index < new_cap);                                   \
            while (new_stat[new_index] == HASH_MAP_FULL)                   \
                new_index = (new_index + 1)%new_cap;                       \
            new_keys[new_index] = hm->keys[i];                             \
            new_vals[new_index] = hm->vals[i];                             \
            new_stat[new_index] = HASH_MAP_FULL;                           \
        }                                                                  \
        free(hm->keys);                                                    \
        free(hm->vals);                                                    \
        free(hm->stat);                                                    \
        hm->keys = new_keys;                                               \
        hm->vals = new_vals;                                               \
        hm->stat = new_stat;                                               \
        hm->capacity = new_cap;                                            \
    }                                                                      \
    ssize_t index = hm_find((*hm), key);                                   \
    if (index < 0) {                                                       \
        index = hash_func(hm->capacity, key);                              \
        assert(index < hm->capacity);                                      \
        while (hm->stat[index] == HASH_MAP_FULL)                           \
            index = (index + 1)%hm->capacity;                              \
        hm->count++;                                                       \
    }                                                                      \
    hm->keys[index] = hm->key_new == NULL ? key : hm->key_new(key);        \
    hm->vals[index] = hm->val_new == NULL ? val : hm->val_new(val);        \
    hm->stat[index] = HASH_MAP_FULL;                                       \
}                                                                          \
                                                                           \
val_type func_prefix##_get(struct_name hm, key_type key) {                 \
    ssize_t index = hm_find(hm, key);                                      \
    if (index < 0) return (val_type){0};                                   \
    return hm.vals[index];                                                 \
}                                                                          \
struct_name func_prefix##_new() {                                          \
    struct_name ret = {0};                                                 \
    ret.next = func_prefix##_next;                                         \
    ret.find = func_prefix##_find;                                         \
    ret.get = func_prefix##_get;                                           \
    ret.set = func_prefix##_set;                                           \
    hm_init(&ret);                                                         \
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
    ret.next = func_prefix##_next;                                         \
    ret.find = func_prefix##_find;                                         \
    ret.get = func_prefix##_get;                                           \
    ret.set = func_prefix##_set;                                           \
    ret.key_new   = key_new;                                               \
    ret.key_destr = key_destr;                                             \
    ret.val_new   = val_new;                                               \
    ret.val_destr = val_destr;                                             \
    hm_init(&ret);                                                         \
    ret.capacity = HASH_MAP_INIT_COUNT;                                    \
    return ret;                                                            \
}                                                                           

#define hm_del(hm, .../* key */) do {                                      \
    assert((hm)->capacity > 0);                                            \
    ssize_t index = hm_find(*(hm), __VA_ARGS__);                           \
    if (index < 0) break;                                                  \
    (hm)->count--;                                                         \
    memset(&(hm)->vals[index], 0, sizeof(*(hm)->vals));                    \
    memset(&(hm)->keys[index], 0, sizeof(*(hm)->keys));                    \
    (hm)->stat[index] = HASH_MAP_TOMBSTONE;                                \
} while (0)                                                                 

#define hm_get_copy(hm, ...) (                                             \
    assert((hm).val_new != NULL &&                                         \
           "hm_get_copy only available for managed hashmaps"),             \
    (hm).val_new(hm_get((hm), __VA_ARGS__))                                \
)                                                                           

#define hm_init(hm) do {                                                   \
    (hm)->keys = malloc(HASH_MAP_INIT_COUNT * sizeof(*(hm)->keys));        \
    (hm)->vals = malloc(HASH_MAP_INIT_COUNT * sizeof(*(hm)->vals));        \
    (hm)->stat = malloc(HASH_MAP_INIT_COUNT * sizeof(*(hm)->stat));        \
    memset((hm)->keys, 0, HASH_MAP_INIT_COUNT * sizeof(*(hm)->keys));      \
    memset((hm)->vals, 0, HASH_MAP_INIT_COUNT * sizeof(*(hm)->vals));      \
    memset((hm)->stat, 0, HASH_MAP_INIT_COUNT * sizeof(*(hm)->stat));      \
} while (0)                                                                 

#define hm_exists(hm, ...) ((hm).find((hm), __VA_ARGS__) >= 0)

#define hm_find(hm, ...) ((hm).find((hm), __VA_ARGS__))
#define hm_get( hm, ...) ((hm).get((hm) , __VA_ARGS__))
#define hm_set( hm, ...) ((hm)->set((hm), __VA_ARGS__))
#define hm_next(hm, ...) ((hm).next((hm), __VA_ARGS__))

#define hm_free(hm) do {                                                   \
    for (size_t i = 0; hm_next(*(hm), &i); ++i) {                          \
        if ((hm)->key_destr != NULL) (hm)->key_destr((hm)->keys[i]);       \
        if ((hm)->val_destr != NULL) (hm)->val_destr((hm)->vals[i]);       \
    }                                                                      \
    free((hm)->keys);                                                      \
    free((hm)->vals);                                                      \
    free((hm)->stat);                                                      \
    (hm)->keys = NULL;                                                     \
    (hm)->vals = NULL;                                                     \
    (hm)->stat = NULL;                                                     \
    (hm)->capacity = 0;                                                    \
    (hm)->count = 0;                                                       \
} while (0)                                                                 

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
*/
TYPED_HASH_MAP(Str2Str, str2str, char*, char*, str_hash, str_equals)

void str2str_print(Str2Str hm) {
    printf("{");
    for (size_t i = 0; str2str_next(hm, &i); ++i) {
        printf("\n    \"%s\": \"%s\",", hm.keys[i], hm.vals[i]);
    }
    printf("\b ");
    puts("\n}");
}

typedef struct {
    double bar;
    int baz;
} Foo;

TYPED_HASH_MAP(Str2Foo, str2foo, char*, Foo, str_hash, str_equals)


int main() {
    // Manual memory management
    Str2Str hm1 = str2str_new();

    hm_set(&hm1, "hello",  "world" );
    hm_set(&hm1, "mother", "fucker");
    hm_set(&hm1, "mother", "fucka" );

    // get reference
    printf("hello  = %s\n", hm_get(hm1, "hello" )); 
    printf("mother = %s\n", hm_get(hm1, "mother"));


    // Heap allocated, automatically managed
    Str2Str hm2 = str2str_new_managed(strdup, strfree, strdup, strfree);

    hm_set(&hm2, "hello",  "world" );
    hm_set(&hm2, "mother", "fucker");
    hm_set(&hm2, "mother", "fucka" );

    // get reference
    printf("hello  = %s\n", hm_get(hm2, "hello" )); 
    printf("mother = %s\n", hm_get(hm2, "mother"));

    // make a copy of value with provided constructor
    printf("hello  = %s\n", hm_get_copy(hm2, "hello" )); 
    printf("mother = %s\n", hm_get_copy(hm2, "mother"));

    hm_set(&hm2, "foo", "bar" );
    hm_set(&hm2, "let's", "go" );

    str2str_print(hm2);

    // Struct type (stored as value)
    Str2Foo hm3 = str2foo_new();

    hm_set(&hm3, "cool",   (Foo){6.9, 420});
    hm_set(&hm3, "cringe", (Foo){6.7, 67 });

    // get reference
    printf("cool   = %lf, %d\n", hm_get(hm3, "cool").bar, hm_get(hm3, "cool").baz); 
    if (hm_exists(hm3, "cringe")) {
        printf("cringe = %lf, %d\n", hm_get(hm3, "cringe").bar, hm_get(hm3, "cringe").baz);
    } else {
        puts("no cringe in this town");
    }

    hm_del(&hm3, "cringe");
    puts("deleting cringe");
    printf("cool   = %lf, %d\n", hm_get(hm3, "cool").bar, hm_get(hm3, "cool").baz); 
    if (hm_exists(hm3, "cringe")) {
        printf("cringe = %lf, %d\n", hm_get(hm3, "cringe").bar, hm_get(hm3, "cringe").baz);
    } else {
        puts("no cringe in this town");
    }

    hm_free(&hm1);
    hm_free(&hm2);
    hm_free(&hm3);
}

