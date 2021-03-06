/* 
 * This file is part of the pebil project.
 * 
 * Copyright (c) 2010, University of California Regents
 * All rights reserved.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _InstrumentationCommon_hpp_
#define _InstrumentationCommon_hpp_
#define _GNU_SOURCE
#include <dlfcn.h>

#ifdef HAVE_MPI
#include <mpi.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#ifdef HAVE_UNORDERED_MAP
#include <tr1/unordered_map>
#define pebil_map_type tr1::unordered_map
#else
#include <map>
#define pebil_map_type map
#endif

#include <Metasim.hpp>

using namespace std;

static pebil_map_type < uint64_t, vector < DynamicInst* > > * Dynamics = NULL;

static void PrintDynamicPoint(DynamicInst* d){
    std::cout
        << "\t"
        << "\t" << "Key 0x" << std::hex << d->Key
        << "\t" << "Vaddr 0x" << std::hex << d->VirtualAddress
        << "\t" << "Oaddr 0x" << std::hex << d->ProgramAddress
        << "\t" << "Size " << std::dec << d->Size
        << "\t" << "Enabled " << (d->IsEnabled? "yes":"no")
        << "\n";
}

static void InitializeDynamicInstrumentation(uint64_t* count, DynamicInst** dyn){
    if (Dynamics == NULL){
        Dynamics = new pebil_map_type < uint64_t, vector < DynamicInst* > > ();
    }

    DynamicInst* dd = *dyn;
    for (uint32_t i = 0; i < *count; i++){
        if (dd[i].IsEnabled){
            assert(dd[i].IsEnabled);
            uint64_t k = dd[i].Key;
            if (Dynamics->count(k) == 0){
                (*Dynamics)[k] = vector<DynamicInst*>();
            }
            (*Dynamics)[k].push_back(&dd[i]);
        }
    }
}

static void GetAllDynamicKeys(set<uint64_t>& keys){
    assert(keys.size() == 0);
    for (pebil_map_type<uint64_t, vector<DynamicInst*> >::iterator it = Dynamics->begin(); it != Dynamics->end(); it++){
        uint64_t k = (*it).first;
        keys.insert(k);
    }
}

static void SetDynamicPointStatus(DynamicInst* d, bool state){

    uint8_t t[DYNAMIC_POINT_SIZE_LIMIT];
    memcpy(t, (uint8_t*)d->VirtualAddress, d->Size);
    memcpy((uint8_t*)d->VirtualAddress, d->OppContent, d->Size);
    memcpy(d->OppContent, t, d->Size);

    d->IsEnabled = state;
    //PrintDynamicPoint(d);
}

static void SetDynamicPoints(std::set<uint64_t>& keys, bool state){
    uint32_t count = 0;
    for (pebil_map_type<uint64_t, vector<DynamicInst*> >::iterator it = Dynamics->begin(); it != Dynamics->end(); it++){
        uint64_t k = (*it).first;
        if (keys.count(k) > 0){
            vector<DynamicInst*> dyns = (*it).second;
            for (vector<DynamicInst*>::iterator dit = dyns.begin(); dit != dyns.end(); dit++){
                DynamicInst* d = (*dit);
                if (state != d->IsEnabled){
                    count++;
                    SetDynamicPointStatus(d, state);
                }
            }
        }
    }
    debug(std::cout << "Thread " << std::hex << pthread_self() << " switched " << std::dec << count << " to " << (state? "on" : "off") << std::endl);
}

// thread id support
typedef struct {
    uint64_t id;
    uint64_t data;
} ThreadData;
#define ThreadHashShift (12)
#define ThreadHashMod   (0xffff)


// handling of different initialization/finalization events
// analysis libraries define these differently
extern "C" {
    extern void* tool_mpi_init();
    extern void* tool_thread_init(pthread_t args);
    extern void* tool_thread_fini(pthread_t args);
    extern void* tool_image_init(void* s, image_key_t* key, ThreadData* td);
    extern void* tool_image_fini(image_key_t* key);
};


// some function re-naming support
#define __give_pebil_name(__fname) \
    __fname ## _pebil_wrapper

#ifdef PRELOAD_WRAPPERS
#define __wrapper_name(__fname) \
    __fname
#else // PRELOAD_WRAPPERS
#define __wrapper_name(__fname) \
    __give_pebil_name(__fname)
#endif // PRELOAD_WRAPPERS



// handle rank/process identification with/without MPI
static int taskid;
#ifdef HAVE_MPI
#define __taskid taskid
#define __ntasks ntasks
static int __ntasks = 1;
#else //HAVE_MPI
#define __taskid getpid()
#define __ntasks 1
#endif //HAVE_MPI

static int GetTaskId(){
    return __taskid;
}
static int GetNTasks(){
    return __ntasks;
}


// a timer
static void ptimer(double *tmr) {
    struct timeval timestr;
    void *tzp=0;

    gettimeofday(&timestr, (struct timezone*)tzp);
    *tmr=(double)timestr.tv_sec + 1.0E-06*(double)timestr.tv_usec;
}

// support for output/warnings/errors
#define METASIM_ID "Metasim"
#define METASIM_VERSION "3.0.0"
#define METASIM_ENV "PEBIL_ROOT"

#define TAB "\t"
#define ENDL "\n"
#define DISPLAY_ERROR cerr << "[" << METASIM_ID << "-r" << GetTaskId() << "] " << "Error: "
#define warn cerr << "[" << METASIM_ID << "-r" << GetTaskId() << "] " << "Warning: "
#define ErrorExit(__msg, __errno) DISPLAY_ERROR << __msg << endl << flush; exit(__errno);
#define inform cout << "[" << METASIM_ID << "-r" << dec << GetTaskId() << "] "
#define SAVE_STREAM_FLAGS(__s) ios_base::fmtflags ff ## __s = __s.flags()
#define RESTORE_STREAM_FLAGS(__s) __s.flags(ff ## __s)


enum MetasimErrors {
    MetasimError_None = 0,
    MetasimError_MemoryAlloc,
    MetasimError_NoThread,
    MetasimError_TooManyInsnReads,
    MetasimError_StringParse,
    MetasimError_FileOp,
    MetasimError_Env,
    MetasimError_NoImage,
    MetasimError_Total,
};

static void TryOpen(ofstream& f, const char* name){
    f.open(name);
    f.setf(ios::showbase);
    if (f.fail()){
        ErrorExit("cannot open output file: " << name, MetasimError_FileOp);
    }
}


// thread handling
extern "C" {
    const int SuspendSignal = SIGUSR2;
    uint32_t CountSuspended = 0;
    pthread_mutex_t countlock;
    pthread_mutex_t pauser;
    bool CanSuspend = false;

    void SuspendHandler(int signum){
        // increment pause counter
        pthread_mutex_lock(&countlock);
        CountSuspended++;
        pthread_mutex_unlock(&countlock);

        // the thread doing the pausing will lock this prior to asking for the pause, therefore every
        // other thread that hits this will wait on the the thread asking for the pause
        pthread_mutex_lock(&pauser);
        pthread_mutex_unlock(&pauser);

        // decrement pause counter
        pthread_mutex_lock(&countlock);
        CountSuspended--;
        pthread_mutex_unlock(&countlock);
    }

    void InitializeSuspendHandler(){
        if (CanSuspend){
            return;
        }
        debug(inform << "Thread " << hex << pthread_self() << " initializing Suspension handling" << ENDL);

        CountSuspended = 0;
        pthread_mutex_init(&pauser, NULL);
        pthread_mutex_init(&countlock, NULL);

        CanSuspend = true;

        struct sigaction NewAction, OldAction;
        NewAction.sa_handler = SuspendHandler;
        sigemptyset(&NewAction.sa_mask);
        NewAction.sa_flags = 0;
     
        sigaction(SuspendSignal, NULL, &OldAction);
        sigaction(SuspendSignal, &NewAction, NULL);
    }

    void SuspendAllThreads(uint32_t size, set<thread_key_t>::iterator b, set<thread_key_t>::iterator e){
        if (!CanSuspend){
            return;
        }

        pthread_mutex_lock(&pauser);
        assert(CountSuspended == 0);

        for (set<thread_key_t>::iterator tit = b; tit != e; tit++){
            if ((*tit) != pthread_self()){
                pthread_kill((*tit), SuspendSignal);
            }
        }

        // wait for all other threads to reach paused state
        uint32_t ksize = size - 1;
        while (CountSuspended < ksize){
            pthread_yield();
        }
        assert(CountSuspended == ksize);
    }

    void ResumeAllThreads(){
        if (!CanSuspend){
            return;
        }

        pthread_mutex_unlock(&pauser);

        // wait for all other threads to exit paused state
        while (CountSuspended > 0){
            pthread_yield();
        }
        assert(CountSuspended == 0);
    }

    typedef struct {
        void* args;
        int (*fcn)(void*);
    } thread_passthrough_args;

    int thread_started(void* args){
        thread_passthrough_args* pt_args = (thread_passthrough_args*)args;
        tool_thread_init(pthread_self());
        
        return pt_args->fcn(pt_args->args);
    }

    static int __give_pebil_name(clone)(int (*fn)(void*), void* child_stack, int flags, void* arg, ...){
        va_list ap;
        va_start(ap, arg);
        pid_t* ptid = va_arg(ap, pid_t*);
        struct user_desc* tls = va_arg(ap, struct user_desc*);
        pid_t* ctid = va_arg(ap, pid_t*);
        va_end(ap);
        /*
        printf("Entry function: 0x%llx\n", fn);
        printf("Stack location: 0x%llx\n", child_stack);
        printf("Flags: %d\n", flags);
        printf("Function args: 0x%llx\n", arg);
        printf("ptid address: 0x%llx\n", ptid);
        printf("tls address: 0x%llx\n", tls);
        printf("ctid address: 0x%llx\n", ctid);
        */    
        static int (*clone_ptr)(int (*fn)(void*), void* child_stack, int flags, void* arg, pid_t *ptid, struct user_desc *tls, pid_t *ctid)
            = (int (*)(int (*fn)(void*), void* child_stack, int flags, void* arg, pid_t *ptid, struct user_desc *tls, pid_t *ctid))dlsym(RTLD_NEXT, "clone");

        // TODO: keep this somewhere and destroy it. it currently is a mem leak
        thread_passthrough_args* pt_args = (thread_passthrough_args*)malloc(sizeof(thread_passthrough_args));
        pt_args->fcn = fn;
        pt_args->args = arg;

        return clone_ptr(thread_started, child_stack, flags, (void*)pt_args, ptid, tls, ctid);
    }

    int __clone(int (*fn)(void*), void* child_stack, int flags, void* arg, ...){
        va_list ap;
        va_start(ap, arg);
        pid_t* ptid = va_arg(ap, pid_t*);
        struct user_desc* tls = va_arg(ap, struct user_desc*);
        pid_t* ctid = va_arg(ap, pid_t*);
        va_end(ap);
        return __give_pebil_name(clone)(fn, child_stack, flags, arg, ptid, tls, ctid);
    }

    int clone(int (*fn)(void*), void* child_stack, int flags, void* arg, ...){
        va_list ap;
        va_start(ap, arg);
        pid_t* ptid = va_arg(ap, pid_t*);
        struct user_desc* tls = va_arg(ap, struct user_desc*);
        pid_t* ctid = va_arg(ap, pid_t*);
        va_end(ap);
        return __give_pebil_name(clone)(fn, child_stack, flags, arg, ptid, tls, ctid);
    }

    int __clone2(int (*fn)(void*), void* child_stack, int flags, void* arg, ...){
        va_list ap;
        va_start(ap, arg);
        pid_t* ptid = va_arg(ap, pid_t*);
        struct user_desc* tls = va_arg(ap, struct user_desc*);
        pid_t* ctid = va_arg(ap, pid_t*);
        va_end(ap);
        return __give_pebil_name(clone)(fn, child_stack, flags, arg, ptid, tls, ctid);
    }

    int pthread_join(pthread_t thread, void **value_ptr){
        pthread_t jthread = thread;

        int (*join_ptr)(pthread_t, void**) = (int (*)(pthread_t, void**))dlsym(RTLD_NEXT, "pthread_join");
        int ret = join_ptr(thread, value_ptr);

        tool_thread_fini(jthread);
        return ret;
    }
};


