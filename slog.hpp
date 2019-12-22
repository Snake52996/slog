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
#include<algorithm>
#include<sstream>
#include<ctime>         // tm time_t time() localtime() strftime()
#include<cstring>       // strcmp
#include<type_traits>   // is_same
using namespace std;
namespace SnakeLog{
    /**
     * @brief 定义日志等级
    */
    enum class LogLevel{
        INFO,       ///< 普通信息
        WARNING,    ///< 警告信息
        ERROR,      ///< 错误信息
        FATAL,      ///< 致命错误信息
        SILENCE     ///< 不输出错误信息
    };
    static string __getLocalTime(){
        auto time0 = time(NULL);
        auto tm_time = *localtime(&time0);
        char temp_str[512];
        strftime(temp_str, 512, "%Y-%m-%d %H:%M:%S", &tm_time);
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
            LoopLogFile() = default;
            LoopLogFile(const string& working_dir){
                this->working_dir_ = working_dir;
                this->current_index_ = 0;
                this->out_file_.open(this->working_dir_ + "log." + to_string(current_index_));
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
            string working_dir_;
            ofstream* output_file_ = nullptr;
            std::tm file_time_;
            char file_time_buf_[512];
            char current_time_buf_[512];
            void updateFileTime(){
                auto time0 = time(NULL);
                file_time_ = *localtime(&time0);
                strftime(file_time_buf_, 512, "%Y-%m-%d", &file_time_);
            }
            bool isSameDay(){
                auto time0 = time(NULL);
                auto current_tm = *localtime(&time0);
                strftime(current_time_buf_, 512, "%Y-%m-%d", &current_tm);
                return (strcmp(current_time_buf_, file_time_buf_) == 0);
            }
            void prepareFile(){
                if(output_file_ != nullptr) return;
                this->updateFileTime();
                ifstream control_file(working_dir_ + ".new");
                if(!control_file.is_open()){
                    ofstream out_control_file(working_dir_ + ".new");
                    out_control_file<<file_time_buf_;
                    out_control_file.close();
                    out_control_file.open(working_dir_ + ".index", ios::app);
                    out_control_file<<file_time_buf_<<endl;
                    out_control_file.close();
                }else{
                    string tmp_date;
                    control_file>>tmp_date;
                    if(tmp_date != file_time_buf_){
                        ofstream out_control_file(working_dir_ + ".new");
                        out_control_file<<file_time_buf_;
                        out_control_file.close();
                        out_control_file.open(working_dir_ + ".index", ios::app);
                        out_control_file<<file_time_buf_<<endl;
                        out_control_file.close();
                    }
                }
                output_file_ = new ofstream(working_dir_ + file_time_buf_, ios::app);
            }
        public:
            DailyLogFile() = default;
            DailyLogFile(const string& working_dir){
                working_dir_ = working_dir;
            }
            ~DailyLogFile(){
                if(output_file_ == nullptr) return;
                if(output_file_->is_open()){
                    output_file_->close();
                }
                delete output_file_;
                output_file_ = nullptr;
            }
            template<typename T>
            DailyLogFile& operator<<(const T& output_message){
                if(output_file_ == nullptr) this->prepareFile();
                else{
                    // 检查时间
                    if(!isSameDay()){
                        output_file_->close();
                        this->updateFileTime();
                        ofstream control_file(working_dir_ + ".new");
                        control_file<<file_time_buf_;
                        control_file.close();
                        control_file.open(working_dir_ + ".index", ios::app);
                        control_file<<file_time_buf_<<endl;
                        control_file.close();
                        output_file_->open(working_dir_ + file_time_buf_, ios::app);
                    }
                }
                if(!output_file_->is_open()){
                    cerr<<"文件打开失败.\n";
                    return *this;
                }
                (*output_file_)<<output_message;
                output_file_->flush();
                return *this;
            }
    };
    /**
     * @brief 封装cout
     * 提供一致化的输出方式并不对cout造成非预期的更改
    */
    class Console{
        public:
            Console() = default;
            Console(const string& meaningless){;}
            template<typename T>
            Console& operator<< (const T& output_data){
                cout<<output_data;
                return *this;
            }
    };
    template<typename OUT_TARGET_T>
    class BasicLog{
        private:
            bool is_first_ = true;
            #ifdef _LINUX
            bool is_colored_ = false;
            #endif
        protected:
            LogLevel log_level_;
            string logger_name_;
            bool show_time_;
            OUT_TARGET_T output_target_;
            stringstream buf_;
            bool is_console_ = false;
        public:
            BasicLog() = delete;
            BasicLog(const string& using_path, const LogLevel& log_level = LogLevel::INFO, const string& logger_name = "", const bool& show_time = true){
                string temp = using_path;
                if(!is_same<OUT_TARGET_T, ofstream>()) if(using_path.back() != '/') temp.push_back('/');
                OUT_TARGET_T temp_target(temp);
                this->output_target_ = move(temp_target);
                this->log_level_ = log_level;
                this->logger_name_ = logger_name;
                this->show_time_ = show_time;
                if(is_same<OUT_TARGET_T, Console>()) this->is_console_ = true;
            }
            inline const LogLevel& level()const noexcept{return this->log_level_;}
            inline void level(const LogLevel& log_level)noexcept{this->log_level_ = log_level;}
            inline const string& name()const noexcept{return this->logger_name_;}
            inline void name(const string& logger_name)noexcept{this->logger_name_ = logger_name;}
            inline bool showTime()const noexcept{return this->show_time_;}
            inline void showTime(const bool& show_time)noexcept{this->show_time_ = show_time;}
            void log(const char* output_string){
                if(is_first_){
                    if(logger_name_.size()) buf_<<"["<<logger_name_<<"]";
                    if(show_time_) buf_<<"["<<__getLocalTime()<<"]";
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
                    if(show_time_) buf_<<"["<<__getLocalTime()<<"]";
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
            void log(const char* format, const T& first_value, ARG...args){
                if(is_first_){
                    if(logger_name_.size()) buf_<<"["<<logger_name_<<"]";
                    if(show_time_) buf_<<"["<<__getLocalTime()<<"]";
                    is_first_ = false;
                }
                while(*format){
                    if(*format == '{' && *(format + 1) == '}'){
                        // 输出变量
                        buf_<<first_value;
                        // 跳过"{}"
                        return this->log(format + 2, args...);
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
            template<typename... ARG>
            void info(const char* format, ARG...args){
                if(this->log_level_ > LogLevel::INFO) return;
                #ifdef _LINUX
                if(this->is_console_){
                    buf_<<"\033[37m";
                    is_colored_ = true;
                }
                #endif
                buf_<<"[I]";
                is_first_ = true;
                return this->log(format, args...);
            }
            template<typename... ARG>
            void warning(const char* format, ARG...args){
                if(this->log_level_ > LogLevel::WARNING) return;
                #ifdef _LINUX
                if(this->is_console_){
                    buf_<<"\033[35m";
                    is_colored_ = true;
                }
                #endif
                buf_<<"[W]";
                is_first_ = true;
                return this->log(format, args...);
            }
            template<typename... ARG>
            void error(const char* format, ARG...args){
                if(this->log_level_ > LogLevel::ERROR) return;
                #ifdef _LINUX
                if(this->is_console_){
                    buf_<<"\033[33m";
                    is_colored_ = true;
                }
                #endif
                buf_<<"[E]";
                is_first_ = true;
                return this->log(format, args...);
            }
            template<typename... ARG>
            void fatal(const char* format, ARG...args){
                if(this->log_level_ > LogLevel::FATAL) return;
                #ifdef _LINUX
                if(this->is_console_){
                    buf_<<"\033[31m";
                    is_colored_ = true;
                }
                #endif
                buf_<<"[F]";
                is_first_ = true;
                return this->log(format, args...);
            }
    };
   typedef BasicLog<Console> ConsoleLog;
   typedef BasicLog<ofstream> FileLog;
   typedef BasicLog<LoopLogFile> LoopFileLog;
   typedef BasicLog<DailyLogFile> DailyLog;
}