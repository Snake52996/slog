_Pragma("once");
#include<string>
using namespace std;
class Cat{
	private:
		const string name_ = "Aailo";
		const unsigned int weight_ = 50;
		const unsigned int height_ = 150;
	public:
		template<typename OUT_STREAM>
		friend OUT_STREAM& operator<< (OUT_STREAM& os, const Cat& output_object);
}cat;
template<typename OUT_STREAM>
OUT_STREAM& operator<< (OUT_STREAM& os, const Cat& output_object){
	os<<"name: "<<output_object.name_<<", height: "<<output_object.height_<<", weight: "<<output_object.weight_;
	return os;
}
string HelloWorld = "Hello world from slog";
unsigned int ui_data = 122;
int i_data = 42;
double d_data = 12.7;
char c_data[] = "字符串数据";
