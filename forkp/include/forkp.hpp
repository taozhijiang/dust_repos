#ifndef _FORKP_HPP_
#define _FORKP_HPP_

#include "general.hpp"
#include "master.hpp"

namespace forkp {

typedef function<bool()> InitFunc;

extern char **exec_main_argv;

#define MasterIntance (forkp::Master::getInstance())

}



#endif // _FORKP_HPP_
