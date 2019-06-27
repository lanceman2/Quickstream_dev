#ifndef __FS_H__

#define __FS_H__

// Change this FS_VERSION string to make a new version.
#define FS_VERSION "0.0.1"


extern struct Fs *fsCreate(void);
extern int fsDestroy(struct Fs *fs);
/**
 * \return load index or -1 on error.  If no filter modules are unloaded
 * between fsLoad() calls the returned indexes will just be a count
 * starting at zero.
 */
extern int fsLoad(struct Fs *fs, const char *fileName,
        const char *loadName);
extern int fsConnect(


#endif // #ifndef __FS_H__
