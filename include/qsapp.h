#ifndef __qsapp_h__
#define __qsapp_h__

#include <inttypes.h>
#include <stdbool.h>


// Change this QS_VERSION string to make a new version.
#define QS_VERSION "0.0.1"

/** API (application programming interface) object hierarchy
 *
 * - Qs app
 *
 * - Qs stream
 *
 * - Qs filter
 *
 * - Qs process
 *
 * - Qs thread
 */ 


/** Create the highest level quickstream construct
 * a quickstream app. */
extern
struct QsApp *qsAppCreate(void);
extern
int qsAppDestroy(struct QsApp *app);

/** Add a filter module to the list of filter modules to be loaded.
 *
 * \return 0 on success.
 */
extern
struct QsFilter *qsAppFilterLoad(struct QsApp *app,
        const char *fileName,
        const char *loadName);

extern
int qsAppPrintDotToFile(struct QsApp *app, FILE *file);


extern
int qsAppDisplayFlowImage(struct QsApp *app, bool waitForDisplay);


extern
int qsFilterUnload(struct QsFilter *filter);


extern
struct QsStream *qsAppStreamCreate(struct QsApp *app);

extern
int qsStreamDestroy(struct QsStream *stream);

extern
int qsStreamConnectFilters(struct QsStream *stream,
        struct QsFilter *from, struct QsFilter *to);

#endif // #ifndef __qsapp_h__
