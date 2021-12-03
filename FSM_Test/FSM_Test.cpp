
#include <iostream>
#include <future>
#include "Bot.h"
#include "windows.h"
#include <string>
using namespace std;
enum {
    walking = 'WALK',//走路状态，注：单引号表示不是字符串，实际是一个整形，也可设置为一个整数
    defending = 'DEFN',//防御状态
    tick = 'tick'//刷新
};

struct ant_t {
    fsm::stack fsm;
    int health, distance, flow;

    ant_t() : health(0), distance(0), flow(1) {
        // define fsm transitions: on(state,trigger) -> do lambda
        fsm.on(walking, 'init') = [&](const fsm::args& args) {//行走状态，绑定init函数（初始化）
            std::cout << "initializing" << std::endl;
        };
        fsm.on(walking, 'quit') = [&](const fsm::args& args) {//行走状态，绑定quit函数（状态结束时清理）
            std::cout << "exiting" << std::endl;
        };
        fsm.on(walking, 'push') = [&](const fsm::args& args) {//行走状态，绑定push函数，暂停时执行
            std::cout << "pushing current task." << std::endl;
        };
        fsm.on(walking, 'back') = [&](const fsm::args& args) {//行走状态，绑定back函数，恢复时执行
            std::cout << "back from another task. remaining distance: " << distance << std::endl;
        };
        fsm.on(walking, tick) = [&](const fsm::args& args) {//行走状态，定义一个tick动作，用于输出一些信息
            std::cout << "\r" << "\\|/-" [abs(distance % 4)] << " walking: " << distance << (flow > 0 ? " -->" : " <--") << " ";
            distance += flow;
            if (1000 == distance) {
                std::cout << "at food!" << std::endl;
                flow = -flow;
            }
            if (-1000 == distance) {
                std::cout << "at home!" << std::endl;
                flow = -flow;
            }
        };
        fsm.on(defending, 'init') = [&](const fsm::args& args) {//防御状态，绑定init函数（初始化）
            health = 4000;
            std::cout << "somebody is attacking me! he has " << health << " health points" << std::endl;
        };
        fsm.on(defending, 'push') = [&](const fsm::args& args) {//行走状态，绑定push函数，暂停时执行
            std::cout << "pushing current task." << std::endl;
        };
        fsm.on(defending, 'back') = [&](const fsm::args& args) {//行走状态，绑定back函数，恢复时执行
            std::cout << "back from another task. remaining distance: " << distance << std::endl;
        };
        fsm.on(defending, tick) = [&](const fsm::args& args) {//行走状态，定义一个tick动作，用于输出一些信息
            std::cout << "\r" << "\\|/-$"[health % 4] << " health: (" << health << ")   ";
            --health;
            if (health < 0) {
                std::cout << std::endl;
                fsm.pop();//血量小于0，结束本状态，恢复为之前的状态
            }
        };

        // set initial fsm state
        fsm.set(walking);//初始化状态为行走状态
    }
};

void AttackMethod()
{

    //switch (switch_on)
    //{
    //default:
    //    break;
    //}
}

ant_t ant;//创建一个蚂蚁战士（状态机）
void MainFunctionThread()
{

    while (true)
    {
        if (0 == rand() % 10000) {
            ant.fsm.push(defending);
        }
        ant.fsm.command(tick);//刷新每一帧，输出一些信息
        Sleep(1);
    }
}


int main() {
    auto CallFunction = async(launch::async, MainFunctionThread);
    int Input = 0;
    while (true)
    {
        cin >> Input;
        switch (Input)
        {
        case 0:
            ant.fsm.push(defending);
            break;
        case 1 :
            ant.fsm.push(walking);
            break;
        default:
            break;
        }
    }



}