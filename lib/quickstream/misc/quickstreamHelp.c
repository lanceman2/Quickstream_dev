/* This file and the program that it makes has to do with command-line
 * options for the quickstream program that is installed as "quickstream"
 * in bin/.
 *
 * This program is not a quickstream user program, but it is used by the
 * quickstream program to keep the command-line options and their
 * documentation consistent by having the options and the documentation of
 * them in one place, here in this file.  Also keeping this separate from
 * the quickstream source code makes the compiled quickstream program a
 * little smaller.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>

/*  description special sequences that make HTML markup that is
 *  in the description.  Of course they mean something else for
 *  other output formats.
 *
 *    "**" = start list <ul> 
 *    "##"  = <li>
 *    "&&" = end list   </ul>
 *
 *    "  " = "&nbsp; "
 *
 *    "--*"  =  "<a href="#name">--*</a>"  where --* is a long_op
 *
 *    "::"   = "<span class=code>"  and will not add '\n' until @@
 *    "@@"   = "</span>"
 */


#define DEFAULT_MAXTHREADS      7

#define STRING(a)   _STR(a)
#define _STR(a)     #a


#define PROG   "quickstream"



static const char *usage =
"  Usage: " PROG " OPTIONS\n"
"\n"
"  Run a quickstream flow graph.\n"
"\n"
"  What filter modules to run with are given in command-line options."
" This program takes action after each command-line argument it parses,"
" so the order of command-line arguments is very important.  A connect"
" option, --connect, before you load any filters will have no effect.\n"
"\n"
"  This program executes code after parsing each command line option"
" in the order that the options are given.  After code for each command"
" line option is executed the program will terminate.\n"
"\n"
"  All command line options require an preceding option flag.  All"
" command line options with no arguments may be given in any of two"
" forms.  The two argument option forms below are equivalent:\n"
"\n"
"**"
"     ##-d\n"
"     ##--display\n"
"&&"
"\n"
"     -display is not a valid option.\n"
"\n"
"  All command line options with arguments may be given in any of three"
" forms.  The three option examples below are equivalent:\n"
"\n"
"**"
"     ##-f stdin\n"
"     ##--filter stdin\n"
"     ##--filter=stdin\n"
"&&"
"\n"
"\n"
"     -filter stdin  and  -filter=stdin  are not valid option arguments.";



// So by putting these options descriptions in this separate program we
// can generate:
//
//   1. HTML documentation,
//   2. the --help ASCII text from running 'quickstream --help', and
//   3. man pages
//
// from this one source, and they all stay consistent.
//
static struct QsOption
{
  char *long_op;
  char short_op;
  char *arg;    // like --option ARG
  bool arg_optional;
  char *description;
} opts[] = {
/*----------------------------------------------------------------------*/
    { "--connect", 'c',   "SEQUENCE",     true/*arg_optional*/,

        "connect loaded filters in a stream."
        " Loaded filters are numbered starting at zero.  For example:\n"
        "\n"
        "   --connect \"0 1 2 4\"\n"
        "\n"
        "will connect the from filter 0 to filter 1 and from "
        "filter 2 to filter 4, where filter 0 is the first "
        "loaded filter, filter 1 is the second loaded filter, "
        "and so on.  If no SEQUENCE argument is given, all "
        "filters that are not connected yet and that are "
        "currently loaded will be connected in the order that "
        "they have been loaded."
    },
/*----------------------------------------------------------------------*/
    { "--display", 'd', 0,                  false/*arg_optional*/,

        "display a graphviz dot graph of the stream.  If display is"
        " called after the stream is readied (via option --ready) this"
        " will show stream channel and buffering details."
    },
/*----------------------------------------------------------------------*/
    { "--display-wait", 'D', 0,              false/*arg_optional*/,
        
        "like --display but this waits for the display program to exit"
        "before going on to the next argument option."
    },
/*----------------------------------------------------------------------*/
    { "--filter", 'f', "FILENAME { args ... }", false,

        "load filter module with filename FILENAME.  An independent"
        " instance of the filter will be created for each time a filter"
        " is loaded and the filters will not share virtual addresses"
        " and variables.  Optional arguments passed in curly"
        " brackets (with spaces around the brackets) are passed to"
        " the module construct() function.  For example:\n"
        "\n"
        "      --filter stdin { --name input }\n"
        "\n"
        "will load the \"stdin\" filter module and pass the arguments"
        " in the brackets, --name input, to the filter module loader,"
        " whereby naming the filter \"input\"."
    },
/*----------------------------------------------------------------------*/
    { "--filter-help", 'F',  "FILENAME",        false,

        "print the filter module help to stdout and then exit."
    },
/*----------------------------------------------------------------------*/
    { "--dot", 'g', 0,                      false,

        "print a graphviz dot file of the current graph to stdout."
    },
/*----------------------------------------------------------------------*/
    { "--help", 'h', 0,                     false/*arg_optional*/,

        "print this help to stdout and then exit."
    },
/*----------------------------------------------------------------------*/
    { "--no-verbose", 'n', 0,                     false/*arg_optional*/,

        "turn off verbose printing.  See --verbose."
    },
/*----------------------------------------------------------------------*/
    { "--plug", 'p', "\"FROM_F TO_F FROM_PORT TO_PORT\"",  false,

        "connects two filters with more control over which"
        " ports get connected.  FROM_F refers to the filter we"
        " are connecting from as a number in order in which the"
        " filters are loading starting at 0.  TO_F refers to the"
        " filter we are connecting to as a number in order in"
        " which the filters are loading starting at 0.  FROM_PORT"
        " refers to the port number that we are connecting from"
        " as viewed from the \"from filter\".  TO_PORT refers to"
        " the port number that we are connecting to as viewed"
        " from the \"to filter\".  For example:\n"
        "\n"
        "     --plug \"0 1 2 3\"\n"
        "\n"
        "will feed the second filter from the first filter loaded,"
        " the feeding, first, filter will output from it's output"
        " port number 2, and the second filter will read what is"
        " fed on it's input port number 3."

    },
/*----------------------------------------------------------------------*/
    { "--ready", 'R', 0,                false,

        "ready the stream.  This calls all the filter start()"
        " functions that exist and gets the stream ready to flow,"
        " except for spawning worker flow threads."
    },
/*----------------------------------------------------------------------*/
    { "--run", 'r', 0,                  false,

        "run the stream.  This readies the stream and runs it."
    },
/*----------------------------------------------------------------------*/
    { "--threads", 't', "NUM",          false,

        "when and if the stream is launched, run at most NUM worker"
        " threads.  The default is " STRING(DEFAULT_MAXTHREADS) "."
        "  If this option is not given before a --run option this option"
        " will not effect that --run option.  On the Linux operating"
        " system the maximum number of threads a process may have can be"
        " gotten from running: cat /proc/sys/kernel/threads-max\n"
        "\n"
        "In quickstream, worker threads are shared between filters."
        " The number of threads that will run is dependent on the stream"
        " flow graph that is constructed.  Threads will only be created"
        " if when there a filter that is not starved or clogged and all"
        " the existing worker threads are busy on another filter.  Think"
        " of threads as flowing in the stream graph to where they are"
        " needed.\n"
        "\n"
        "quickstream can run with with one worker thread.  You can"
        " set the maximum number of worker threads to zero and then"
        " the main thread will run the stream (putting management to"
        " work)."
    },
/*----------------------------------------------------------------------*/
    { "--verbose", 'v', 0,                  false,

        "print more information to stderr."
    },
/*----------------------------------------------------------------------*/
    { "--version", 'V', 0,                  false/*arg_optional*/,

        "print the quickstream package version and then exit."
    },
/*----------------------------------------------------------------------*/
    { 0,0,0,0,0 }
};


