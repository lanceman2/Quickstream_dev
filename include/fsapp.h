#ifndef __fsapp_h__
#define __fsapp_h__

// Change this FS_VERSION string to make a new version.
#define FS_VERSION "0.0.1"

/** API (application programming interface) object hierarchy
 *
 * - Fs app
 *
 * - Fs stream
 *
 * - Fs filter
 *
 * - Fs process
 *
 * - Fs thread
 */ 


/** Create the highest level faststream construct
 * a faststream app. */
extern
struct FsApp *fsAppCreate(void);
extern
int fsAppDestroy(struct FsApp *app);

/** Add a filter module to the list of filter modules to be loaded.
 *
 * \return 0 on success.
 */
extern
struct FsFilter *fsAppFilterLoad(struct FsApp *app,
        const char *fileName,
        const char *loadName);

extern
int fsFilterUnload(struct FsFilter *f, const char *loadName);


extern
struct FsStream fsAppCreateStream(struct FsApp *app);


#endif // #ifndef __fsapp_h__
