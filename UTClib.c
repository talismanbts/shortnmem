/* System and standard includes */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* user specific includes */

#include "UTClib.h"

static int findUTC_debug = DEBUG_OFF;  /* default is debug output is off */

/*-------------------------------------------------------------------------
 * Debugging function allows user to set a level of DEBUG information
 *    currently allows only OFF (default 0 value) or ON (anything else)
 */
void UTCLIB_DEBUG_SET(int lvl)
{
    findUTC_debug = lvl;
}

/*-------------------------------------------------------------------------
 * Debugging function allows output to be dropped or output to console
 */
void UTCLIB_DEBUG(char *a, ...)
{
    va_list  newargs;

    if (findUTC_debug != DEBUG_OFF)
    {
        va_start( newargs, a );
        vprintf( a, newargs );
        va_end( newargs );
    }
};

/*-------------------------------------------------------------------------
 * Verify date is valid
 *     YYYY - 4 digit year (check to see if date is a leap year)
 *     MM   - 2 digit month (01-12)
 *     DD   - 2 digit day (01-31)
 *            Verify day is valid for specified month
 *            30 days max - 04, 06, 09, 11
 *                 (April, July, September, November)
 *            28 or 29 days max - 02
 *                 (February or Leap Year February)
 *            31 days max - 01, 03, 05, 07, 08, 10, 12
 *                 (April, July, September, November)
 */
int valid_date(int format, int year, int month, int day)
{
    int  days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    switch (format)
    {
        case YYYYMMDD: /* YYYY-MM-DD */
            /* NOTE: There are possibilities that multiple formats
             *       might make the same month and day checks, but
             *       for now we only have the 4 digit year format.
             *       The re-check of format allows us to then put
             *       those additional year validations here if they
             *       are small enough in format changing.
             */
            if (format == YYYYMMDD)
            {
                /* first make sure year is valid */
                if ((year < 0) || (year > 9999))
                {
                    return INVALID_YEAR;
                }
            }
            /* now make sure month is valid */
            if ((month < 1) || (month > 12))
            {
                return INVALID_MONTH;
            }
            /* check for leap year */
            if ( ( ((year % 4) == 0) && ((year % 100) != 0) )
            ||   ( ((year % 100) == 0) && ((year % 400) == 0) ) )
            {
                days[1]++; /* add 1 day for the leap year to February */
            }
            /* make sure day is valid for the month */
            if ( day > days[ month - 1 ] )
            {
                return INVALID_DAY;
            }
            break;
        default:
            return INVALID_REQUEST; /* fail on unknown format check */
    }
    return VALIDATED;
}

/*-------------------------------------------------------------------------
 * Verify time is valid
 *     hh   - 2 digit hour (00-23)
 *     mm   - 2 digit minute (00-59)
 *     ss   - 2 digit second (00-59)
 *     NOTE: Allow checking of hh:mm or hh:mm:ss formats
 *           Other formats can be added to allow for verifying
 *           12 hour am/pm times if desired at a later time
 *     NOTE: Change the switch to use an enum type list possibly
 */
int valid_time(int format, int hour, int minute, int second)
{
    switch (format)
    {
        case HHMMSS: /* hh:mm:ss */
            if ( (second > 59) || (second < 0) )
            {
                return INVALID_SECOND; /* fail on unknown format check */
            }
            /* fall through to validate the rest since it's duplicated */
        case HHMM: /* hh:mm */
            if ( (minute > 59) || (minute < 0) )
            {
                return INVALID_MINUTE; /* fail on unknown format check */
            }
            if ( (hour > 23) || (hour < 0) )
            {
                return INVALID_HOUR; /* fail on unknown format check */
            }
            break;
        default:
            return INVALID_REQUEST; /* fail on unknown format check */
    }
    return VALIDATED; /* fail on unknown format check */
}

/*-------------------------------------------------------------------------
 * Parse for format YYYY-MM-DDThh:mm:ssTZD  (Presumed to be UTC-8601)
 *     YYYY - 4 digit year
 *     MM   - 2 digit month (01-12)
 *     DD   - 2 digit day (01-31)
 *     hh   - 2 digit hour (00-23)
 *     mm   - 2 digit minute (00-59)
 *     ss   - 2 digit second (00-59)
 *     TZD  - time zone (Z or +hh:mm or -hh:mm)
 *            Note: The presumption is that various additional formats
 *                  that are allowed by ISO 8601 are not going to be
 *                  considered valid for this example.  Either time is
 *                  indicated as Z for Zulu time, or the time modification
 *                  for UTC is 4 character separated format is allowed
 */
