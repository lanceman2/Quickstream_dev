#ifndef __FS_H__

#define __FS_H__

// Change this FS_VERSION string to make a new version.
#define FS_VERSION "0.0.1"


extern struct Fs *fsCreate(void);
extern int fsDestroy(struct Fs *fs);
extern int fsLoad(struct Fs *fs, const char *fileName,
        const char *loadName);


#endif // #ifndef __FS_H__