// some help geting task/process information into strings
static void AppendPidString(string& str){
    char buf[6];
    sprintf(buf, "%05d", getpid());
    buf[5] = '\0';

    str.append(buf);
}

static void AppendRankString(string& str){
    char buf[9];
    sprintf(buf, "%08d", GetTaskId());
    buf[8] = '\0';

    str.append(buf);
}

static void AppendTasksString(string& str){
    char buf[9];
    sprintf(buf, "%08d", GetNTasks());
    buf[8] = '\0';

    str.append(buf);
}


// data management support
#define DataMap std::pebil_map_type

template <class T> class DataManager {
private:
    pthread_mutex_t mutex;

    DataMap <image_key_t, DataMap<thread_key_t, T> > datamap;
    T (*datagen)(T, uint32_t, image_key_t, thread_key_t, image_key_t);
    void (*datadel)(T);
    uint64_t (*dataref)(T);

    DataMap <image_key_t, DataMap<uint32_t, double> > timers;

    uint32_t currentthreadseq;
    DataMap <thread_key_t, uint32_t> threadseq;

    uint32_t currentimageseq;
    DataMap <image_key_t, uint32_t> imageseq;

    image_key_t firstimage;

    // stores data in a ThreadData[] which can be more easily accessed by tools.
    DataMap <image_key_t, ThreadData*> threaddata;

    uint32_t HashThread(thread_key_t tid){
        return (tid >> ThreadHashShift) & ThreadHashMod;
    }

    uint64_t SetThreadData(image_key_t iid, thread_key_t tid, uint32_t typ){
        uint32_t h = HashThread(tid);

        assert(threaddata.count(iid) == 1);
        assert(datamap.count(iid) == 1);
        assert(datamap[iid].count(tid) == 1);

        ThreadData* td = threaddata[iid];

        uint32_t actual = h;
        
        while (td[actual].id != 0){
            actual = (actual + 1) % (ThreadHashMod + 1);
        }
        T d = datamap[iid][tid];
        td[actual].id = (uint64_t)tid;
        td[actual].data = (uint64_t)dataref(d);

        if (typ == ImageType){
            inform << "Image " << hex << iid << " thread " << tid;
        } else {
            inform << "Thread " << hex << tid << " image " << iid;
        }
        cout
            << " setting up thread data at index "
            << dec << actual << TAB << (uint64_t)td
            << " -> " << hex << td[actual].data
            << endl;

        // just fail if there was a collision. it makes writing tools much easier so we see how well this works for now
        if (actual != h){
            warn << "Collision placing thread-specific data: slot " << dec << h << " already taken" << ENDL;
        }
        assert(actual == h);
        return td[actual].data;
    }
    void RemoveThreadData(image_key_t iid, thread_key_t tid){
        uint32_t h = HashThread(tid);

        assert(threaddata.count(iid) == 1);
        assert(datamap.count(iid) == 1);
        assert(datamap[iid].count(tid) == 1);

        ThreadData* td = threaddata[iid];

        uint32_t actual = h;
        while (td[actual].id != tid){
            actual = (actual + 1) % (ThreadHashMod + 1);
        }
        td[actual].id = 0;
        td[actual].data = 0;
    }

public:

    set<thread_key_t> allthreads;
    set<thread_key_t> donethreads;
    set<image_key_t> allimages;

    static const uint32_t ThreadType = 0;
    static const uint32_t ImageType = 1;

    DataManager(T (*g)(T, uint32_t, image_key_t, thread_key_t, image_key_t), void (*d)(T), uint64_t (*r)(T)){
        datagen = g;
        datadel = d;
        dataref = r;

        pthread_mutex_init(&mutex, NULL);
        currentthreadseq = 0;
        threadseq[GenerateThreadKey()] = currentthreadseq++;

        currentimageseq = 0;
        firstimage = 0;
    }

    ~DataManager(){
    }

    bool Lock(){
        bool res = (pthread_mutex_lock(&mutex) == 0);
        return res;
    }

    bool UnLock(){
        bool res = (pthread_mutex_unlock(&mutex) == 0);
        return res;
    }

    // these can only be called correctly by the current thread
    thread_key_t GenerateThreadKey(){
        return pthread_self();
    }

    uint32_t GetThreadSequence(thread_key_t tid){
        if (threadseq.count(tid) != 1){
            inform << "Thread not available!?! " << hex << tid << ENDL;
        }
        assert(threadseq.count(tid) == 1 && "thread must be added with AddThread method");
        return threadseq[tid];
    }

    uint32_t GetImageSequence(image_key_t iid){
        assert(imageseq.count(iid) == 1 && "image must be added with AddImage method");
        return imageseq[iid];
    }

    void FinishThread(thread_key_t tid){
        assert(donethreads.count(tid) == 0);
        donethreads.insert(tid);
    }

    bool ThreadLives(thread_key_t tid){
        if (allthreads.count(tid) == 0){
            return false;
        }
        if (donethreads.count(tid) > 0){
            return false;
        }
        return true;
    }

    void AddThread(thread_key_t tid){
        Lock(); // FIXME necessary?
        assert(allthreads.count(tid) == 0);
        assert(threadseq.count(tid) == 0);
        assert(firstimage != 0);

        threadseq[tid] = currentthreadseq++;

        for (set<image_key_t>::iterator iit = allimages.begin(); iit != allimages.end(); iit++){
            assert(datamap[(*iit)].size() > 0);
            assert(datamap[(*iit)].count(tid) == 0);

            if(allthreads.size() > 0) {
                set<thread_key_t>::iterator tit = allthreads.begin();
                assert(tit != allthreads.end());
                datamap[*iit][tid] = datagen(datamap[*iit][*tit], ThreadType, *iit, tid, firstimage);
                assert(NULL != datamap[*iit][tid]);
            }
            /*
             * tit = allthreads.begin()
             * datamap[image][new_thread] = datagen(datamap[image][old_thread], ThreadType, image, new_thread, firstimage)
             * assert(wasn't null);
            for (set<thread_key_t>::iterator tit = allthreads.begin(); tit != allthreads.end(); tit++){
	        datamap[(*iit)][tid] = datagen(datamap[(*iit)][(*tit)], ThreadType, (*iit), tid, firstimage);
                assert(datamap[(*iit)][tid]);
                break;
            }
            */

            assert(threaddata.count(*iit) == 1);
            SetThreadData((*iit), tid, ThreadType);
        }
        allthreads.insert(tid);
        UnLock();
    }

    void AddThread(){
        AddThread(pthread_self());
    }

    void RemoveData(image_key_t iid, thread_key_t tid){
        assert(datamap.count(iid) == 1);
        assert(datamap[iid].count(tid) == 1);

        T data = datamap[iid][tid];
        datadel(data);
        datamap[iid].erase(tid);
    }

    void RemoveThread(){
        assert(false);
        thread_key_t tid = pthread_self();
        assert(allthreads.count(tid) == 1);

        for (set<image_key_t>::iterator iit = allimages.begin(); iit != allimages.end(); iit++){
            assert(datamap[(*iit)].size() > 0);
            assert(datamap[(*iit)].count(tid) == 1);
            RemoveData((*iit), tid);
            RemoveThreadData((*iit), tid);
        }
        allthreads.erase(tid);
    }

    void SetTimer(image_key_t iid, uint32_t idx){
        double t;
        ptimer(&t);

        if (timers.count(iid) == 0){
            timers[iid] = DataMap<uint32_t, double>();
        }
        assert(timers.count(iid) == 1);
        timers[iid][idx] = t;
    }

    double GetTimer(image_key_t iid, uint32_t idx){
        assert(timers.count(iid) == 1);
        assert(timers[iid].count(idx) == 1);
        double t = timers[iid][idx];
        return t;
    }

    image_key_t AddImage(T data, ThreadData* t, image_key_t iid){
        Lock(); // FIXME necessary?
        thread_key_t tid = pthread_self();

        assert(allimages.count(iid) == 0);
        imageseq[iid] = currentimageseq++;

        // insert data for this thread
        allthreads.insert(tid);
        allimages.insert(iid);

        datamap[iid] = DataMap<thread_key_t, T>();
        assert(datamap.count(iid) == 1);
        datamap[iid][tid] = data;

        threaddata[iid] = t;
        uint64_t dataloc = SetThreadData(iid, tid, ImageType);
        if (allthreads.size() == 1){
            for (uint32_t i = 0; i < ThreadHashMod + 1; i++){
                t[i].data = dataloc;
            }
        }

        if (firstimage == 0){
            firstimage = iid;
        }

        // create/insert data every thread
        assert(datamap.count(iid) == 1);
        for (set<thread_key_t>::iterator it = allthreads.begin(); it != allthreads.end(); it++){
            datamap[iid][(*it)] = datagen(data, ImageType, iid, (*it), firstimage);
            assert(datamap[iid].count((*it)) == 1);
        }
        UnLock();
        return iid;
    }

    // Return data for any image and this thread.
    // Probably best used when only one image exists.
    T GetData(){
        return GetData(pthread_self());
    }

    T GetData(thread_key_t tid){
        set<image_key_t>::iterator iit = allimages.begin();
        assert(iit != allimages.end());
        return GetData(*iit, tid);
    }

    T GetData(image_key_t iid, thread_key_t tid){
        if (datamap.count(iid) != 1){
            inform << "About to fail iid check with " << dec << datamap.count(iid) << ENDL;
        }
        assert(datamap.count(iid) == 1);
        assert(datamap[iid].count(tid) == 1);
        T t = datamap[iid][tid];
        return t;
    }

    uint32_t CountThreads(){
        return allthreads.size();
    }
    uint32_t CountImages(){
        return allimages.size();
    }
};

