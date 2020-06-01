
// called at stream stop.
//
// Some parameters get callbacks are keep across restart and some are
// not.
extern void
_qsParameterRemoveCallbacksForRestart(struct QsFilter *filter);
