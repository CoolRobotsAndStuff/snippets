/* tag Macros */
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

#define streq(str1, str2) (strcmp((str1), (str2)) == 0)

#define DEG2RAD (3.14/180.0)

#define randfloat() ((float)rand()/(float)RAND_MAX)

/* tag Dynamic arrays */

#define da_size(array) ((array.capacity) > 0 ? sizeof(array.items[0]) * (array.capacity) : 0)
#define da_last(array) (array.items[(array.count-1)])
#define da_last_index(array) ((array).count-1)

#ifndef DA_INIT_CAP 
#define DA_INIT_CAP 10
#endif

#define da_append(da, item)                                                      \
do {                                                                             \
    if ((da)->count >= (da)->capacity) {                                         \
        (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;   \
        (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
    }                                                                            \
    (da)->items[(da)->count++] = (item);                                         \
} while (0)

#define da_insert(da, index, item)                                               \
do {                                                                             \
    size_t __index = (index);                                                    \
    if ((da)->count >= (da)->capacity) {                                         \
        (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;   \
        (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
    }                                                                            \
    for (size_t __i = (da)->count; __i > __index; --__i) {                       \
        (da)->items[__i] = (da)->items[__i-1];                                   \
    }                                                                            \
    (da)->count += 1;                                                            \
    (da)->items[__index] = (item);                                               \
} while (0)

#define da_insert_after(da, index, item)                                                   \
do {                                                                             \
    size_t __index = (index);                                                    \
    if ((da)->count >= (da)->capacity) {                                         \
        (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;   \
        (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
    }                                                                            \
    for (size_t __i = (da)->count; __i > __index+1; --__i) {                     \
        (da)->items[__i] = (da)->items[__i-1];                                   \
    }                                                                            \
    (da)->count += 1;                                                            \
    (da)->items[__index+1] = (item);                                             \
} while (0)

#define da_delete(da, index)                               \
do {                                                       \
    size_t __index = (index);                              \
    for (size_t __i = __index; __i < (da)->count-1; ++__i) \
        (da)->items[__i] = (da)->items[__i+1];             \
    (da)->count -= 1;                                      \
} while(0)

#define da_add(da1, da2)                                                                           \
do {                                                                                                  \
    (da1)->count += (da2).count;                                                                      \
    if (((da1)->count + (da2).count) >= (da1)->capacity) {                                           \
        (da1)->capacity = (da1)->capacity == 0 ? (da2).capacity : (da1)->capacity + (da2).capacity;  \
        (da1)->items = realloc((da1)->items, (da1)->capacity*sizeof(*(da1)->items));                   \
    }                                                                                                 \
    memcpy((da1)->items + (da1)->count, (da2).items, (da2).count*sizeof(*(da2).items)); \
} while(0)

/* tag Time */

#elif defined(_WIN32)
int64_t get_time_ns() {
    static LARGE_INTEGER frequency;
    static bool initialized = false;
    if (!initialized) {
        QueryPerformanceFrequency(&frequency); 
        initialized = true;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    uint64_t time = counter.QuadPart;
    time *= 1000000000;
    time /= frequency.QuadPart;
    return (int64_t)time;
}

#elif defined(__unix__)
int64_t get_time_ns(void) {
    struct timespec ts;
    #ifdef CLOCK_MONOTONIC
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
            return ts.tv_sec * 1000000000LL + ts.tv_nsec;
        }
    #endif
    #ifdef CLOCK_REALTIME
        if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
            return ts.tv_sec * 1000000000LL + ts.tv_nsec;
        }
    #endif
    puts("Error: get_time_ns");
    return -1;
}
#endif

#ifdef __unix__
int my_usleep(long useconds) {
    return usleep(useconds);
}
int my_msleep(long mseconds) {
    return usleep(mseconds*1000);
}
int mini_sleep() {
    return usleep(1);
}
#elif defined(_WIN32)
int my_usleep(long useconds) {
    Sleep(useconds / 1000);
    return 0;
}
int my_msleep(long mseconds) {
    Sleep(mseconds);
    return 0;
}
int mini_sleep() {
    Sleep(1);
    return 0;
}
#endif

/* tag IO */

void flush_stdin() {
    for (int c=' ';  c!='\n' && c!=EOF; c=getchar()); 
}

#if defined(__unix__)
void clear_line() { printf("\r\033[K"); }

int set_stdin_nonblocking() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) return 1;
    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) return 1;
    return 0;
}

int set_stdin_blocking() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) return 1;
    if (fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK) == -1) return 1;
    return 0;
}

bool nonblocking_scanf(int* ret, const char* fmt, ...) {
    /* --- Can be ommited if set manually */
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) goto on_error;
    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) goto on_error;
    /* --- */

    va_list args;
    va_start(args, fmt);
    *ret = vscanf(fmt, args);
    va_end(args);


    /* --- Can be ommited if set manually */
    if (fcntl(STDIN_FILENO, F_SETFL, flags) == -1) goto on_error;
    /* --- */

    if (*ret < 0 && errno == EAGAIN) return false;

    return true;

    on_error:
        *ret = -1;
        return true;

}

#elif defined(_WIN32)

void clear_line() {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(console, &csbi);
    COORD beggining_of_line = {.X=0, .Y=csbi.dwCursorPosition.Y};
    DWORD _;
    FillConsoleOutputCharacter(console, ' ', csbi.dwSize.X, beggining_of_line, &_);
    SetConsoleCursorPosition(console, beggining_of_line);
}

bool nonblocking_scanf(int* ret, const char* fmt, ...) {
    #define SEQ_INPUT_EVENT_BUF_SIZE 128
    INPUT_RECORD event_buffer[SEQ_INPUT_EVENT_BUF_SIZE];

    static DWORD event_count, already_read;
    HANDLE std_input = GetStdHandle(STD_INPUT_HANDLE);
    PeekConsoleInput(std_input, event_buffer, SEQ_INPUT_EVENT_BUF_SIZE, &event_count);

    static char strbuff[1024]; //TODO: I'm not sure what to do about this
    static int strbuff_index = 0;

    for (int i = already_read; i < event_count; ++i) {
        if (event_buffer[i].EventType == KEY_EVENT && event_buffer[i].Event.KeyEvent.bKeyDown) {
            char c = event_buffer[i].Event.KeyEvent.uChar.AsciiChar;
            switch (c) {
            case 0: break;
            case '\r':
                strbuff[strbuff_index] = '\0';
                printf("\r\n");

                va_list args;
                va_start(args, fmt);
                    *ret = vsscanf(strbuff, fmt, args);
                va_end(args);

                strbuff_index = 0;
                already_read  = 0;
                if (*ret != 0) {
                    FlushConsoleInputBuffer(std_input);
                }
                return true;

            case '\b':
                strbuff_index--;
                printf("\b \b");
                break;

            default: 
                strbuff[strbuff_index++] = c;
                putchar(c);
            }
        }
    }
    already_read = event_count;
    return false;
}

#endif
