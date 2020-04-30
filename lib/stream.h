

// Note: this is called FreeFilterRunResources; it does not free up all
// filter resources, just some things that could change between stop() and
// start().
//
static inline
void FreeFilterRunResources(struct QsFilter *f) {

    if(f->jobs) {

        DASSERT(f->stream);

        uint32_t numJobs = GetNumAllocJobsForFilter(f->stream, f);

        if(f->numInputs) {
            for(uint32_t i=0; i<numJobs; ++i) {

                // Free the input() arguments:
                struct QsJob *job = f->jobs + i;

                DASSERT(job->inputBuffers);
                DASSERT(job->inputLens);
                DASSERT(job->isFlushing);
                DASSERT(job->advanceLens);
#ifdef DEBUG
                memset(job->inputBuffers, 0,
                        f->numInputs*sizeof(*job->inputBuffers));
                memset(job->inputLens, 0,
                        f->numInputs*sizeof(*job->inputLens));
                memset(job->isFlushing, 0,
                        f->numInputs*sizeof(*job->isFlushing));
                memset(job->advanceLens, 0,
                        f->numInputs*sizeof(*job->advanceLens));
#endif
                free(job->inputBuffers);
                free(job->inputLens);
                free(job->isFlushing);
                free(job->advanceLens);
            }

            DASSERT(f->readers);
#ifdef DEBUG
            memset(f->readers, 0, f->numInputs*sizeof(*f->readers));
#endif
            free(f->readers);
        }


        if(f->numOutputs) {
            for(uint32_t i=0; i<numJobs; ++i) {

                // Free the input() arguments:
                struct QsJob *job = f->jobs + i;

                DASSERT(job->outputLens);
#ifdef DEBUG
                memset(job->outputLens, 0,
                        f->numOutputs*sizeof(*job->outputLens));
#endif
                free(job->outputLens);
            }
        }


#ifdef DEBUG
        memset(f->jobs, 0, numJobs*sizeof(*f->jobs));
#endif
        free(f->jobs);

        f->jobs = 0;
        f->unused = 0;
        f->numWorkingThreads = 0;
        f->workingFirst = 0;
        f->workingLast = 0;
    }

    if(f->mutex) {
        DASSERT(f->maxThreads > 1);
        DASSERT(f->stream->maxThreads > 1);
        CHECK(pthread_mutex_destroy(f->mutex));
#ifdef DEBUG
        memset(f->mutex, 0, sizeof(*f->mutex));
#endif
        free(f->mutex);
    }
#ifdef DEBUG
    else {
        DASSERT(f->stream);
        DASSERT(f->maxThreads == 1 || f->stream->maxThreads < 2);
    }
#endif

    if(f->numOutputs) {
        DASSERT(f->outputs);

        // We do not free buffers if they have not been allocated;
        // as in a qsStreamReady() failure case.
        if(f->outputs->buffer)
            FreeBuffers(f);


        // For every output in this filter we free the readers.
        for(uint32_t i=f->numOutputs-1; i!=-1; --i) {
            struct QsOutput *output = &f->outputs[i];
            DASSERT(output->numReaders);
            DASSERT(output->readers);
#ifdef DEBUG
            memset(output->readers, 0,
                    sizeof(*output->readers)*output->numReaders);
#endif
            free(output->readers);
        }

#ifdef DEBUG
        memset(f->outputs, 0, sizeof(*f->outputs)*f->numOutputs);
#endif
        free(f->outputs);
#ifdef DEBUG
        f->outputs = 0;
#endif
        f->numOutputs = 0;
        f->isSource = false;
    }

    f->numInputs = 0;
}


// Note this is call FreerRunResources; it does not free up all stream
// resources, just things that could change between stop() and start().
//
static inline
void FreeRunResources(struct QsStream *s) {

    DASSERT(s);
    DASSERT(s->app);

    for(int32_t i=0; i<s->numConnections; ++i) {
        // These can handle being called more than once per filter.
        FreeFilterRunResources(s->connections[i].from);
        FreeFilterRunResources(s->connections[i].to);
    }

    if(s->maxThreads) {

        CHECK(pthread_mutex_destroy(&s->mutex));
        CHECK(pthread_cond_destroy(&s->cond));
        CHECK(pthread_cond_destroy(&s->masterCond));
#ifdef DEBUG
        memset(&s->mutex, 0, sizeof(s->mutex));
        memset(&s->cond, 0, sizeof(s->cond));
        memset(&s->masterCond, 0, sizeof(s->masterCond));
#endif
        s->maxThreads = 0;
        s->numThreads = 0;
        s->numIdleThreads = 0;
        s->jobFirst = 0;
        s->jobLast = 0;
    }

    if(s->numSources) {
        // Free the stream sources list
#ifdef DEBUG
        memset(s->sources, 0, sizeof(*s->sources)*s->numSources);
#endif
        free(s->sources);
        s->sources = 0;
        s->numSources = 0;
    }
}