// https://stackoverflow.com/questions/18783988/how-to-get-windows-size-from-linux
static int getCols(const int fd) {
    struct winsize sz;
    int result;

    do {
        result = ioctl(fd, TIOCGWINSZ, &sz);
    } while (result == -1 && errno == EINTR);

    if(result == -1)
        return 76; // default width
    return sz.ws_col;
}


static inline int NSpaces(int n, int c) {
    int ret = n;
    if(n < 1) return 0;
    while(n--) putchar(c);
    return ret;
}

// examples:  str="  hi ho"  returns 4
//            str=" hi ho"   returns 3
//            str="hi ho"    returns 2
static inline int GetNextWordLength(const char *str) {

    int n = 0;

    if(*str == '\n')
        return n;

    while(*str == ' ') {
        ++n;
        ++str;
    }
    while(*str && *str != ' ' && *str != '\n') {

        // skip special chars
        if(*str == '*' && *(str+1) == '*') {
            str += 2;
            continue;
        } else if(*str == '#' && *(str+1) == '#') {
            str += 2;
            continue;
        } else if(*str == '&' && *(str+1) == '&') {
            str += 2;
            continue;
        }

        ++n;
        ++str;
    }
    return n;
}

// types of file output
#define HTML  (0)
#define TXT   (1)


static inline int PutNextWord(const char **s, int type) {

    int n = 0;
    const char *str = *s;

    while(*str == ' ') {
        putchar(*str);
        ++n;
        ++str;
    }
    while(*str && *str != ' ' && *str != '\n') {

        // replace special chars
        if(*str == '*' && *(str+1) == '*') {
            str += 2;
            if(type == HTML) {
                printf("<ul>\n");
            }
            continue;
        } else if(*str == '#' && *(str+1) == '#') {
            str += 2;
            if(type == HTML)
                printf("  <li>");
            continue;
        } else if(*str == '&' && *(str+1) == '&') {
            str += 2;
            if(type == HTML)
                printf("</ul>");
            continue;
        }

        putchar(*str);
        ++n;
        ++str;
    }

    *s = str;

    return n;
}


static void
printHtml(const char *s, int s1, int s2) {

    while(*s) {
    
        int n = 0;
        printf("<p>\n");

        while(*s) {

            // add desc up to length s2
            while(*s && (n + GetNextWordLength(s)) <= s2) {
                if(*s == '\n') { ++s; break; }
                n += PutNextWord(&s, HTML);
            }
            printf("\n");
            n = 0;
            if(*s == '\n' || *s == '\0') {
                printf("</p>\n");
                if(*s) ++s;
                break;
            }
        }
    }
}


