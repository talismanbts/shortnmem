/* System and standard includes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* user specific includes */

#include "UTClib.h"

/* Local defines */

#define MAX_LINE_LEN 500
#define MAX_FILE_LEN 512

enum lcl_exit_codes_l {
    SUCCESS,
    PARM_ERROR,
    PARM_MISSING,
    PARM_UNKNOWN,
    FILE_NOT_FOUND,
    MEM_ALLOC
};

enum parse_formats_l {
    TABLE_FORM,  /* a date on each line */
    TEXT_FORM,   /* dates randomly in text */
         /* possible future formats, but the above TEXT_FORM could probably handle most */
    TRIM_FORM,   /* a date with 'extraneous white space' on each line */
    COLUMN_FORM, /* a date starts on specific column of each line */
    FIELD_FORM   /* dates in fields (white space or comma separated) */
};

/*-------------------------------------------------
 * Usage:  a help/man page for the program
 */
int usage( int val, char *name )
{
    printf( "Usage: %s <-f {filename}> [-t {table|text}] [-verbose]\n", name );
    printf( "    {filename} - file to read and parse\n" );
    printf( "    table - DEFAULT setting.  Indicates the dates are 1\n" );
    printf( "            per line in file with no aditional text\n" );
    printf( "    text - indicates dates are randomly located in text\n" );
    printf( "           (this evaluation will take longer)\n" );
    printf( "    -verbose - outputs additional text during run\n" );
    printf( "             (primarily for DEBUGGING)\n" );
    printf( "  Exit values:\n" );
    printf( "    %d - parse of data for dates successful\n", SUCCESS );
    printf( "    %d - general parameter error\n", PARM_ERROR );
    printf( "    %d - required parameter missing\n", PARM_MISSING );
    printf( "    %d - unknown parameter given\n", PARM_UNKNOWN );
    printf( "    %d - source file not found\n", FILE_NOT_FOUND );
    printf( "    %d - memory allocation error during parse\n", MEM_ALLOC );
    exit( val );
}

/*-------------------------------------------------
 * cleanup:  free the allocated link list and exit with given code
 */
int cleanup( int val, dtv_t *list_entry )
{
    dtv_t * walker = NULL;

    while (list_entry != NULL)
    {
        /* save the next entry */
        walker = list_entry->next;
        free( list_entry );
        list_entry = walker;
    }
    exit( val );
}

/*-------------------------------------------------
 * Arguments
 *   specify file to read
 *   indicating what kind of file check to perform
 *     1) Validate a flat file list of dates
 *     2) Read a file and locate dates in text
 */
