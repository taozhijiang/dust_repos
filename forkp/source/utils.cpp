#include "general.hpp"

#include <unistd.h>
#include <fcntl.h>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/support/date_time.hpp>

#include "master.hpp"

namespace forkp {


    extern char **exec_main_argv = NULL;

    bool st_feed_watchdog( int src, WorkerStat_Ptr& workstat) {
        char read_buf;

        read(src, &read_buf, 1);
        if (workstat->this_miss_cnt > 0)
            workstat->this_miss_cnt = 0;

        return true;
    }

    bool st_transform_to_fd( int src, int des ) {
        char buff[512];
        int ret = 0;

        while ((ret = read(src, buff, 512)) > 0)
            write(des, buff, ret);

        return true;
    }

    bool st_rename_process(const char* name) {
        if (!name &&  !strlen(name))
            return false;

        if (exec_main_argv && exec_main_argv[0]) {
            BOOST_LOG_T(info) << "Rename original process name! ";
            std::size_t len = strlen(exec_main_argv[0]);
            strncpy(exec_main_argv[0], name, len);

            return true;
        }

        return false;
    }

    int st_make_nonblock(int socket)
    {
        int flags = 0;

        flags = fcntl (socket, F_GETFL, 0);
    	flags |= O_NONBLOCK;
        fcntl (socket, F_SETFL, flags);

        return 0;
    }

    namespace blog_sink = boost::log::sinks;
    namespace blog_expr = boost::log::expressions;
    namespace blog_keyw = boost::log::keywords;
    namespace blog_attr = boost::log::attributes;

    void boost_log_init(const string& prefix) {
        boost::log::add_common_attributes();
        //boost::log::core::get()->add_global_attribute("Scope",  blog_attr::named_scope());
        boost::log::core::get()->add_global_attribute("Uptime", blog_attr::timer());

        boost::log::add_file_log(
            blog_keyw::file_name = prefix+"_%N.log",
            blog_keyw::time_based_rotation =
                    blog_sink::file::rotation_at_time_point(0, 0, 0),
            blog_keyw::open_mode = std::ios_base::app,
            blog_keyw::format = blog_expr::stream
               // << std::setw(7) << std::setfill(' ') << blog_expr::attr< unsigned int >("LineID") << std::setfill(' ') << " | "
                << "["   << blog_expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d, %H:%M:%S.%f")
                << "] [" << blog_expr::format_date_time< blog_attr::timer::value_type >("Uptime", "%O:%M:%S")
               // << "] [" << blog_expr::format_named_scope("Scope", blog_keyw::format = "%n (%F:%l)")
                << "] <"  << boost::log::trivial::severity << "> "
                << blog_expr::message,
            blog_keyw::auto_flush = true
            );

        // trace debug info warning error fatal
        boost::log::core::get()->set_filter (
            boost::log::trivial::severity >= boost::log::trivial::trace);

        BOOST_LOG_T(info) << "BOOST LOG V2 Initlized OK!";

        return;
    }

}

