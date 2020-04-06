#define LEAFNAMELEN  (_QS_FILTER_MAXNAMELEN + 2)


static inline
char *GetFilterLeafName(const char *filterName,
        char leafName[LEAFNAMELEN]) {
    snprintf(leafName, LEAFNAMELEN, "%s\a", filterName);
    return leafName;
}
