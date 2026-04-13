/* Rather large snippet meant to be copy-pasted directly into your code.
 * Example usage at the end of the file.
 * This is Public Domain.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#define HASHMAP_EMPTY 0
#define HASHMAP_FULL 1
#define HASHMAP_TOMBSTONE 2
#define HASHMAP_MAX_FILL_PERCENT 80
#define HASHMAP_INIT_CAPACITY 128

typedef struct {
    char* keys;
    char* vals;
    char*  stat;
    size_t capacity;
    size_t count;
    bool (*eq)(void*, void*);
    uint32_t (*hash)(size_t cap, void*);
} GenericHashmap;

#define Hashmap(key_t, val_t) struct { \
    key_t* keys;               \
    val_t* vals;               \
    char*  stat;               \
    size_t capacity;           \
    size_t count;              \
    bool (*eq)(key_t*, key_t*);\
    uint32_t (*hash)(size_t cap, key_t*);\
    key_t tmp_key;             \
    val_t tmp_val;             \
    ssize_t tmp_idx;           \
}

bool hm__next(void* hm_, size_t* i) {
    GenericHashmap* hm = hm_;
    for (; (*i) < hm->capacity; (*i)++) {
        if (hm->stat[*i] == HASHMAP_FULL) return true;
    }
    return false;
}

#define hm_next(hm, i) hm__next(&(hm), i)

bool hm__each(void* hm_, size_t* i, void* key, size_t key_size, void* val, size_t val_size) {
    GenericHashmap* hm = hm_;
    for (; (*i) < hm->capacity; (*i)++) {
        if (hm->stat[*i] == HASHMAP_FULL) {
            memcpy(val, &hm->vals[(*i)*val_size], val_size);
            memcpy(key, &hm->keys[(*i)*key_size], key_size);
            return true;
        }
    }
    return false;
}

#define hm_each(hm, key_ptr, val_ptr) (size_t hm__i = 0; hm__each(&(hm), &hm__i, key_ptr, sizeof((hm).tmp_key), val_ptr, sizeof((hm).tmp_val)); hm__i++)

void hm__grow_if_needed(void* hm_, size_t key_size, size_t val_size) {
    GenericHashmap* hm = hm_;
    if (!hm->capacity == 0 && !(hm->count >= (hm->capacity*HASHMAP_MAX_FILL_PERCENT/100))) return;

    size_t new_cap = (hm->capacity == 0 ? HASHMAP_INIT_CAPACITY : hm->capacity*2);
    char* new_keys = malloc(new_cap * key_size);
    char* new_vals = malloc(new_cap * val_size);
    char* new_stat = malloc(new_cap);

    memset(new_keys, 0, new_cap * key_size);
    memset(new_vals, 0, new_cap * val_size);
    memset(new_stat, 0, new_cap);

    for (size_t i = 0; hm__next(hm, &i); ++i) {
        size_t new_index = hm->hash(new_cap, &hm->keys[i*key_size]);
        assert(new_index < new_cap);
        while (new_stat[new_index] == HASHMAP_FULL)
            new_index = (new_index + 1)%(new_cap);
        memcpy(&new_keys[new_index*key_size], &hm->keys[i*key_size], key_size);
        memcpy(&new_vals[new_index*val_size], &hm->vals[i*val_size], val_size);
        new_stat[new_index] = HASHMAP_FULL;
    }
    if (hm->capacity > 0) {
        free(hm->keys);
        free(hm->vals);
        free(hm->stat);
    }
    hm->keys = new_keys;
    hm->vals = new_vals;
    hm->stat = new_stat;
    hm->capacity = new_cap;
}

ssize_t hm__find(void* hm_, void* key, size_t key_size) {
    GenericHashmap* hm = hm_;
    if (hm->capacity <= 0) return -1;
    size_t index = hm->hash(hm->capacity, key);
    size_t init_index = index;
    assert(index < hm->capacity);
    while (1) switch (hm->stat[index]) {
        case HASHMAP_EMPTY: return -1;
        case HASHMAP_FULL:
            if (hm->eq(&hm->keys[index*key_size], key)) {
                return index;
            }
        case HASHMAP_TOMBSTONE:
            index = (index + 1)%hm->capacity;
            if (index == init_index) return -1;
            break;
        default: assert(0 && "UNREACHABLE: invalid status.");
    }
}

#define hm_find(hm, key) hm__find(&(hm), ((hm).tmp_key = (key), &(hm).tmp_key), sizeof((hm).tmp_key))

void hm__set(void* hm_, void* key, size_t key_size, void* val, size_t val_size) {
    GenericHashmap* hm = hm_;
    hm__grow_if_needed(hm, key_size, val_size);
    ssize_t index = hm__find(hm, key, key_size);
    if (index < 0) {
        assert(hm->count < hm->capacity && "Exceeded hashmap capacity");
        index = hm->hash(hm->capacity, key);
        assert(index < hm->capacity);
        while (hm->stat[index] == HASHMAP_FULL)
            index = (index + 1)%hm->capacity;
        hm->count++;
    }
    memcpy(&hm->keys[index*key_size], key, key_size);
    memcpy(&hm->vals[index*val_size], val, val_size);
    hm->stat[index] = HASHMAP_FULL;
}

#define hm_set(hm, key, ...) do { \
    char hm__tmp_key[sizeof((hm)->tmp_key)] = {0}; \
    (hm)->tmp_key = (key); \
    memcpy(&hm__tmp_key, &(hm)->tmp_key, sizeof((hm)->tmp_key)); \
    char hm__tmp_val[sizeof((hm)->tmp_val)] = {0}; \
    (hm)->tmp_val = (__VA_ARGS__); \
    memcpy(&hm__tmp_val, &(hm)->tmp_val, sizeof((hm)->tmp_val)); \
    hm__set((hm), &hm__tmp_key, sizeof((hm)->tmp_key), &hm__tmp_val, sizeof((hm)->tmp_val)); \
} while(0)

#define hm_get(hm, ...) (((hm).tmp_idx = hm_find((hm), (__VA_ARGS__))) >= 0 ? (hm).vals[(hm).tmp_idx] : (abort(), (hm).vals[0]))

#define hm_check_get(hm, key, val) (              \
    ((hm).tmp_idx = hm_find((hm), (key))) >= 0 ? ( \
        *val = (hm).vals[(hm).tmp_idx],             \
        1                                            \
    ) : (                                             \
        0                                              \
    )                                                   \
)


void hm__del(void* hm_, void* key, size_t key_size, size_t val_size) {
    GenericHashmap* hm = hm_;
    assert(hm->capacity > 0);
    ssize_t index = hm__find(hm, key, key_size);
    if (index < 0) return;
    hm->count--;
    memset(&hm->keys[index*key_size], 0, sizeof(key_size));
    memset(&hm->vals[index*val_size], 0, sizeof(val_size));
    hm->stat[index] = HASHMAP_TOMBSTONE;
}

#define hm_del(hm, key) hm__del((hm), ((hm)->tmp_key = (key), &(hm)->tmp_key), sizeof((hm)->tmp_key), sizeof((hm)->tmp_val))

uint32_t FNV_1a(void *key, int length) {
    uint8_t *bytes = (uint8_t*)key;
    uint32_t hash = 2166136261u;

    for (int i = 0; i < length; i++) {
        hash ^= bytes[i];
        hash *= 16777619;
    }
    return hash;
}

uint32_t str_hash(size_t capacity, char** data) { return FNV_1a(*data, strlen(*data))%capacity; }
bool str_equals(char** data1, char** data2) { return strcmp(*data1, *data2) == 0; }

/* -- EXAMPLE USAGE -- */

