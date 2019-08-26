/** \page quickstream_program quickstream 
 *
 * \brief stream runner program
 
\tableofcontents
quickstream is a command-line program that can load, connect filters, and
run the loaded filters in a flow stream

\section quickstream_args command-line arguements

\section quickstream_examples quickstream examples

\subsection quickstream_helloworld quickstream helloworld


*/



#include <stdio.h>

#include "../include/qsapp.h"

int main(void) {

    printf("hello quickstream version %s\n", QS_VERSION);

    return 0;
}
