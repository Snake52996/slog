/**
 * @brief 超轻型日志工具
*/
_Pragma("once");
#if defined(_WIN32) || defined(_WIN64)
	#define _WINDOWS
#elif defined(__linux) || defined(__linux__)
	#define _LINUX
#endif
#include<fstream>
#include<iostream>
#include<string>
#include<algorithm>     // forward() move()
#include<sstream>
#include<ctime>         // tm time_t time() localtime() strftime()
#include<cstring>       // strcmp
#include<type_traits>   // is_same
using namespace std;
namespace SnakeLog{
    // 定义终端颜色代码
    #ifdef _LINUX
        static constexpr char LEVEL_COLOR_CODE[][6] = {"", "\033[34m", "\033[36m", "\033[37m", "\033[35m", "\033[33m", "\033[31m", ""};
    #else
        static constexpr char LEVEL_COLOR_CODE[][6] = {"", "", "", "", "", "", "", ""};
    #endif
    // 定义各日志等级的简称
    static constexpr char SHORT_LEVEL_CODE[][4] = {"", "[V]", "[D]", "[I]", "[W]", "[E]", "[F]", ""};
    // 宏定义几种常见的时间格式
    #define LOG_TIME_FULL_TIME "%Y-%m-%d %H:%M:%S"  ///< 完整时间:年月日时分秒
    #define LOG_TIME_DATE_TIME "%Y-%m-%d"           ///< 日期时间:年月日
    #define LOG_TIME_CLOCK_TIME "%H:%M:%S"          ///< 时钟时间:时分秒
    /**
     * @brief 定义日志等级
    */
    enum class LogLevel{
        ALL,        ///< 输出全部的日志信息
        VERBOSE,    ///< 最多的信息
        DEBUG,      ///< 调试信息
        INFO,       ///< 普通信息
        WARNING,    ///< 警告信息
        ERROR,      ///< 错误信息
        FATAL,      ///< 致命错误信息
        SILENCE     ///< 不输出任何日志信息
    };
    template<typename T>
    static string __getLocalTime(const T& format){
        auto time0 = time(NULL);
        auto tm_time = *localtime(&time0);
        char temp_str[512];
        strftime(temp_str, 512, format, &tm_time);
        string temp_string = temp_str;
        return temp_string;
    }
    /**
     * @brief 封装循环日志文件
     * 提供与ofstream完全一致的<<操作符操作,在接受一次输出之前检查当前文件的大小,若大于限定则轮转文件.覆盖最老的文件(若文件数量已达规定的上限)
    */
    class LoopLogFile{
        private:
            unsigned int max_size_ = 5 * 1024 * 1024;   ///< 单个文件的最大大小.
            unsigned int max_file_number_ = 5;          ///< 最大创建文件的数量
            ofstream out_file_;                         ///< 输出文件
            string working_dir_;                        ///< 当前工作路径
            unsigned int current_index_;                ///< 当前的文件序号
        public:
            LoopLogFile() = delete;
            /**
              * @brief 构造循环文件
              * @param[in] working_dir 输入的工作路径，可以是任何可以隐式转换为string的类型
              * @pre working_dir应当总是以'/'结尾，并表示一个已存在的且有写权限的文件夹
              * @warning 不会检查working_dir的合法性
             */
             template<typename STRING_T>
             LoopLogFile(const STRING_T& working_dir){
                this->working_dir_ = working_dir;
                if(working_dir_.back() != '/') working_dir_ += "/";     // 若路径末尾没有'/'则添加之
                this->current_index_ = 0;
                this->out_file_.open(this->working_dir_ + to_string(current_index_) + ".log");
             }
             ~LoopLogFile(){
                 if(out_file_.is_open()) out_file_.close();
             }
            /**
             * @brief 重载<<运算符提供与标准库一致的输出操作
             * @warning 并不保证任何文件均不大于最大单个文件大小.文件更改仅发生在两次文件写入之间.
            */
            template<typename T>
            LoopLogFile& operator<< (const T& output_data){
                // 先检查文件是否需要轮转
                // 保证被覆盖的文件(如果存在)中总是会被写入新内容.避免空文件覆盖老文件
                if(this->out_file_.tellp() >= this->max_size_){     // 需要轮转
                    // 关闭文件
                    this->out_file_.close();
                    // 转换当前文件编号
                    current_index_ = (current_index_ + 1) % max_file_number_;
                    // 打开文件
                    this->out_file_.open(this->working_dir_ + to_string(current_index_));
                }
                if(!this->out_file_.is_open()){
                    cerr<<"日志文件打开失败.\n";
                    return *this;
                }
                this->out_file_<<output_data;
                return *this;
            }
    };
    class DailyLogFile{
        private:
            string working_dir_;                   ///< 工作路径
            ofstream output_file_;                 ///< 输出文件
            char last_output_time_buf_[20];        ///< 最近一次输出时的日期(char字符串格式)
            char current_output_time_buf_[20];     ///< 当前一次输出时的日期(char字符串格式)
            /**
             * @brief 将指定的字符串更新为当前的日期
             * @param[out] target_buffer 指定的C-style字符串
             * @pre 假定提供的字符串具有足够的空间
            */
            void updateTimeBuffer(char*& target_buffer){
                auto time0 = time(NULL);
                auto current_tm = *localtime(&time0);
                strftime(target_buffer, 512, "%Y-%m-%d", &current_tm);
            }
            bool isSameDay(){
                updateTimeBuffer(current_output_time_buf_);     // 更新当前时间
                return (strcmp(current_output_time_buf_, last_output_time_buf_) == 0);
            }
            /**
             * @brief 更新控制文件(.index和.new)中的数据
             * @note 在.index后追加一行当前的日期,将.new中的数据更换为当前的日期
            */
            void updateSavedTime(){
                ofstream control_file(working_dir_ + ".new");
                control_file<<current_output_time_buf_;
                control_file.close();
                control_file.open(working_dir_ + ".index", ios::app);
                control_file<<current_output_time_buf_<<endl;
                control_file.close();
            }
        public:
            DailyLogFile() = delete;
            template<typename STRING_T>
            DailyLogFile(const STRING_T& working_dir):output_file_(){
                working_dir_ = working_dir;
                if(working_dir_.back() != '/') working_dir_ += "/";
                ifstream control_file(working_dir_ + ".new");
                if(control_file.is_open()){
                    control_file>>last_output_time_buf_;
                    control_file.close();
                }
            }
            ~DailyLogFile(){
                if(output_file_.is_open()){
                    output_file_.close();
                }
            }
            template<typename T>
            DailyLogFile& operator<<(const T& output_message){
                if(!isSameDay()){   // 当前的日期和上一次输出不同
                    if(output_file_.is_open()) output_file_.close();
                    // 现在输出文件已经关闭
                    updateSavedTime();  // 更新控制文件
                    strncpy(last_output_time_buf_, current_output_time_buf_, 20);   // 更新上一次输出时间
                }
                // 现在:或者当前的日期和上一次输出的日期一致,否则则保证文件已经关闭,则此时若输出文件未被打开,总应当用当前日期打开之
                if(!output_file_.is_open()) output_file_.open(working_dir_ + current_output_time_buf_, ios::app);
                if(!output_file_.is_open()){    // 文件仍未被打开
                    cerr<<"文件打开失败.\n";
                    return *this;
                }
                output_file_<<output_message;
                return *this;
            }
    };
    /**
     * @brief 封装cout
     * 提供一致化的输出方式并不对cout造成非预期的更改
    */
    class Console{
        public:
            Console(){;}
            template<typename T>
            Console& operator<< (const T& output_data){
                cout<<output_data;
                return *this;
            }
    };
    template<typename OUT_TARGET_T>
    class BasicLog{
        private:
            bool is_first_ = true;          ///< 当前是否为同一次输出中的第一部分
            bool is_colored_ = false;       ///< 当前是否已被染色
        protected:
            LogLevel log_level_;
            string logger_name_;
            string time_format_;
            OUT_TARGET_T output_target_;
            stringstream buf_;
            bool is_console_ = false;
        public:
            BasicLog() = delete;
            template<typename STRING_TYPE_1, typename STRING_TYPE_2, typename...ARG>
            BasicLog(const LogLevel& log_level, const STRING_TYPE_1& logger_name, const STRING_TYPE_2& time_format, ARG&&...args):output_target_(forward<ARG>(args)...){
                log_level_ = log_level;
                logger_name_ = logger_name;
                time_format_ = time_format;
                is_console_ = is_same<OUT_TARGET_T, Console>();
            }
            ~BasicLog(){}
            inline const LogLevel& level()const noexcept{return this->log_level_;}
            inline void level(const LogLevel& log_level)noexcept{this->log_level_ = log_level;}
            inline const string& name()const noexcept{return this->logger_name_;}
            /**
             * @brief 设置日志器名称
             * @param[in] log_level 新的日志器名称.可以是任何可以隐式转换为string的类型.留空则不输出
            */
            template<typename STRING_TYPE>
            inline void name(const STRING_TYPE& logger_name)noexcept{this->logger_name_ = logger_name;}
            /**
             * @brief 获取当前的时间格式
             * @return 返回当前的时间格式
            */
            inline const string& timeFormat()const noexcept{return this->time_format_;}
            /**
             * @brief 设置时间格式
             * @param[in] log_level 新的时间格式.可以是任何可以隐式转换为string的类型.留空则不输出
            */
            template<typename STRING_TYPE>
            inline void timeFormat(const STRING_TYPE& time_format)noexcept{this->time_format_ = time_format;}
            template<typename T>
            void log(const T& output_string){
                if(is_first_){
                    if(logger_name_.size()) buf_<<"["<<logger_name_<<"]";
                    if(time_format_.size()) buf_<<"["<<__getLocalTime(time_format_.c_str())<<"]";
                }
                buf_<<output_string;
                #ifdef _LINUX
                if(is_colored_){
                    buf_<<"\033[0m";
                    is_colored_ = false;
                }
                #endif
                output_target_<<buf_.str();
                buf_.str("");
            }
            template<typename T>
            void log(const char* format, const T& message){
                if(is_first_){
                    if(logger_name_.size()) buf_<<"["<<logger_name_<<"]";
                    if(time_format_.size()) buf_<<"["<<__getLocalTime(time_format_.c_str())<<"]";
                    is_first_ = false;
                }
                bool var_outputted = false;
                while(*format){
                    if(*format == '{' && *(format + 1) == '}'){
                        // 输出变量
                        buf_<<message;
                        var_outputted = true;
                        // 跳过"{}"
                        ++format;
                    }else if(*format == '{' && *(format + 1) == '{'){
                        buf_<<*format;
                        ++format;
                    }else{
                        buf_<<*format;
                    }
                    ++format;
                }
                if(!var_outputted){
                    cerr<<"提供了多余的参数.该参数为:\n"<<message;
                }
                #ifdef _LINUX
                if(is_colored_){
                    buf_<<"\033[0m";
                    is_colored_ = false;
                }
                #endif
                output_target_<<buf_.str();
                buf_.str("");
                is_first_ = true;
            }
            template<typename T, typename... ARG>
            void log(const char* format, const T& first_value, ARG&&...args){
                if(is_first_){
                    if(logger_name_.size()) buf_<<"["<<logger_name_<<"]";
                    if(time_format_.size()) buf_<<"["<<__getLocalTime(time_format_.c_str())<<"]";
                    is_first_ = false;
                }
                while(*format){
                    if(*format == '{' && *(format + 1) == '}'){
                        // 输出变量
                        buf_<<first_value;
                        // 跳过"{}"
                        return this->log(format + 2, forward<ARG>(args)...);
                    }else if(*format == '{' && *(format + 1) == '{'){
                        buf_<<*format;
                        ++format;
                    }else{
                        buf_<<*format;
                    }
                    ++format;
                }
                cerr<<"提供了过多的多余的参数.拒绝打印该日志.\n";
                #ifdef _LINUX
                is_colored_ = false;
                #endif
                buf_.str("");
            }
            template<typename T, typename... ARG>
            void metaLog(const LogLevel& level, const T& format, ARG&&... args){
                if(log_level_ > level) return;
                if(is_console_){    // 是在向控制台输出
                    buf_<<LEVEL_COLOR_CODE[int(level)];     // 染色
                    is_colored_ = true;                     // 标记已染色
                }
                buf_<<SHORT_LEVEL_CODE[int(level)];         // 添加等级标识
                is_first_ = true;                           // 标记为起始
                return log(format, forward<ARG>(args)...);  // 转发继续处理
            }
            template<typename T, typename... ARG>
            void verbose(const T& format, ARG&&...args){
                metaLog(LogLevel::VERBOSE, format, forward<ARG>(args)...);
            }
            template<typename T, typename... ARG>
            void debug(const T& format, ARG&&...args){
                metaLog(LogLevel::DEBUG, format, forward<ARG>(args)...);
            }
            template<typename T, typename... ARG>
            void info(const T& format, ARG&&...args){
                metaLog(LogLevel::INFO, format, forward<ARG>(args)...);
            }
            template<typename T, typename... ARG>
            void warning(const T& format, ARG&&...args){
                metaLog(LogLevel::WARNING, format, forward<ARG>(args)...);
            }
            template<typename T, typename... ARG>
            void error(const T& format, ARG&&...args){
                metaLog(LogLevel::ERROR, format, forward<ARG>(args)...);
            }
            template<typename T, typename... ARG>
            void fatal(const T& format, ARG&&...args){
                metaLog(LogLevel::FATAL, format, forward<ARG>(args)...);
            }
    };
   typedef BasicLog<Console> ConsoleLog;
   typedef BasicLog<ofstream> FileLog;
   typedef BasicLog<LoopLogFile> LoopFileLog;
   typedef BasicLog<DailyLogFile> DailyLog;
}