template <class T, class V> class FastData {
private:
    uint32_t threadcount;
    uint32_t imagecount;
    uint32_t capacity;
    DataManager<T>* alldata;
    void (*dataid)(V, image_key_t*);

    // [i][j]
    // i == thread id
    // j == buffer index
    T** stats;
    pthread_mutex_t lock;

    void Lock(){ pthread_mutex_lock(&lock); }
    void UnLock(){ pthread_mutex_unlock(&lock); }

public:
    FastData(void (*di)(V, image_key_t*), DataManager<T>* all, uint32_t cap){
        dataid = di;

        threadcount = 1;
        imagecount = 0;

        capacity = cap;

        alldata = all;
        assert(alldata->CountThreads() == 1);
        assert(alldata->CountImages() == 1);
        assert(alldata->GetThreadSequence(pthread_self()) == 0);

        stats = new T*[threadcount];
        stats[0] = new T[capacity];
        T dat = alldata->GetData();
        for (uint32_t i = 0; i < capacity; i++){
            stats[0][i] = dat;
        }

        pthread_mutex_init(&countlock, NULL);
    }

    ~FastData(){
        if (stats){
            for (uint32_t i = 0; i < threadcount; i++){
                delete[] stats[i];
            }
            delete[] stats;
        }
    }

    // tid must have already been added to alldata
    // copy data from old stats and initialize new thread data
    void AddThread(thread_key_t tid){
        Lock();
        uint32_t tid_index = alldata->GetThreadSequence(tid);
        uint32_t newsize = tid_index+1 > threadcount ? tid_index+1 : threadcount;

//        inform << "Adding new thread with index " << tid_index << " and new size will be " << newsize << ENDL;
//        inform << "Old size was " << threadcount << ENDL;

        // copy old data
        T** tmp = new T*[newsize];
        for (uint32_t i = 0; i < threadcount; ++i){
            tmp[i] = stats[i];
        }
//        inform << "Copied old data" << ENDL;

        // initialize new data
        for(uint32_t i = threadcount; i < newsize; ++i){
            tmp[i] = new T[capacity];
            if(i != tid_index)
                memset(tmp[i], 0, sizeof(T) * capacity);
        }
//        inform << "Initialized new data" << ENDL;

        // if only one image, can't count on it being refreshed later
        if(imagecount == 1) {
            T dat = alldata->GetData(tid);
            for(uint32_t j = 0; j < capacity; ++j){
                tmp[tid_index][j] = dat;
            }
        }
//        inform << "Initialized this threads data" << ENDL;

        delete[] stats;
        stats = tmp;
        threadcount = newsize;

//        inform << "Finished adding new thread" << ENDL;
        UnLock();
    }

    void AddImage(){
        Lock();
        imagecount++;
        if (imagecount == 1){
            assert(threadcount == 1);
            assert(imagecount == alldata->CountImages());
            assert(threadcount == alldata->CountThreads());
            for (uint32_t j = 0; j < capacity; j++){
                stats[0][j] = alldata->GetData();
            }
        }
        UnLock();
    }

    // updates stats to hold the correct image data for the
    // entries in buffer
    void Refresh(V buffer, uint32_t num, thread_key_t tid){
        debug(assert(imagecount > 0));
        debug(assert(threadcount > 0));
        debug(assert(num <= capacity));

        if (imagecount == 1){
            return;
        }

        uint32_t threadseq = alldata->GetThreadSequence(tid);
        assert(threadseq < threadcount);

        image_key_t i;
        image_key_t ci = 0;
        T di = NULL;

        for (uint32_t j = 0; j < num; j++, buffer++){

            dataid(buffer, &i);

            if (i == 0){
                if (alldata->CountThreads() > 1){
                    continue;
                }
                assert(false && "the only way blank buffer entries should exist is when threads get signal-interrupted mid-block");
            }

            if (i != ci){
                ci = i;
                di = alldata->GetData(i, tid);
            }
            stats[threadseq][j] = di;

            debug(assert(stats[threadseq][j]));
        }
    };

    T* GetBufferStats(thread_key_t tid){
        assert(imagecount > 0);
        assert(threadcount > 0);

        uint32_t threadseq = alldata->GetThreadSequence(tid);
        assert(threadseq < threadcount);
        //inform << "Getting buffer stats for thread " << dec << threadseq << ENDL;

        return stats[threadseq];
    }
};

