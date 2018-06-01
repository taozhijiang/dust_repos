#ifndef _WORKER_HPP_
#define _WORKER_HPP_

#include "general.hpp"
#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <cassert>

namespace forkp {

typedef struct {
	int read_;
	int write_;
} Notify;

class Master;
typedef function<void()> taskFunc;
class Worker;
static Worker* p_worker = NULL;
static void workerSignalHandler(int signo);

extern bool st_rename_process(const char* name);
extern void signal_default();

enum class WorkerType {
    workerProcess = 1,
    workerExec = 2,
};


class Worker {

    friend class Master;
    friend void workerSignalHandler(int signo);

public:
    Worker(const char* name, const taskFunc& func):
        type_(WorkerType::workerProcess),
        cwd_(nullptr),
        exec_(nullptr),
        exec_argv_(nullptr),
        func_(func),
        pid_ ( getpid() ),
        ppid_ ( getpid() ),
        notify_( {-1, -1} ),
        channel_( {-1, -1} )
    {
        strncpy(proc_title_, name, 16);
        proc_title_[sizeof(proc_title_)-1] = 0;
    }

    Worker(const char* name, const char* cwd,
           const char* const exec, char *const *argv):
        type_(WorkerType::workerExec),
        cwd_(cwd),
        exec_(exec),
        exec_argv_(argv),
        func_(taskFunc()),
        pid_ ( getpid() ),
        ppid_ ( getpid() ),
        notify_( {-1, -1} ),
        channel_( {-1, -1} )
    {
        strncpy(proc_title_, name, 16);
        proc_title_[sizeof(proc_title_)-1] = 0;
    }


    virtual ~Worker(){
        workerReset();
    }

    // 重置状态，只可能被master 调用
    void workerReset() {
        pid_ = ppid_ = getpid();

        if (channel_.read_ != -1)
            close(channel_.read_);
        if (channel_.write_ != -1)
            close(channel_.write_);

        if (notify_.read_ != -1)
            close(notify_.read_);
        if (notify_.write_ != -1)
            close(notify_.write_);

        channel_.read_ = channel_.write_ = -1;
        notify_.read_ = notify_.write_ = -1;

        return;
    }

    // 使用函数开辟进程
    void startProcess() {

        assert(pid_ == getppid());
        pid_ = getpid();
        st_rename_process(proc_title_);

        if (!prepStart()) {
            BOOST_LOG_T(info) << "prepStart error, we abort for " << proc_title_;
            ::abort();
        }

        func_();
    }

    // 使用外部exec开辟进程
    void startExec() {
        assert(pid_ == getppid());
        pid_ = getpid();

        // Can not handle exec rename ... but just exec argv[1] specify it!
        // st_rename_process(proc_title_);
        if (cwd_){
            BOOST_LOG_T(info) << "Changing working dir to " << cwd_;
            ::chdir(cwd_);
        }

        if (!prepStart()) {
            BOOST_LOG_T(error) << "prepStart error, we abort for " << proc_title_;
            ::abort();
        }

        ::execv(exec_, exec_argv_);

        // if return, err found
        ::abort();
    }

private:

    bool prepStart() {
        // 暴露this给传统C函数使用
        p_worker = this;

        // reset SignalHandler
        signal_default();

        // 注意，下面的调用必须谨慎，因为exec族函数会继承calling process的
        // signal_pend和signal_mask！！！

        if (type_ == WorkerType::workerProcess) {

            FORKP_SIG_BLOCK(FORKP_SIG::CHLD);

            //::signal(FORKP_SIG_R(FORKP_SIG::WATCH_DOG), SIG_IGN);
            ::signal(FORKP_SIG_R(FORKP_SIG::WATCH_DOG), workerSignalHandler);
        }

        return true;
    }


private:
    char proc_title_[16];
    WorkerType type_;

    const char* cwd_;
    const char* exec_;
    char *const *exec_argv_;
    const taskFunc func_;

    pid_t pid_;
    pid_t ppid_;
    Notify notify_;
    Notify channel_;  // STDOUt/STDERR redirect
};

typedef shared_ptr<Worker> Worker_Ptr;

/**
 * 对于子进程的保活，如果是Process类型，那么父进程定期向
 * 子进程发送信号，子进程从notify_发回一个字节
 *
 * 对于一般的Exec，父进程只能用非信号的kill，查看子进程是否还存在
 */
static void workerSignalHandler(int signo) {

    // 对Master发送的信号，通过notify_通道回写一个字节
    if (signo == FORKP_SIG_R(FORKP_SIG::WATCH_DOG) ) {
        if (!p_worker || p_worker->notify_.write_ == -1) {
            BOOST_LOG_T(error) << "for process, notify_.write_ is -1 !!";
            ::abort();
        }

        write(p_worker->notify_.write_, "C", 1);
    }
}


}



#endif // _WORKER_HPP_
