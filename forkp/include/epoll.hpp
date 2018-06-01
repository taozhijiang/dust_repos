#ifndef _EPOLLH_HPP_
#define _EPOLLH_HPP_

#include "general.hpp"
#include <sys/epoll.h>
#include <map>

namespace forkp {

typedef function<bool()> epollHandler;

// epoll with handler, closuer
class Epoll {
public:
    Epoll(std::size_t max_events):
    max_events_(max_events),
    event_fd_(-1),
    events_(NULL),
    handlers_()
    {
        // 不支持异步操作
        if(max_events_ == 0)
            return;

        events_ = (struct epoll_event*)calloc(max_events_, sizeof(struct epoll_event));
        assert(events_);
        memset(events_, 0, max_events_ * sizeof(struct epoll_event));
        event_fd_ = epoll_create(max_events_);
        assert(event_fd_ != -1);
    }

    bool addEvent(int fd, int events, const epollHandler& handler){
        if( ! ctlEvent(fd, EPOLL_CTL_ADD, events) )
            return false;

        if( handlers_.find(fd) != handlers_.end() )
            BOOST_LOG_T(info) << "Handler for " << fd << " already exist, replace it! ";

        handlers_[fd] = handler;
        return true;
    }

    bool delEvent(int fd) {
        handlers_.erase(fd);
        return ctlEvent(fd, EPOLL_CTL_DEL, 0);
    }

    // EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL
    // EPOLLIN|EPOLLOUT|EPOLLERR
    bool ctlEvent(int fd, int op, int events) {
        struct epoll_event event;
        int ret = 0;

        if (event_fd_ == -1 || max_events_ == 0)
        {
            BOOST_LOG_T(debug) << "Resize events default to 128 ";
            max_events_ = 128; // for default
            events_ = (struct epoll_event*)calloc(max_events_, sizeof(struct epoll_event));
            assert(events_);
            memset(events_, 0, max_events_ * sizeof(struct epoll_event));
            event_fd_ = epoll_create(max_events_);
            assert(event_fd_ != -1);
        }

        st_make_nonblock(fd);
        event.data.fd = fd;
        event.events = events | EPOLLET; //edge tridge
        ret = epoll_ctl (event_fd_, op, fd, &event);

        return ret == -1 ? false : true;
    }

    // -1 block for ever
    // collected something, return true
    bool traverseAndHandleEvent(int ms){
        if (event_fd_ == -1 || !events_)
            return false;

        int ret  = epoll_wait(event_fd_, events_, max_events_, ms);
        if (ret == -1) {
            if ( errno == EINTR)
                return true;

            BOOST_LOG_T(debug) << "epoll_wait error: " << ms << ", ret=" << ret;
            return false;
        }

        if (ret == 0)
            return false;

        std::map<int, epollHandler>::const_iterator cit;
        for (int i=0; i<ret; ++i) {
            cit = handlers_.find(events_[i].data.fd);
            if( cit != handlers_.end() ){
                (cit->second)();
            }
            else {
                BOOST_LOG_T(debug) << "null handler for " << events_[i].data.fd ;
            }
        }

        return true;
    }

    ~Epoll(){
        if (event_fd_ != -1)
            close(event_fd_);

        free(events_);
    }

private:
    std::size_t max_events_;
    int event_fd_;
    struct epoll_event* events_;
    std::map<int, epollHandler> handlers_;
};

}


#endif //_EPOLLH_HPP_
