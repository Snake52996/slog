/**
 * @brief 超轻型日志工具
*/
_Pragma("once");
#include<fstream>
#include<iostream>
#include<string>
#include<algorithm>
#include<sstream>
using namespace std;
namespace SnakeLog{
    /**
     * @brief 定义日志等级
    */
    enum class LogLevel{
        INFO,       ///< 普通信息
        DEBUG,      ///< 调试信息
        WARNING,    ///< 警告信息
        ERROR,      ///< 错误信息
        FATAL       ///< 致命错误信息
    };
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
        protected:
            LogLevel log_level_;
            string logger_name_;
            OUT_TARGET_T output_target_;
            stringstream buf_;
        public:
            BasicLog() = delete;
            BasicLog(const string& using_path, const LogLevel& log_level = LogLevel::INFO, const string& logger_name = ""){
                OUT_TARGET_T temp_target(using_path);
                this->output_target_ = move(temp_target);
                this->log_level_ = log_level;
                this->logger_name_ = logger_name;
            }
            inline const LogLevel& level()const noexcept{return this->log_level_;}
            inline void level(const LogLevel& log_level)noexcept{this->log_level_ = log_level;}
            inline const string& name()const noexcept{return this->logger_name_;}
            inline void name(const string& logger_name)noexcept{this->logger_name_ = logger_name;}
            void log(const char* output_string){
                buf_<<output_string;
                output_target_<<buf_.str();
                buf_.str("");
            }
            template<typename T>
            void log(const char* format, const T& message){
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
                output_target_<<buf_.str();
                buf_.str("");
            }
            template<typename T, typename... ARG>
            void log(const char* format, const T& first_value, ARG...args){
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
                buf_.str("");
            }
            template<typename... ARG>
            void info(const char* format, ARG...args){
                if(this->log_level_ > LogLevel::INFO) return;
                if(this->logger_name_ != "") buf_<<"[ "<<this->logger_name_<<" | I ] ";
                else buf_<<"[ I ] ";
                return this->log(format, args...);
            }
            template<typename... ARG>
            void debug(const char* format, ARG...args){
                if(this->log_level_ > LogLevel::DEBUG) return;
                if(this->logger_name_ != "") buf_<<"[ "<<this->logger_name_<<" | D ] ";
                else buf_<<"[ D ] ";
                return this->log(format, args...);
            }
            template<typename... ARG>
            void warning(const char* format, ARG...args){
                if(this->log_level_ > LogLevel::WARNING) return;
                if(this->logger_name_ != "") buf_<<"[ "<<this->logger_name_<<" | W ] ";
                else buf_<<"[ W ] ";
                return this->log(format, args...);
            }
            template<typename... ARG>
            void error(const char* format, ARG...args){
                if(this->log_level_ > LogLevel::ERROR) return;
                if(this->logger_name_ != "") buf_<<"[ "<<this->logger_name_<<" | E ] ";
                else buf_<<"[ E ] ";
                return this->log(format, args...);
            }
            template<typename... ARG>
            void fatal(const char* format, ARG...args){
                if(this->log_level_ > LogLevel::FATAL) return;
                if(this->logger_name_ != "") buf_<<"[ "<<this->logger_name_<<" | F ] ";
                else buf_<<"[ F ] ";
                return this->log(format, args...);
            }
    };
   typedef BasicLog<Console> ConsoleLog;
   typedef BasicLog<ofstream> FileLog;
   typedef BasicLog<LoopLogFile> LoopFileLog;
}