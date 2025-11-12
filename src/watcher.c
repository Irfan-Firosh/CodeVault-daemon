#include "watcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FSEventStreamRef streamRef = NULL;
static dispatch_queue_t eventQueue = NULL;
static cv_repo_callback user_callback = NULL;

static void fs_callback(
    ConstFSEventStreamRef streamRef,
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[]
) {
    char **paths = eventPaths;
    for (size_t i = 0; i < numEvents; i++) {
        if (user_callback) user_callback(paths[i]);
    }
}

int cv_watcher_start(const char *path, cv_repo_callback cb) {
    CFStringRef cfPath = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&cfPath, 1, NULL);
    
    user_callback = cb;

    FSEventStreamContext context = {0, NULL, NULL, NULL, NULL};

    streamRef = FSEventStreamCreate(NULL, &fs_callback, &context, 
                                    pathsToWatch, kFSEventStreamEventIdSinceNow, 
                                    1.0, kFSEventStreamCreateFlagFileEvents);
    
    if (!streamRef) {
        fprintf(stderr, "Failed to create FSEventStream\n");
        CFRelease(pathsToWatch);
        CFRelease(cfPath);
        return 1;
    }

    eventQueue = dispatch_queue_create("com.codevault.fsevents", DISPATCH_QUEUE_SERIAL);
    FSEventStreamSetDispatchQueue(streamRef, eventQueue);
    FSEventStreamStart(streamRef);

    CFRelease(pathsToWatch);
    CFRelease(cfPath);

    printf("[codevault] Watching %s for changes...\n", path);
    CFRunLoopRun();
    return 0;
}

void cv_watcher_stop(void) {
    if (streamRef) {
        FSEventStreamStop(streamRef);
        FSEventStreamInvalidate(streamRef);
        FSEventStreamRelease(streamRef);
        streamRef = NULL;
    }
    if (eventQueue) {
        dispatch_release(eventQueue);
        eventQueue = NULL;
    }
    CFRunLoopStop(CFRunLoopGetCurrent());
}