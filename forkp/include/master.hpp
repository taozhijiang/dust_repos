#ifndef _MASTER_HPP_
#define _MASTER_HPP_

#include "general.hpp"
#include "worker.hpp"
#include "epoll.hpp"

#include <map>
#include <set>
#include <deque>
#include <sys/types.h>
#include <unistd.h>

#include <boost/format.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/timerfd.h>

namespace forkp {

/**
 * Alive check method
 *
 * PIPE 'C'
 * kill(0) exist
 */

extern void signal_init();
extern void boost_log_init(const string& prefix);
extern void backtrace_init();

typedef function<bool()> InitFunc;
typedef function<void()> taskFunc;
typedef function<bool()> epollHandler;

extern bool st_rename_process(const char* name);
extern bool st_transform_to_fd(int src,  int des);

typedef struct {
    pid_t        this_pid;
    time_t       this_start_tm;
    std::size_t  this_miss_cnt; // 当前启动时候错过的心跳

    pid_t        start_pid;       //最初始的pid
    time_t       start_tm;
    std::size_t  restart_cnt;     // 启动成功次数
    std::size_t  restart_err_cnt; // 启动失败次数

    int          out_log_fd;

    Worker_Ptr   worker;
} WorkerStat_t;

typedef shared_ptr<WorkerStat_t> WorkerStat_Ptr;

extern bool st_feed_watchdog( int src, WorkerStat_Ptr& workstat);

class Master{

    friend bool st_feed_watchdog( int src, WorkerStat_Ptr& workstat);

public:
    static Master& getInstance() {
        if (!master_instance_)
            createInstance();

        return *master_instance_;
    }

    bool user_init_register(const InitFunc& func){
        init_list_.emplace_back(func);
        return true;
    }


    bool insertDeferWorkPid(pid_t pid) {
        defer_workers_.push_back(pid);
        return true;
    }

    bool spawnWorkers( const char* name, const char* cwd,
                        const char* exec, char *const *argv) {
        Worker_Ptr node = make_shared<Worker>(name, cwd, exec, argv);
        WorkerStat_Ptr workstat = make_shared<WorkerStat_t>();
        if (!node || !workstat)
            return false;

        // 首次启动参数
        workstat->start_tm = ::time(NULL);
        workstat->restart_cnt = 0;
        workstat->restart_err_cnt = 0;
        workstat->out_log_fd = -1;
        workstat->worker = node;

        return trySpawnWorkers(workstat);
    }

    // 用户空间启动进程
    bool spawnWorkers(const char* name, const taskFunc& func){
        Worker_Ptr node = make_shared<Worker>(name, func);
        WorkerStat_Ptr workstat = make_shared<WorkerStat_t>();
        if (!node || !workstat)
            return false;

        // 首次启动参数
        workstat->start_tm = ::time(NULL);
        workstat->restart_cnt = 0;
        workstat->restart_err_cnt = 0;
        workstat->out_log_fd = -1;
        workstat->worker = node;

        return trySpawnWorkers(workstat);
    }

    // 内部可以使用的重启进程的接口
    bool trySpawnWorkers(WorkerStat_Ptr& workstat){
        bool ret = false;

        if (!forkPrepare(workstat)) {
            BOOST_LOG_T(error) << "fork_prepare worker failed, push to dead_workers!";
            ++ workstat->restart_err_cnt;
            dead_workers_.insert(workstat);
            return false;
        }

        pid_t worker_pid = spawn_workers_internel(workstat);

        // parent process routine

        if (worker_pid < 0) {
            BOOST_LOG_T(error) << "start worker failed, push to dead_workers!";
            ++ workstat->restart_err_cnt;
            dead_workers_.insert(workstat);
            return false;
        }


        close(workstat->worker->channel_.write_);
        workstat->worker->channel_.write_ = -1;

        if (!workstat->start_pid)
            workstat->start_pid = worker_pid;

        if (workstat->out_log_fd < 0) {
            char file[PATH_MAX];
            snprintf(file, PATH_MAX, "./log/%s_%d.log", workstat->worker->proc_title_,
                     workstat->start_pid);
            workstat->out_log_fd = open(file, O_CREAT|O_WRONLY|O_APPEND, 0640);
            if (workstat->out_log_fd < 0)
                BOOST_LOG_T(error) << "redirect target child process stdout/stderr to " << file << " failed!";
        }

        if (workstat->out_log_fd > 0) {
            assert (workstat->worker->channel_.read_ != -1);
            ret = epollh_->addEvent(workstat->worker->channel_.read_,
                                EPOLLIN | EPOLLERR,
                                bind(st_transform_to_fd,
                                     workstat->worker->channel_.read_,
                                     workstat->out_log_fd));
            if (!ret)
                BOOST_LOG_T(error) << "register channel_ failed for " << workstat->worker->channel_.read_;
        }

        if (workstat->worker->type_ == WorkerType::workerProcess) {
            assert (workstat->worker->notify_.read_ != -1);

            close(workstat->worker->notify_.write_);
            workstat->worker->notify_.write_ = -1;

            ret = epollh_->addEvent(workstat->worker->notify_.read_,
                                EPOLLIN | EPOLLERR,
                                bind(st_feed_watchdog,
                                     workstat->worker->notify_.read_,
                                     workstat));
            if (!ret)
                BOOST_LOG_T(error) << "register notify_ failed for " << workstat->worker->notify_.read_;
        }

        ++workstat->restart_cnt;
        assert(workers_.find(worker_pid) == workers_.end());
        workers_[worker_pid] = workstat;

        BOOST_LOG_T(debug) << "start worker OK for " << workstat->worker->proc_title_ <<
            ", with pid=" << worker_pid ;
        return true;
    }

