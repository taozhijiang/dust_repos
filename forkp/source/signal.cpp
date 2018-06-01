#include "general.hpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/support/date_time.hpp>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <execinfo.h>
#include <signal.h>

#include <sys/wait.h>

#include <set>
#include "master.hpp"

namespace forkp {

    #define MasterIntance (forkp::Master::getInstance())
    Master* Master::master_instance_ = nullptr;

    struct forkp_sig_cmd  FORKP_SIG_CMD;

    /**
     * 虽然Master是单线程的，但是信号是异步的，在信号处理函数中操作
     * 全局数据结构是不安全的，所以这也是Nginx使用设置标识然后主进程
     * 轮训处理的方式的原因，因为这样可以避免某些数据结构的保护工作
     *
     * 默认情况下，信号处理函数调用的过程中会block掉正在处理的信号
     */

    void signalHander(int signo) {
        if (signo == FORKP_SIG_R(FORKP_SIG::CHLD)) {
            /**
             * waitpid可能一次信号有多个CHILD就绪，必须放在循环中直到返回0表示处理完毕，
             * 否则会有遗留的僵尸线程问题
             */
            for (;;) {
                int stat;

                pid_t pid = waitpid(-1, &stat, WNOHANG);
                if (pid == 0)
                    return;

                if (pid < 0) {
                    BOOST_LOG_T(error) << "SIGCHLD wait error!";
                    return;
                }

                if (WIFEXITED(stat))
                    BOOST_LOG_T(debug) << "child process " << pid << " exit normal!";
                else
                    BOOST_LOG_T(error) << "child process " << pid << " exit not normal!";

                MasterIntance.insertDeferWorkPid(pid);

            }
        }
        else if (signo == FORKP_SIG_R(FORKP_SIG::FORKP_INFO)) {
            MasterIntance.showAllStat();
        }
        else if (signo == FORKP_SIG_R(FORKP_SIG::SHDN_CHLD)) {
            FORKP_SIG_CMD.shutdown_child = 1;
        }
        else if (signo == FORKP_SIG_R(FORKP_SIG::REOP_CHLD)) {
            FORKP_SIG_CMD.reopen_child = 1;
        }
        return;
    }

    /* signal init */
    struct signal_t {
        FORKP_SIG signo;
        const char* desc;
        //void (*sighandler_t)(int)
        sighandler_t handler;
    };

    static std::vector<signal_t> signal_list {
        {FORKP_SIG::FORKP_INFO, "SIG_FORKP_INFO: print info", signalHander },
        {FORKP_SIG::SHDN_CHLD,  "SIG_SHDN_CHLD: shutdown all children process", signalHander },
        {FORKP_SIG::REOP_CHLD,  "SIG_REOP_CHLD: restart all children process", signalHander },
        {FORKP_SIG::CHLD,       "SIG_CHLD: child process terminated", signalHander },
        {FORKP_SIG::PIPE,       "SIG_PIPE: SIG_IGN", SIG_IGN },
        {FORKP_SIG::WATCH_DOG,  "SIG_WATCH_DOG: SIG_IGN", SIG_IGN },
    };

    extern void signal_init() {
        std::vector<signal_t>::const_iterator cit;
        for (cit = signal_list.cbegin(); cit != signal_list.cend(); ++cit) {
            BOOST_LOG_T(debug) << "Signal Hook for " << cit->desc;
            ::signal(FORKP_SIG_R(cit->signo), cit->handler);
        }

        BOOST_LOG_T(info) << "Signal Init OK!";
        return;
    }

    extern void signal_default() {
        std::vector<signal_t>::const_iterator cit;
        for (cit = signal_list.cbegin(); cit != signal_list.cend(); ++cit) {
            if (cit->signo == FORKP_SIG::PIPE) {
                ::signal(FORKP_SIG_R(cit->signo), SIG_IGN);
            }
            ::signal(FORKP_SIG_R(cit->signo), SIG_DFL);
        }

        BOOST_LOG_T(info) << "Signal Default OK!";
        return;
    }

    static void backtrace_info(int sig, siginfo_t *info, void *f)
    {
        int j, nptrs;
    #define BT_SIZE 100
        char **strings;
        void *buffer[BT_SIZE];

        fprintf(stderr,       "\nSignal [%d] received.\n", sig);
        BOOST_LOG_T(fatal) << "\nSignal [" << sig << "] received.\n";
        fprintf(stderr,       "======== Stack trace ========");
        BOOST_LOG_T(fatal) << "======== Stack trace ========\n";

        nptrs = ::backtrace(buffer, BT_SIZE);
        BOOST_LOG_T(fatal) << "backtrace() returned %d addresses";
        fprintf(stderr,       "backtrace() returned %d addresses\n", nptrs);

        strings = ::backtrace_symbols(buffer, nptrs);
        if (strings == NULL)
        {
            perror("backtrace_symbols");
            BOOST_LOG_T(fatal) << "backtrace_symbols";
            exit(EXIT_FAILURE);
        }

        for (j = 0; j < nptrs; j++)
        {
            fprintf(stderr, "%s\n", strings[j]);
            BOOST_LOG_T(fatal) << strings[j];
        }

        free(strings);

        fprintf(stderr,       "Stack Done!\n");
        BOOST_LOG_T(fatal) << "Stack Done!";

        ::kill(getpid(), sig);
        ::abort();

    #undef BT_SIZE
    }

    extern void backtrace_init() {

        struct sigaction act;

        sigemptyset(&act.sa_mask);
        act.sa_flags     = SA_NODEFER | SA_ONSTACK | SA_RESETHAND | SA_SIGINFO;
        act.sa_sigaction = backtrace_info;
        sigaction(SIGABRT, &act, NULL);
        sigaction(SIGBUS,  &act, NULL);
        sigaction(SIGFPE,  &act, NULL);
        sigaction(SIGSEGV, &act, NULL);

        BOOST_LOG_T(info) << "Backtrace Init OK!";
        return;
    }

}

