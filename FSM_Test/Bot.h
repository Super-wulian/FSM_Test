#pragma once

#define FSM_VERSION "1.0.0" /* (2015/11/29) Code revisited to use fourcc integers (much faster); clean ups suggested by Chang Qian
#define FSM_VERSION "0.0.0" // (2014/02/15) Initial version */

#include <algorithm>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fsm
{
    template<typename T>//模板函数，实现任意类型数据转换字符串
    inline std::string to_string(const T& t) {
        std::stringstream ss;
        return ss << t ? ss.str() : std::string();
    }

    template<>
    inline std::string to_string(const std::string& t) {
        return t;
    }

    typedef std::vector<std::string> args;
    typedef std::function< void(const fsm::args& args) > call;

    struct state {//代表状态
        int name;
        fsm::args args;

        state(const int& name = 'null') : name(name)//使用整形初始化一个状态
        {}

        state operator()() const {//运算符重载，返回状态自己
            state self = *this;
            self.args = {};
            return self;
        }
        template<typename T0>
        state operator()(const T0& t0) const {//设置状态的数据t0，并转换成字符串保存
            state self = *this;
            self.args = { fsm::to_string(t0) };
            return self;
        }
        template<typename T0, typename T1>
        state operator()(const T0& t0, const T1& t1) const {//设置状态的数据t0 t1
            state self = *this;
            self.args = { fsm::to_string(t0), fsm::to_string(t1) };
            return self;
        }

        operator int() const {
            return name;
        }

        bool operator<(const state& other) const {
            return name < other.name;
        }
        bool operator==(const state& other) const {
            return name == other.name;
        }

        template<typename ostream>
        inline friend ostream& operator<<(ostream& out, const state& t) {//打印一个状态
            if (t.name >= 256) {
                out << char((t.name >> 24) & 0xff);
                out << char((t.name >> 16) & 0xff);
                out << char((t.name >> 8) & 0xff);
                out << char((t.name >> 0) & 0xff);
            }
            else {
                out << t.name;
            }
            out << "(";
            std::string sep;
            for (auto& arg : t.args) {
                out << sep << arg;
                sep = ',';
            }
            out << ")";
            return out;
        }
    };

    typedef state trigger;

    struct transition {
        fsm::state previous, trigger, current;

        template<typename ostream>
        inline friend ostream& operator<<(ostream& out, const transition& t) {
            out << t.previous << " -> " << t.trigger << " -> " << t.current;
            return out;
        }
    };

    class stack {//保存状态的容量
    public:

        stack(const fsm::state& start = 'null') : deque(1) {//使用一个状态初始化容器
            deque[0] = start;
            call(deque.back(), 'init');//执行init函数
        }

        stack(int start) : stack(fsm::state(start))
        {}

        ~stack() {
            // ensure state destructors are called (w/ 'quit')
            while (size()) {
                pop();//执行每个状态的qiut函数
            }
        }

        // pause current state (w/ 'push') and create a new active child (w/ 'init')
        void push(const fsm::state& state) {//暂停一个状态
            if (deque.size() && deque.back() == state) {
                return;
            }
            // queue
            call(deque.back(), 'push');//执行上一个状态push函数
            deque.push_back(state);
            call(deque.back(), 'init');//执行新状态的init函数
        }

        // terminate current state and return to parent (if any)
        void pop() {//恢复一个状态
            if (deque.size()) {
                call(deque.back(), 'quit');//执行当前状态的qiut函数
                deque.pop_back();
            }
            if (deque.size()) {
                call(deque.back(), 'back');//执行下一个状态的back函数（待恢复的状态）
            }
        }

        // set current active state
        void set(const fsm::state& state) {//设置当前状态为state ,并自动执行相关函数
            if (deque.size()) {
                replace(deque.back(), state);
            }
            else {
                push(state);
            }
        }

        // number of children (stack)
        size_t size() const {
            return deque.size();
        }

        // info
        // [] classic behaviour: "hello"[5] = undefined, "hello"[-1] = undefined
        // [] extended behaviour: "hello"[5] = h, "hello"[-1] = o, "hello"[-2] = l
        fsm::state get_state(signed pos = -1) const {//获取当前状态
            signed size = (signed)(deque.size());
            return size ? *(deque.begin() + (pos >= 0 ? pos % size : size - 1 + ((pos + 1) % size))) : fsm::state();
        }
        fsm::transition get_log(signed pos = -1) const {
            signed size = (signed)(log.size());
            return size ? *(log.begin() + (pos >= 0 ? pos % size : size - 1 + ((pos + 1) % size))) : fsm::transition();
        }
        std::string get_trigger() const {
            std::stringstream ss;
            return ss << current_trigger, ss.str();
        }

        bool is_state(const fsm::state& state) const {
            return deque.empty() ? false : (deque.back() == state);
        }

        /* (idle)___(trigger)__/''(hold)''''(release)''\__
        bool is_idle()      const { return transition.previous == transition.current; }
        bool is_triggered() const { return transition.previous == transition.current; }
        bool is_hold()      const { return transition.previous == transition.current; }
        bool is_released()  const { return transition.previous == transition.current; } */

        // setup
        fsm::call& on(const fsm::state& from, const fsm::state& to) {//返回表中绑定的一个匿名函数
            return callbacks[bistate(from, to)];
        }

        // generic call
        bool call(const fsm::state& from, const fsm::state& to) const {
            std::map< bistate, fsm::call >::const_iterator found = callbacks.find(bistate(from, to));
            if (found != callbacks.end()) {
                log.push_back({ from, current_trigger, to });
                if (log.size() > 50) {
                    log.pop_front();
                }
                found->second(to.args);
                return true;
            }
            return false;
        }

        // user commands
        bool command(const fsm::state& trigger) {//执行trigger 函数
            size_t size = this->size();
            if (!size) {
                return false;
            }
            current_trigger = fsm::state();
            std::deque< states::reverse_iterator > aborted;
            for (auto it = deque.rbegin(); it != deque.rend(); ++it) {
                fsm::state& self = *it;
                if (!call(self, trigger)) {
                    aborted.push_back(it);
                    continue;
                }
                for (auto it = aborted.begin(), end = aborted.end(); it != end; ++it) {
                    call(**it, 'quit');
                    deque.erase(--(it->base()));
                }
                current_trigger = trigger;
                return true;
            }
            return false;
        }
        template<typename T>
        bool command(const fsm::state& trigger, const T& arg1) {
            return command(trigger(arg1));
        }
        template<typename T, typename U>
        bool command(const fsm::state& trigger, const T& arg1, const U& arg2) {
            return command(trigger(arg1, arg2));
        }

        // debug
        template<typename ostream>
        ostream& debug(ostream& out) {
            int total = log.size();
            out << "status {" << std::endl;
            std::string sep = "\t";
            for (states::const_reverse_iterator it = deque.rbegin(), end = deque.rend(); it != end; ++it) {
                out << sep << *it;
                sep = " -> ";
            }
            out << std::endl;
            out << "} log (" << total << " entries) {" << std::endl;
            for (int i = 0; i < total; ++i) {
                out << "\t" << log[i] << std::endl;
            }
            out << "}" << std::endl;
            return out;
        }

        // aliases
        bool operator()(const fsm::state& trigger) {//执行带参数的函数
            return command(trigger);
        }
        template<typename T>
        bool operator()(const fsm::state& trigger, const T& arg1) {
            return command(trigger(arg1));
        }
        template<typename T, typename U>
        bool operator()(const fsm::state& trigger, const T& arg1, const U& arg2) {
            return command(trigger(arg1, arg2));
        }
        template<typename ostream>
        inline friend ostream& operator<<(ostream& out, const stack& t) {
            return t.debug(out), out;
        }

    protected:

        void replace(fsm::state& current, const fsm::state& next) {//替换某一状态
            call(current, 'quit');
            current = next;
            call(current, 'init');
        }

        typedef std::pair<int, int> bistate;
        std::map< bistate, fsm::call > callbacks;

        mutable std::deque< fsm::transition > log;
        std::deque< fsm::state > deque;//状态列表，最后一个为当前状态，前面的为暂停的状态
        fsm::state current_trigger;//当前状态

        typedef std::deque< fsm::state > states;
    };
}