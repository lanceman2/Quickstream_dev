// https://stackoverflow.com/questions/1085083/regular-expressions-in-c-examples#1085406

#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
print_regerror (int errcode, size_t length, regex_t *compiled) {
    char buffer[length];
    regerror(errcode, compiled, buffer, length);
    fprintf(stderr, "Regex match failed: %s\n", buffer);
}

static void
usage(const char *argv0) {

    fprintf(stderr,
"  Usage: %s REGEX EXP0 [ EXP1 ... ]\n"
"\n"
"   print matches to regular expression REGEX from the rest of the arguments.\n"
"\n",
    argv0);
}



int main (int argc, char *argv[]) {
    regex_t regex;
    int result;

    if (argc < 3) {
        // The number of passed arguments is lower than the number of
        // expected arguments.
        fputs("Missing command line arguments\n\n", stderr);
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    result = regcomp(&regex, argv[1], REG_EXTENDED);
    if(result) {
        // Any value different from 0 means it was not possible to 
        // compile the regular expression, either for memory problems
        // or problems with the regular expression syntax.
        if (result == REG_ESPACE)
            fprintf(stderr, "%s\n", strerror(ENOMEM));
        else
            fputs("Syntax error in the regular expression passed as first argument\n", stderr);
        return EXIT_FAILURE;               
    }
  
    for (int i = 2; i < argc; i++) {
        result = regexec(&regex, argv[i], 0, NULL, 0);
        if (!result)
            printf("'%s' matches the regular expression '%s'\n", argv[i], argv[1]);
        else if(result == REG_NOMATCH)
            printf ("'%s' doesn't the regular expression '%s'\n", argv[i], argv[1]);
        else {
            // The function returned an error; print the string 
            // describing it.
            // Get the size of the buffer required for the error message.
            size_t length = regerror (result, &regex, NULL, 0);
            print_regerror(result, length, &regex);       
            return EXIT_FAILURE;
        }
    }

    /* Free the memory allocated from regcomp(). */
    regfree(&regex);
    return EXIT_SUCCESS;
}