int format_match(char *dtstr, int format)
{
    int  i;
    int  chkret = VALIDATED;
    int  chklen = 0;
    int  chkyr  = 0;
    int  chkmon = 0;
    int  chkday = 0;
    int  chkhr  = 0;
    int  chkmin = 0;
    int  chksec = 0;
    int  chktzh = 0;
    int  chktzm = 0;
    char chkval[5] = { 0 };
    int  chk8601[18] = { 0,  1,  2,  3,  5,  6,  8 , 9, 11,
                        12, 14, 15, 17, 18, 20, 21, 23, 24 };

    switch (format)
    {
        case UTC8601:
            chklen = strlen(dtstr);
            /* first make sure the required format requirements are met */
            if ( (chklen < 20 ) /* too short to have all information */
            ||   (chklen > 25 ) /* too much data for the above fields */
            ||   (dtstr[4] != '-' )
            ||   (dtstr[7] != '-' )
            ||   (dtstr[10] != 'T' )
            ||   (dtstr[13] != ':' )
            ||   (dtstr[16] != ':' ) )
            {
                return INVALID_FORMAT; /* fail on unknown format check */
            }
            /* we can check the format for TZD */
            if ( (chklen == 20 ) /* TZD can only be Z */
            &&   (dtstr[19] != 'Z' ) )
            {
                UTCLIB_DEBUG( "Debug: tmz len 20 val [%c]\n", dtstr[19] );
                return INVALID_TMZ; /* fail on TZD not valid */
            }
            if (dtstr[19] == 'Z' )
            {
                /* We have a correct TMZ so shorten the chklen
                 * and NULL terminate the string after the Z
                */
                UTCLIB_DEBUG( "Debug: tmz len [%d] val [%c] TRIMMING!\n",
                              chklen, dtstr[19] );
                dtstr[20] = '\0';
                chklen = 20;
            }
            if ( (chklen == 25 ) /* TZD must be +hh:mm or -hh:mm */
            &&   ( (dtstr[22] != ':' )
            ||     (dtstr[19] != '-' ) && (dtstr[19] != '+' ) ) )
            {
                UTCLIB_DEBUG( "Debug: tmz len 25 vals [%c][%c]\n",
                               dtstr[19], dtstr[22] );
                return INVALID_TMZ; /* fail on TZD not valid */
            }
            if ( (chklen != 20 )    /* TZD = Z */
            &&   (chklen != 25 ) )  /* TZD = +hh:mm or -hh:mm */
            {
                UTCLIB_DEBUG( "Debug: tmz len [%d]\n", chklen );
                return INVALID_TMZ; /* fail on TZD not valid */
            }
            /* at this point the string is correct size and minimal format
             * now verify numeric values and save them off to validate
             */
            for( i=0; i<14; i++ )
            {
                if ( isdigit( dtstr[ chk8601[i] ] ) == 0 )
                {
                    UTCLIB_DEBUG( "Debug: non-numeric pos [%d][%c]\n",
                                    i, dtstr[ chk8601[i] ] );
                    return INVALID_FORMAT; /* fail on TZD not valid */
                }
            }
            if (chklen == 25 ) /* TZD must be +hh:mm or -hh:mm */
            {
                for( i=15; i<18; i++ )
                {
                    if ( isdigit( dtstr[ chk8601[i] ] ) == 0 )
                    {
                        return INVALID_TMZ; /* fail on TZD not valid */
                    }
                }
            }
            strncpy( chkval, dtstr, 4 );
            chkyr = atoi( chkval );
            UTCLIB_DEBUG( "Debug: str [%s] val [%d]\n", chkval, chkyr );
            memset( chkval, '\0', sizeof(chkval) );
            strncpy( chkval, dtstr+5, 2 );
            chkmon = atoi( chkval );
            UTCLIB_DEBUG( "Debug: str [%s] val [%d]\n", chkval, chkmon );
            memset( chkval, '\0', sizeof(chkval) );
            strncpy( chkval, dtstr+8, 2 );
            chkday = atoi( chkval );
            UTCLIB_DEBUG( "Debug: str [%s] val [%d]\n", chkval, chkday );
            memset( chkval, '\0', sizeof(chkval) );
            chkret = valid_date(YYYYMMDD, chkyr, chkmon, chkday);
            if ( chkret != VALIDATED)
            {
                return chkret;
            }
            strncpy( chkval, dtstr+11, 2 );
            chkhr = atoi( chkval );
            UTCLIB_DEBUG( "Debug: str [%s] val [%d]\n", chkval, chkhr );
            memset( chkval, '\0', sizeof(chkval) );
            strncpy( chkval, dtstr+14, 2 );
            chkmin = atoi( chkval );
            UTCLIB_DEBUG( "Debug: str [%s] val [%d]\n", chkval, chkmin );
            memset( chkval, '\0', sizeof(chkval) );
            strncpy( chkval, dtstr+17, 2 );
            chksec = atoi( chkval );
            UTCLIB_DEBUG( "Debug: str [%s] val [%d]\n", chkval, chksec );
            memset( chkval, '\0', sizeof(chkval) );
            chkret = valid_time(HHMMSS, chkhr, chkmin, chksec);
            if ( chkret != VALIDATED)
            {
                return chkret;
            }
            if (chklen == 25 )  /* TZD = +hh:mm or -hh:mm */
            {
                strncpy( chkval, dtstr+20, 2 );
                chktzh = atoi( chkval );
                UTCLIB_DEBUG( "Debug: str [%s] val [%d]\n", chkval, chktzh );
                memset( chkval, '\0', sizeof(chkval) );
                strncpy( chkval, dtstr+23, 2 );
                chktzm = atoi( chkval );
                UTCLIB_DEBUG( "Debug: str [%s] val [%d]\n", chkval, chktzm );
                memset( chkval, '\0', sizeof(chkval) );
                chkret = valid_time(HHMM, chktzh, chktzm, 0);
                if ( chkret != VALIDATED)
                {
                    return INVALID_TMZ;
                }
            }
            break;
        default:
            return INVALID_FORMAT; /* fail on unknown format check */
    }
    return VALIDATED; /* check */
}


