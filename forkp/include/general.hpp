#ifndef _GENERAL_HPP_
#define _GENERAL_HPP_

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <cstring>
#include <string>
using std::string;

#include <cstdint>
using std::int64_t;
using std::uint64_t;

#include <boost/log/trivial.hpp>

// C++0x
#if __cplusplus <= 199711L
#define nullptr 0

#include <boost/function.hpp>
using boost::function;

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
using boost::shared_ptr;
using boost::make_shared;

#include <boost/bind.hpp>
using boost::bind;

#else

#include <memory>

using std::function;

using std::shared_ptr;
using std::make_shared;

using std::bind;

#endif

#include <signal.h>

namespace forkp {

static inline const char* basename(const char* file) {
    const char* p = strrchr(file, '/');
    if (p) return p + 1;

    p = strrchr(file, '\\');
    if (p) return p + 1;

    return file;
}

#if 0
#define BOOST_LOG_T(x) std::cerr<<std::endl<<#x<<":"<<basename(__FILE__)<<__LINE__<<"[@"<<__func__<<"]"<<" "
#else
#define BOOST_LOG_T(x) BOOST_LOG_TRIVIAL(x)<<basename(__FILE__)<<":"<<__LINE__<<"[@"<<__func__<<"]"<<" "
#endif


int st_make_nonblock(int socket);

/**
 * 这里有两个设计和开发方向可以细化：
 * 1).是把本软件当做框架库的方式，新开发的程序以可调用体的方式注册进来，注重于对单个同构
 *    程序进程的精确控制，类似于面向Nginx的服务进程的管理；
 * 2).另外一个是作为通用的监测程序，注册和管理已经开发完毕的二进制程序，这个方向也就是说
 *    会处理多个异构的服务，所以重点在于服务的增添删减，各个服务单独操作，不让整个系统停摆
 */

enum class FORKP_SIG {
    FORKP_INFO = SIGUSR1,  /* 显示forkp信息 */
    SHDN_CHLD = SIGTERM,   /* 杀死所有children process*/
    REOP_CHLD = SIGUSR2,   /* 杀死后重启所有children process*/
    WATCH_DOG = SIGWINCH,  /* 看门狗，只在Process模式下支持 */
    CHLD      = SIGCHLD,
    PIPE      = SIGPIPE,
};

struct forkp_sig_cmd {
    volatile bool terminate;
    volatile bool shutdown_child;
    volatile bool reopen_child;
};
extern struct forkp_sig_cmd  FORKP_SIG_CMD;

#define FORKP_SIG_R(x) (static_cast<int>(x))

#define FORKP_SIG_BLOCK(x) do { \
    sigset_t set; \
    ::sigemptyset(&set); \
    ::sigaddset(&set, FORKP_SIG_R(x)); \
    ::sigprocmask(SIG_BLOCK, &set, NULL); \
} while(0)

#define FORKP_SIG_UNBLOCK(x) do { \
    sigset_t set; \
    ::sigemptyset(&set); \
    ::sigaddset(&set, FORKP_SIG_R(x)); \
    ::sigprocmask(SIG_UNBLOCK, &set, NULL); \
} while(0)

// RAII signal helper
class FORKP_SIG_GUARD {
public:
    FORKP_SIG_GUARD(FORKP_SIG sig):
    sig_(sig) {
        FORKP_SIG_BLOCK(sig_);
    }

    ~FORKP_SIG_GUARD() {
        FORKP_SIG_UNBLOCK(sig_);
    }
private:
    FORKP_SIG sig_;
};

}



#endif // _GENERAL_HPP_
