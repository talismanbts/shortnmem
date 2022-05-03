#if !defined( FINDUTC_HEADER)
  #define FINDUTC_HEADER

/* Set types and enums used for calls and return values */

typedef enum {
    VALIDATED,
    INVALID_FORMAT,
    INVALID_YEAR,
    INVALID_MONTH,
    INVALID_DAY,
    INVALID_HOUR,
    INVALID_MINUTE,
    INVALID_SECOND,
    INVALID_TMZ,
    INVALID_REQUEST,
    INVALID_MEMORY
} code_t;

typedef enum {
    UTC8601,
    HHMMSS,
    HHMM,
    YYYYMMDD
} format_t;

typedef enum {
    DEBUG_OFF,
    DEBUG_LIB,
    DEBUG_USR
} debug_t;

typedef struct DTV_ENTRY {
    char              dtstr[26];
    int               count;
    struct DTV_ENTRY *next;
    struct DTV_ENTRY *prev;
} dtv_t;

/* function prototype declarations */

int valid_date(int format, int year, int month, int day);
int valid_time(int format, int hour, int minute, int second);
int format_match(char *dtstr, int format);
dtv_t *make_entry(char *chk_str);
int insert_or_match(dtv_t **chk_list, char *chk_str);

/* setup a DEBUG output allowing us to turn on and off DEBUG from cmd line */

void UTCLIB_DEBUG(char *a, ...);
void UTCLIB_DEBUG_SET(int lvl);

#endif