int main( int argc, char **argv)
{
    dtv_t *valid_list = NULL;
    dtv_t *list_walker = NULL;
    FILE  *fptr = NULL;
    char   line[MAX_LINE_LEN+1];
    char   clean_line[MAX_LINE_LEN+1];
    char   filename[MAX_FILE_LEN+1];
    int    chk_val = 0;
    int    offset = 0;
    int    loop_file = 1; /* default to on for file read */
    int    loop_line = 0;
    int    i = 0;
    int    filename_arg_found = 0;
    int    line_fill_amount = MAX_LINE_LEN;
    int    full_line_read = 1;  /* start with a full read buffer for text mode */
    int    line_read_initial = 0;  /* start with a full read buffer for text mode */
    int    parse_form = 0;      /* default TABLE format */

    memset( line, '\0', sizeof(line) );
    memset( filename, '\0', sizeof(filename) );
    memset( clean_line, '\0', sizeof(clean_line) );
    if ( (argc < 2) || (argc > 6) )
    {
        printf( "invalid number of arguments\n", argv[i] );
        usage( PARM_ERROR, argv[0] );
    }
    /* Start working through the parameters passed in */
    for (i=1; i<argc; i++)
    {
        if ( strcmp( argv[i], "-f" ) == 0 )
        {
            /* make sure we have a filename argument */
            if ( i+1 == argc )
            {
                usage( PARM_MISSING, argv[0] );
            }
            i++; /*move to next argument */
            strncpy( filename, argv[i], MAX_FILE_LEN );
            filename_arg_found = 1;
        }
        else if ( strcmp( argv[i], "-t" ) == 0 )
        {
            /* make sure we have another argument */
            if ( i+1 == argc )
            {
                usage( PARM_MISSING, argv[0] );
            }
            i++; /*move to next argument */
            if ( stricmp( argv[i], "table" ) == 0 )
            {
                parse_form = TABLE_FORM;
            }
            else if ( stricmp( argv[i], "text" ) == 0 )
            {
                parse_form = TEXT_FORM;
            }
            else
            {
                usage( PARM_ERROR, argv[0] );
            }
        }
        else if ( stricmp( argv[i], "-verbose" ) == 0 )
        {
            UTCLIB_DEBUG_SET( DEBUG_USR );
        }
        else if ( stricmp( argv[i], "-help" ) == 0 )
        {
            usage( SUCCESS, argv[0] );
        }
        else
        {
            printf( "Unknown argument [%s]\n", argv[i] );
            usage( PARM_UNKNOWN, argv[0] );
        }
    }
    if ( !filename_arg_found )
    {
        /* unable to open parse file */
        printf( "Required parameter <filename> missing!\n", filename );
        usage( PARM_MISSING, argv[0] );
    }
    fptr = fopen( filename, "r" );
    if ( fptr == NULL )
    {
        /* unable to open parse file */
        printf( "Unable to open file [%s]!\n", filename );
        cleanup( FILE_NOT_FOUND, valid_list );
    }
    /* our first read should be the maximum */
    full_line_read = 1; /* having read nothing yet, consider the 'Previous' line a full read */
    while ( loop_file )
    {
        line_read_initial = 0;
        if ( full_line_read == 1 )
        {
            line_fill_amount = MAX_LINE_LEN;
            line_read_initial = 1;
        }
        if ( fgets(line+(MAX_LINE_LEN-line_fill_amount), line_fill_amount, fptr) == NULL )
        {
            loop_file = 0;
            /* check for end of file */
            if ( feof(fptr) )
            {
                continue;
            }
            /* log error and break as well */
            printf("Read error!  Displaying Partial Results!\n" );
            continue;
        }
        UTCLIB_DEBUG("Debug: read line <%s>\n", line );
        chk_val = strlen(line) - 1;
        if ( line[ chk_val ] == '\n' )
        {
            line[ chk_val ] = '\0'; /* remove EOL */
            full_line_read = 1;     /* make sure we know this line ended */
        }
        else
        {
            full_line_read = 0;     /* this line did not end on this read */
        }
        switch (parse_form)
        {
            case TABLE_FORM:
                /* is this not the initial buffer of a (possibly) long line? */
                if ( line_read_initial == 0 )
                {
                    /* TABLE mode expects a single date per line skip the rest of long line */
                    continue;
                }
                /* parse the file 'line by line' */
                UTCLIB_DEBUG("Debug: parsing <%s>\n", line );
                chk_val = format_match( line, UTC8601 );
                if ( chk_val == VALIDATED )
                {
                    UTCLIB_DEBUG("Debug: VALIDATED <%s>\n", line );
                    /* text read was valid */
                    chk_val = insert_or_match( &valid_list, line );
                    if ( chk_val != VALIDATED )
                    {
                        /* A memory issue occured in creating our list */
                        printf( "Memory allocation error!\n" );
                        cleanup( MEM_ALLOC, valid_list );
                    }
                    UTCLIB_DEBUG("Debug: Inserted <%s>\n", line );
                }
                else
                {
                    UTCLIB_DEBUG("Debug: match fail <%d> <%s>\n",
                                  chk_val, line );
                }
                break;
            case TEXT_FORM:
                /* start at the beginning of the current line */
                offset = 0;
                loop_line = 1; /* true */
                while ( loop_line )
                {
                    memset( clean_line, '\0', sizeof(clean_line) );
                    strncpy( clean_line, line + offset, 25 );
                    UTCLIB_DEBUG("Debug: parsing <%s>\n", clean_line );
                    /* parse the file by checking for dates by character */
                    chk_val = format_match( clean_line, UTC8601 );
                    if ( chk_val == VALIDATED )
                    {
                        /* text read was valid */
                        chk_val = insert_or_match( &valid_list, clean_line );
                        if ( chk_val != VALIDATED )
                        {
                            /* A memory issue occured in creating our list */
                            printf( "Memory allocation error!\n" );
                            cleanup( MEM_ALLOC, valid_list );
                        }
                        /* move the minimum size and restart parse */
                        offset += 20;
                    }
                    else
                    {
                        /* no match, first let's make sure we have enough
                         *   text left to locate a new  date
                         * make sure if we are 1 character away from a long
                         *   form date it all gets pushed into the next buffer
                         * also make sure if we read to end of line we just finish
                         *   parsing this buffer (for a possible short form date)
                         */
                        if ( ( ((strlen(line)-offset)<25) && (full_line_read == 0) )
                        ||   ((strlen(line)-offset)<20) )
                        {
                            /* we have less than the minimum left */
                            loop_line = 0;
                        }
                        else
                        {
                            offset++; /* just 1 character */
                        }
                    }
                }
                /* is this buffer part of a (possibly) long line? */
                if ( full_line_read == 0 )
                {
                    /* save the remaining part of the long line (19 characters */
                    memset( line, '\0', sizeof(line) );
                    strncpy( line, clean_line, sizeof(line) );
                    line_fill_amount = MAX_LINE_LEN - strlen(line);
                }
                break;
            default:
                break;
        }
    }
    fclose( fptr );
    fptr  = NULL;
    list_walker = valid_list;
    printf( "The follwing Valid dates were located in the file:\n" );
    while ( list_walker != NULL )
    {
        printf ( "  Date: %s  Found %d times\n",
                 list_walker->dtstr, list_walker->count );
        list_walker = list_walker->next;
    }
    cleanup( SUCCESS, valid_list );
    return( 0 ); /* not really needed as the cleanup will exit */
}