/*-------------------------------------------------------------------------
 * Evaluate the given linked list for a string match
 *     If we find an existing match update the count of that date
 *     If we don't find a match allocate a new entry and insert it
 */
dtv_t *make_entry(char *chk_str)
{
    dtv_t *blank = NULL;

    blank = calloc( 1, sizeof( dtv_t ) );
    if (blank != NULL)
    {
        strncpy(blank->dtstr, chk_str, 25);
        blank->count = 1;
    }
    return blank;
}

/*-------------------------------------------------------------------------
 * Evaluate the given linked list for a string match
 *     If we find an existing match update the count of that date
 *     If we don't find a match allocate a new entry and insert it
 */
int insert_or_match(dtv_t **chk_list, char *chk_str)
{
    int  chk_val = 0;
    dtv_t *list_walker = *chk_list;
    dtv_t *new_entry = NULL;

    /* is the list empty? */
    if (list_walker == NULL)
    {
        /* create the first entry in the list and set the new list head */
        new_entry = make_entry(chk_str);
        if (new_entry == NULL)
        {
            return INVALID_MEMORY;
        }
        *chk_list = new_entry;
        return VALIDATED;
    }
    while (list_walker != NULL)
    {
        /* start walking the list */
        chk_val = strcmp(chk_str, list_walker->dtstr );
        if (chk_val == 0)
        {
            /* Match Found! increase the count of the current entry */
            list_walker->count++;
            return VALIDATED;
        }
        if (chk_val < 0)
        {
            /* Value needs to be inserted prior to current entry */
            new_entry = make_entry(chk_str);
            if (new_entry == NULL)
            {
                return INVALID_MEMORY;
            }
            new_entry->next = list_walker;
            new_entry->prev = list_walker->prev;
            list_walker->prev = new_entry;
            if (list_walker == *chk_list)
            {
                /* set the new list head */
                *chk_list = new_entry;
            }
            else
            {
                /* update the prev ptr to add new_entry */
                new_entry->prev->next = new_entry;
            }
            return VALIDATED;
        }
        if (chk_val > 0)
        {
            /* see if we are at the end of the list */
            if (list_walker->next == NULL)
            {
                /* append the new entry to the list end */
                new_entry = make_entry(chk_str);
                if (new_entry == NULL)
                {
                    return INVALID_MEMORY;
                }
                list_walker->next = new_entry;
                new_entry->prev = list_walker;
                return VALIDATED;
            }
            /* move to the next list entry */
            list_walker = list_walker->next;
        }
    }
}