    void masterLoop() {

    	for( ; ; ) {

            epollh_->traverseAndHandleEvent(50);

            if (FORKP_SIG_CMD.terminate){
                BOOST_LOG_T(debug) << " !!!!!!!!!!!!!!!!!!!!";
                BOOST_LOG_T(debug) << " !!!!!!!!!!!!!!!!!!!!";
                BOOST_LOG_T(debug) << " !!!!!!!!!!!!!!!!!!!!";

                ::exit(EXIT_SUCCESS);
            }

            processDeferChild();

            if (FORKP_SIG_CMD.shutdown_child) {
                if (!workers_.empty()) {
                    shutdownAllChild();
                } else {
                    BOOST_LOG_T(info) << "SHUTDOWN children process finished!";
                    FORKP_SIG_CMD.shutdown_child = false;
                }
            }

            // shutdown all children process, and restart them all
            if (FORKP_SIG_CMD.reopen_child) {
                if (!workers_.empty()) {
                    shutdownAllChild();
                } else {
                    BOOST_LOG_T(info) << "SHUTDOWN children process finished, not respawn them!";
                    FORKP_SIG_CMD.reopen_child = false;

                    // 启动的时候，如果启动失败会重新修改dead_workers_容器，干扰迭代器，所以
                    // 这里必须将需要启动的worker拷贝出来，然后清空dead_workers_
                    // std::set can only retrive const
                    std::vector<WorkerStat_Ptr>::iterator it;
                    std::vector<WorkerStat_Ptr> respawn(dead_workers_.cbegin(), dead_workers_.cend());
                    dead_workers_.clear();
                    for (it = respawn.begin(); it != respawn.end(); ++it) {
                        BOOST_LOG_T(debug) << "respown child process " << (*it)->worker->proc_title_;
                        trySpawnWorkers(*it);
                    }
                }
            }
        }

    }

    ~Master() = default;

    bool userInitProc() {
        std::vector<InitFunc>::const_iterator it;
        for (it = init_list_.cbegin(); it != init_list_.cend(); ++it){
            if (!(*it)())
                return false;
        }

        BOOST_LOG_T(info) << "User Init OK!";
        return true;
    }