static void
printParagraphs(const char *s, int s1, int s2, int count) {

    int n = count;

    while(*s) {

        // add desc up to length s2
        while(*s && (n + GetNextWordLength(s)) <= s2) {
            if(*s == '\n') { ++s; break; }
            n += PutNextWord(&s, TXT);
        }

        printf("\n");

        if(*s == '\0') break;

        // Start next row.
        n = 0;
        n += NSpaces(s1, ' ');
        if(*s == ' ' && *(s+1) != ' ') ++s;
    }
}



static void
printDescription(const struct QsOption *opt, int s0, int s1, int s2) {

    const char *s = opt->description;
    int n = 0;

    // start with "    --long-arg ARG  "
    n += NSpaces(s0, ' ');
    n += printf("%s|-%c", opt->long_op, opt->short_op);
    if(opt->arg) {
        n += printf(" ");
        if(opt->arg_optional)
            n += printf("[");
        n += printf("%s", opt->arg);
        if(opt->arg_optional)
            n += printf("]");
    }
    n += printf("  ");
    n += NSpaces(s1 - n, ' ');

    printParagraphs(s, s1, s2, n);
}


int main(int argc, char **argv) {
    
    {
        ssize_t n = 2;
        if(argc > 1)
            n = strlen(argv[1]);

        if(argc < 2 || argc > 3 || n != 2 ||
                argv[1][0] != '-' || 
                (argv[1][1] != 'c' && argv[1][1] != 'h' &&
                 argv[1][1] != 'i' && argv[1][1] != 'o' &&
                 argv[1][1] != 'O')
                || argv[1][2] != '\0'
        ) {
            printf("   Usage: %s [ -c | -h | -t ]\n"
                "\n"
                "  Generate HTML, text, and C code that is related\n"
                "  to the program quickstream.\n"
                "  This program helps us keep documention and code\n"
                "  consistent, by putting the command-line options\n"
                "  documentation and code in one file.\n"
                "  Returns 0 on success and 1 on error.  This\n"
                "  program always prints to stdout.\n"
                "\n"
                " -----------------------------------------\n"
                "              OPTIONS\n"
                " -----------------------------------------\n"
                "\n"
                "    -c  print the C code of the argument options\n"
                "\n"
                "    -h  print --help text for quickstream\n"
                "\n"
                "    -i  print intro in HTML\n"
                "\n"
                "    -o  print HTML options table\n"
                "\n"
                "    -O  print all options with a space between\n"
                "\n",
                argv[0]);
            return 1;
        }
    }

    // start and stop char positions:
    static int s0 = 3;
    static int s1 = 18;
    static int s2; // tty width or default width


    struct QsOption *opt = opts;

    while(opt->description) {
        struct QsOption *opt2 = opt+1;
        while(opt2->description) {
            if(opt2->short_op == opt->short_op) {
                fprintf(stderr,
                        "ERROR there are at least two options with "
                        "short option -%c\n", opt2->short_op);
                return 1;
            }
            ++opt2;
        }
        ++opt;
    }

    opt = opts;


    switch(argv[1][1]) {

        case 'c':
            printf("// This is a generated file\n\n");
            printf("#define DEFAULT_MAXTHREADS ((uint32_t) %d)\n\n",
                    DEFAULT_MAXTHREADS);
            printf("static const\nstruct opts qsOptions[] = {\n");
            while((*opt).description) {
                printf("    { \"%s\", '%c' },\n",
                        opt->long_op + 2, opt->short_op);
                ++opt;
            }
            printf("    { 0, 0 }\n};\n");
            return 0;

        case 'h':
            s2 = getCols(STDOUT_FILENO) - 1;
            if(s2 > 200) s2 = 200;
            if(s2 < 40) s2 = 40;

            printf("\n");
            printParagraphs(usage, s0, s2, 0);
            printf("\n");

            putchar(' ');
            NSpaces(s2-1, '-');
            putchar('\n');
            NSpaces(s2/2 - 6, ' ');
            printf("OPTIONS\n");
            putchar(' ');
            NSpaces(s2-1, '-');
            printf("\n\n");

            while((*opt).description) {
                printDescription(opt, s0, s1, s2);
                printf("\n");
                ++opt;
            }
            return 0;

        case 'i':
            printHtml(usage, 4, 76);
            return 0;

        case 'o':
            printf("<pre>\n");
            s2 = 80;
            printf("\n");

            while((*opt).description) {
                printDescription(opt, s0, s1, s2);
                printf("\n");
                ++opt;
            }
            printf("</pre>\n");
            return 0;

        case 'O':

            // First long options
            printf("%s", opt->long_op);
            ++opt;
            while(opt->description) {
                printf(" %s", opt->long_op);
                ++opt;
            }
            // Next short options
            opt = opts;
            while(opt->description) {
                printf(" -%c", opt->short_op);
                ++opt;
            }
            putchar('\n');
            return 0;
     }

    // This should not happen.
    return 1;
}
