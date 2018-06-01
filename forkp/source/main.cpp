#include <sys/types.h>          /* See NOTES */
#include <string>

#include "general.hpp"
#include "forkp.hpp"

void taskFu(int id) {
    for (;;){
        std::cerr << "Start taskFunc with: " << id << std::endl;
        ::sleep(1);
    }
}

bool init_func() {
    std::cout << "User Init Func Called!" << std::endl;
    return true;
}


int main(int argc, char* argv[])
{
    // for change process name
    forkp::exec_main_argv = argv;

    MasterIntance.user_init_register(init_func);

    MasterIntance.userInitProc();

    // MasterIntance.spawnWorkers("test_proc", std::bind(taskFu, 2));
    // MasterIntance.spawnWorkers("test_proc", std::bind(taskFu, 8));

    char *args1[] = {"fp:airobot", (char *) 0 };
    MasterIntance.spawnWorkers("airobot", "/home/taozj/project/airobot/",
    							"/home/taozj/project/airobot/Debug/airobot", args1);


    char *args2[] = {"fp:ailawd", (char *) 0 };
    MasterIntance.spawnWorkers("ailawd", "/home/taozj/project/ailawd_c++0x/",
    							"/home/taozj/project/ailawd_c++0x/Debug/ailawd", args2);

    char *args3[] = {"fp:ai-log", (char *) 0 };
    MasterIntance.spawnWorkers("airobot_logd", "/home/taozj/project/airobot_logd/",
    							"/home/taozj/project/airobot_logd/Debug/airobot_logd", args3);

    char *args4[] = {"fp:redis-srv", (char *) 0 };
    MasterIntance.spawnWorkers("redis srv", "/home/taozj/project/redis-3.2.4/",
    							"/home/taozj/project/redis-3.2.4/src/redis-server", args4);


    MasterIntance.masterLoop();

    return 0;
}