    void showAllStat() {
        BOOST_LOG_T(info) << "!!!! forkp status info !!!!";
        std::cerr << "!!!! forkp status info !!!!" << std::endl;

        BOOST_LOG_T(info) << "!!!! active workers:" ;
        std::cerr << "!!!! active workers:" << std::endl;
        if (workers_.empty()) {
            BOOST_LOG_T(info) << "<< None >>" ;
            std::cerr << "<< None >>" << std::endl;
        }
        std::map<pid_t, WorkerStat_Ptr>::const_iterator m_it;
        for (m_it = workers_.cbegin(); m_it != workers_.cend(); ++m_it) {
            BOOST_LOG_T(info) << boost::format("[%c]proc:%s, pid:%d, start_pid:%lu, start_tm:%lu, this_start_tm:%lu, restart_cnt:%lu ")
            % (m_it->second->worker->type_ == WorkerType::workerProcess ? 'P':'E') %
                m_it->second->worker->proc_title_ % m_it->second->this_pid % m_it->second->start_pid %
                m_it->second->start_tm %
                m_it->second->this_start_tm % m_it->second->restart_cnt;
            std::cerr << boost::format("[%c]proc:%s, pid:%d, start_pid:%lu, start_tm:%lu, this_start_tm:%lu, restart_cnt:%lu ")
            % (m_it->second->worker->type_ == WorkerType::workerProcess ? 'P':'E') %
                m_it->second->worker->proc_title_ % m_it->second->this_pid % m_it->second->start_pid %
                m_it->second->start_tm %
                m_it->second->this_start_tm % m_it->second->restart_cnt << std::endl;
        }

        BOOST_LOG_T(info) << "!!!! dead workers:" ;
        std::cerr << "!!!! dead workers:" << std::endl;
        if (dead_workers_.empty()){
            BOOST_LOG_T(info) << "<< None >>" << std::endl;
            std::cerr << "<< None >>" << std::endl;
        }
        std::set<WorkerStat_Ptr>::const_iterator s_it;
        for (s_it = dead_workers_.cbegin(); s_it != dead_workers_.cend(); ++s_it) {
            BOOST_LOG_T(info) << boost::format("[%c]proc:%s, pid:%d, start_pid:%lu, start_tm:%lu, this_start_tm:%lu, restart_cnt:%lu ")
            % ((*s_it)->worker->type_ == WorkerType::workerProcess ? 'P':'E') %
                (*s_it)->worker->proc_title_ % (*s_it)->this_pid % (*s_it)->start_pid % (*s_it)->start_tm %
                (*s_it)->this_start_tm % (*s_it)->restart_cnt;
            std::cerr << boost::format("[%c]proc:%s, pid:%d, start_pid:%lu, start_tm:%lu, this_start_tm:%lu, restart_cnt:%lu ")
            % ((*s_it)->worker->type_ == WorkerType::workerProcess ? 'P':'E') %
                (*s_it)->worker->proc_title_ % (*s_it)->this_pid % (*s_it)->start_pid % (*s_it)->start_tm %
                (*s_it)->this_start_tm % (*s_it)->restart_cnt << std::endl;
        }
    }

private:

    void processDeferChild() {

        FORKP_SIG_GUARD defer_guard(FORKP_SIG::CHLD);

        if (defer_workers_.empty())
            return;

        std::vector<pid_t>::const_iterator cit;
        WorkerStat_Ptr workstat;
        std::vector<WorkerStat_Ptr> respawn;

        for (cit = defer_workers_.cbegin(); cit != defer_workers_.cend(); ++cit) {
            if (workers_.find(*cit) == workers_.end()) {
                BOOST_LOG_T(error) << "get child process obj failed => " << *cit;
                continue;
            }

            workstat = workers_.at(*cit);
            workers_.erase(*cit);
            if (FORKP_SIG_CMD.reopen_child || FORKP_SIG_CMD.shutdown_child) {
                if (workstat->worker->type_ == WorkerType::workerProcess) {
                    epollh_->delEvent(workstat->worker->notify_.read_);
                }
                epollh_->delEvent(workstat->worker->channel_.read_);
                workstat->worker->workerReset();
                dead_workers_.insert(workstat);
                continue;
            }

            respawn.push_back(workstat);
        }

        defer_workers_.clear();

        if (respawn.empty())
            return;

        // If success, will auto attached to workers_,
        // else, will append to dead_workers_.
        std::vector<WorkerStat_Ptr>::iterator res;
        for (res = respawn.begin(); res != respawn.end(); ++res) {
            BOOST_LOG_T(debug) << "respown child process " << (*res)->worker->proc_title_;
            trySpawnWorkers(*res);
        }

        return;
    }

    void shutdownAllChild() {
        std::map<pid_t, WorkerStat_Ptr>::const_iterator m_it;

        for (m_it = workers_.cbegin(); m_it != workers_.cend(); ++m_it) {
            BOOST_LOG_T(info) << boost::format("SHUTDOWN PROCESS: %s, pid=%lu") %
                m_it->second->worker->proc_title_ % m_it->second->this_pid;
            ::kill(m_it->second->this_pid, SIGKILL);
        }
    }

    bool watchDogCallback() {
        char null_read[8];
        read(watch_dog_timer_fd_, null_read, 8);

        std::map<pid_t, WorkerStat_Ptr>::const_iterator m_it;
        for (m_it = workers_.cbegin(); m_it != workers_.cend(); ++m_it) {

            if (m_it->second->this_miss_cnt > 3) {
                BOOST_LOG_T(error) << "Maxium miss for " << m_it->second->worker->proc_title_ <<
                    ", with pid " << m_it->second->this_pid;
                BOOST_LOG_T(error) << "Kill it immediately!";

                ::kill( m_it->first, SIGKILL );
                continue;
            }

            if (m_it->second->worker->type_ == WorkerType::workerProcess) {
                ::kill(m_it->first, FORKP_SIG_R(FORKP_SIG::WATCH_DOG) );
                ++ m_it->second->this_miss_cnt;
            } else {
                if ( ::kill(m_it->first, 0) == -1 )
                    ++ m_it->second->this_miss_cnt;
                else
                    m_it->second->this_miss_cnt = 0;
            }
        }

        return true;
    }

