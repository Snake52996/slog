#include "testdefine.hpp"
#include "../slog.hpp"
#include<iostream>
#include<string>
using namespace std;
using namespace SnakeLog;
int main(){
    DailyLog a(LogLevel::VERBOSE, "", "", ".");
    a.verbose("{}\n", HelloWorld);
    a.debug("无符号整数:{}和有符号整数:{}\n", ui_data, i_data);
    a.info("浮点数:{}\n", d_data);
    a.warning("C-style字符串:{}\n", c_data);
    a.error("混合在一起:{} + {} + {} = {}, {}\n", ui_data, i_data, d_data, ui_data + i_data + d_data, "回答正确");
    a.fatal("自定义类型:{}\n", cat);
    return 0;
}
