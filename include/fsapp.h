#ifndef __FSAPP_H__

#define __FSAPP_H__

// Change this FS_VERSION string to make a new version.
#define FS_VERSION "0.0.1"

/** API (application programming interface) object hierarchy
 *
 * - Fs app object
 *
 * - Fss stream
 *
 * - Fsf filter
 *
 * - Fsp process
 *
 * - Fst thread
 */ 


/** Create the highest level faststream construct
 * a faststream app. */
extern
struct FsApp *fsCreateApp(void);
extern
int fsDestroy(struct Fs *fs);

/** Add a filter module to the list of filter modules to be loaded.
 *
 * \return 0 on success.
 */
extern
int fsLoadFilter(struct Fs *fs, const char *fileName, const char *loadName);
extern
int fsUnloadFilter(struct Fs *fs, const char *loadName);


extern
struct FsApp fsCreateStream(struct FsApp *app)


#endif // #ifndef __FS_H__