typedef Hashmap(char*, char*) Str2Str;

Str2Str str2str() {
    return (Str2Str) {
        .hash = str_hash,
        .eq = str_equals
    };
}

typedef struct {
    double bar;
    int baz;
} Foo;

typedef Hashmap(char*, Foo) Str2Foo;

Str2Foo str2foo() {
    return (Str2Foo) {
        .hash = str_hash,
        .eq = str_equals
    };
}

int main() {
    Str2Str hm = str2str();

    hm_set(&hm, "hello",  "world" );
    hm_set(&hm, "mother", "fucker");
    hm_set(&hm, "mother", "fucka" );

    hm_set(&hm, "bye", hm_get(hm, "hello"));
    hm_set(&hm, hm_get(hm, "hello"), hm_get(hm, "hello"));

    printf("{\n");
    char *key, *val; 
    for hm_each(hm, &key, &val)
        printf("    %s: %s,\n", key, val);
    printf("}\n");

    printf("hello  = %s\n", hm_get(hm, "hello" )); 
    printf("mother = %s\n", hm_get(hm, "mother"));

    hm_set(&hm, "cringe", "67");
    char* value;
    if (hm_check_get(hm, "cringe", &value)) {
        printf("cringe = %s\n", value);
    } else {
        puts("no cringe in this town");
    }

    puts("deleting cringe");
    hm_del(&hm, "cringe");

    if (hm_check_get(hm, "cringe", &value)) {
        printf("cringe = %s\n", value);
    } else {
        puts("no cringe in this town");
    }

    printf("{\n");
    for hm_each(hm, &key, &val)
        printf("    %s: %s,\n", key, val);
    printf("}\n");

    puts("---------------------");

    // Struct type (stored as value)
    Str2Foo hm2 = str2foo();

    hm_set(&hm2, "cool",   (Foo){6.9, 420});
    hm_set(&hm2, "cringe", (Foo){6.7, 67 });

    printf("{\n");
    char *foo_key;
    Foo foo;
    for hm_each(hm2, &foo_key, &foo)
        printf("    %s: { bar: %f, baz: %d}, \n", foo_key, foo.bar, foo.baz);
    printf("}\n");

    // get reference
    printf("cool   = %lf, %d\n", hm_get(hm2, "cool").bar, hm_get(hm2, "cool").baz); 
    if (hm_check_get(hm2, "cringe", &foo)) {
        printf("cringe = %lf, %d\n", foo.bar, foo.baz);
    } else {
        puts("no cringe in this town");
    }

    puts("deleting cringe");
    hm_del(&hm2, "cringe");

    printf("cool   = %lf, %d\n", hm_get(hm2, "cool").bar, hm_get(hm2, "cool").baz); 
    if (hm_check_get(hm2, "cringe", &foo)) {
        printf("cringe = %lf, %d\n", foo.bar, foo.baz);
    } else {
        puts("no cringe in this town");
    }
}