// support for MPI wrapping
static bool MpiValid = false;
static bool IsMpiValid() { return MpiValid; }

extern "C" {
#ifdef HAVE_MPI
// C init wrapper
#ifdef USES_PSINSTRACER
int __give_pebil_name(MPI_Init)(int* argc, char*** argv){
    int retval = 0;
#else
int __wrapper_name(MPI_Init)(int* argc, char*** argv){
    int retval = PMPI_Init(argc, argv);
#endif // USES_PSINSTRACER

    PMPI_Comm_rank(MPI_COMM_WORLD, &__taskid);
    PMPI_Comm_size(MPI_COMM_WORLD, &__ntasks);

    MpiValid = true;

    fprintf(stdout, "-[p%d]- Mapping pid to taskid %d/%d in MPI_Init wrapper\n", getpid(), __taskid, __ntasks);
    tool_mpi_init();

    return retval;
}

extern void pmpi_init_(int*);

#ifdef USES_PSINSTRACER
void __give_pebil_name(mpi_init_)(int* ierr){
#else
void __wrapper_name(mpi_init_)(int* ierr){
    pmpi_init_(ierr);
#endif // USES_PSINSTRACER

    PMPI_Comm_rank(MPI_COMM_WORLD, &__taskid);
    PMPI_Comm_size(MPI_COMM_WORLD, &__ntasks);

    MpiValid = true;

    fprintf(stdout, "-[p%d]- Mapping pid to taskid %d/%d in mpi_init_ wrapper\n", getpid(), __taskid, __ntasks);
    tool_mpi_init();
}
#endif // HAVE_MPI
};

#endif //_InstrumentationCommon_hpp_