    int setupWatchDog() {
        int timerfd = -1;

        struct timespec now;
        struct itimerspec itv;

        if( (timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)) == -1)
            return -1;

        ::clock_gettime(CLOCK_MONOTONIC, &now);
        memset(&itv, 0, sizeof(struct itimerspec));

        // 1s的监测间隔
        itv.it_value.tv_sec = now.tv_sec + 1;
        itv.it_interval.tv_sec = 1;

        if( timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &itv, NULL) == -1){
            close(timerfd);
            return -1;
        }

        if ( ! epollh_->addEvent(timerfd,
                                EPOLLIN | EPOLLERR, bind(&Master::watchDogCallback, this)) ) {
                BOOST_LOG_T(error) << "register notify_ failed for " << timerfd;
                close(timerfd);
                return -1;
        }

        return timerfd;
    }

    bool forkPrepare(WorkerStat_Ptr& workstat) {
        int pipefd[2];

        workstat->worker->workerReset();

        // channel_，用于子进程的STDOUT STDERR的重定向

        if ( pipe(pipefd) < 0 ) {
            BOOST_LOG_T(error) << "pipe() Error!";
            return false;
        }

        st_make_nonblock(pipefd[0]);
        st_make_nonblock(pipefd[1]);

        workstat->worker->channel_.read_ = pipefd[0];
        workstat->worker->channel_.write_ = pipefd[1];

        if ( workstat->worker->type_ == WorkerType::workerProcess)
        {
            if ( pipe(pipefd) < 0 ) {
                BOOST_LOG_T(error) << "pipe() for notify_ Error!";
                return false;
            }

            st_make_nonblock(pipefd[0]);
            st_make_nonblock(pipefd[1]);

            workstat->worker->notify_.read_ = pipefd[0];
            workstat->worker->notify_.write_ = pipefd[1];
        }

        workstat->this_miss_cnt = 0;

        return true;
    }

    // -1 for error case
    pid_t spawn_workers_internel(WorkerStat_Ptr& workstat)
    {
        time_t now = ::time(NULL);
        // 避免过快的fork()
        if (now - workstat->this_start_tm < 2)
            ::sleep(1);

        pid_t pid = fork();
        if (pid < 0) {
            BOOST_LOG_T(error) << "Fork() Error!";
            return -1;
        }

        if (pid == 0) // child process
        {

            // 重定向子进程的输出、标准错误输出到channel
            int ret = 0;
            close(workstat->worker->channel_.read_);
            workstat->worker->channel_.read_ = -1;

            ret = ::dup2(workstat->worker->channel_.write_, STDOUT_FILENO);
            ret += ::dup2(workstat->worker->channel_.write_, STDERR_FILENO);

            if (ret < 0){
                BOOST_LOG_T(error) << "dup2 STDOUT_FILENO STDERR_FILENO  Error!";
            }

            if (workstat->worker->type_ == WorkerType::workerProcess) {
                // notify_
                close(workstat->worker->notify_.read_);
                workstat->worker->notify_.read_ = -1;

                workstat->worker->startProcess();
            }
            else {
                workstat->worker->startExec();
            }
        }

        // parent process continue

        workstat->this_pid = pid;
        workstat->this_start_tm = ::time(NULL);

        return pid;
    }

private:

    static void createInstance() {
        static Master master;
        master.Init();
        master_instance_ = &master;
    }

    Master():
        name_("forkp master"),
        workers_(), dead_workers_(), defer_workers_(),
        init_list_(),
        epollh_(make_shared<Epoll>(128)),
        watch_dog_timer_fd_(-1)
    {}

    bool Init() {
    	boost_log_init("forkpRun");
        backtrace_init();
        signal_init();

        st_rename_process(name_);

        FORKP_SIG_CMD.terminate = false;
        FORKP_SIG_CMD.reopen_child = false;
        FORKP_SIG_CMD.shutdown_child = false;

        watch_dog_timer_fd_ = setupWatchDog();

        return true;
    }

    // defined at signal.cpp
    static Master* master_instance_;

private:
    const char* name_;
    std::map<pid_t, WorkerStat_Ptr> workers_;
    std::set<WorkerStat_Ptr>        dead_workers_;  // 没有启动成功的任务

    // 当收到CHLD信号的时候，将对应的进程pid放到这里，减少信号处理的工作，退后的工作放到进程上下文中
    std::vector<pid_t>              defer_workers_;
    std::vector<InitFunc> init_list_;  // container set not supportted
    shared_ptr<Epoll> epollh_;
    int watch_dog_timer_fd_;
};

}



#endif // _MASTER_HPP_
