/***---DJGPP COMPATIBILITY---***/

/***---DOS: TYPES---***/
typedef struct snailDosMemoryBlock {
	int32_t paragraphs;
	int32_t segment;
	int32_t selector;
} snailDosMemoryBlock;

// DJGPP doesn't have these.
// For now just make them do-nothing stubs.
static inline void flockfile(FILE *file) {}
static inline void funlockfile(FILE *file) {}

// getdelim/getline - source https://github.com/ivanrad/getline/blob/master/getline.c
// public domain code (UNLICENSE)
ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream) {
    char c, *cur_pos, *new_lineptr;
    size_t new_lineptr_len;

    if (lineptr == NULL || n == NULL || stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == NULL) {
        *n = 128; /* init len */
        if ((*lineptr = (char *)malloc(*n)) == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    cur_pos = *lineptr;
    for (;;) {
        c = getc(stream);

        if (ferror(stream) || (c == EOF && cur_pos == *lineptr))
            return -1;

        if (c == EOF)
            break;

        if ((*lineptr + *n - cur_pos) < 2) {
            if (SSIZE_MAX / 2 < *n) {
#ifdef EOVERFLOW
                errno = EOVERFLOW;
#else
                errno = ERANGE; /* no EOVERFLOW defined */
#endif
                return -1;
            }
            new_lineptr_len = *n * 2;

            if ((new_lineptr = (char *)realloc(*lineptr, new_lineptr_len)) == NULL) {
                errno = ENOMEM;
                return -1;
            }
            cur_pos = new_lineptr + (cur_pos - *lineptr);
            *lineptr = new_lineptr;
            *n = new_lineptr_len;
        }

        *cur_pos++ = c;

        if (c == delim)
            break;
    }

    *cur_pos = '\0';
    return (ssize_t)(cur_pos - *lineptr);
}

ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    return getdelim(lineptr, n, '\n', stream);
}
