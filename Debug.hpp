#ifndef STEP_DEBUG_HPP
#define STEP_DEBUG_HPP
#include <string>
#include <sstream>
#include <map>
#include <mutex>
#include <vector>
#include <set>
#include <functional>
#include <condition_variable>
namespace sdb {
		
// ----- ugly but useful templates -----
template< typename t, typename = void >
struct function_type_information;

template< typename r, typename c, typename ... a >
struct function_type_information< r (c::*)( a ... ) const >
	: function_type_information< r (*)( a ... ) > {};
	
template< typename r, typename ... a >
struct function_type_information< r (*)( a ... )> {
	using tuple = std::tuple<a...>;
	using result = r;
};

template< typename ftor >
struct function_type_information< ftor, typename std::conditional< false, decltype( & ftor::operator () ), void >::type >
	: function_type_information< decltype( & ftor::operator () )> {};

template <typename F, typename Tuple, std::size_t... Is>
void tuple_call(F f, Tuple && t, std::index_sequence<Is...> is) {
	f(std::get<Is>( std::forward<Tuple>(t) )...);
}

template <typename F, typename Tuple>
void call(F f, Tuple && t) {
	using ttype = typename std::decay<Tuple>::type;
	tuple_call(f, std::forward<Tuple>(t), std::make_index_sequence<std::tuple_size<ttype>::value>{});
}
// ----------------------------------------

class Stage;
class Debug {
	typedef std::function<void(std::string, std::string, bool)> StageCallbackType;
	typedef std::function<void(const void*)> ReportCallbackType;
	friend Stage;
	private:
		bool enabled;
		bool skip_breakpoints;
		bool in_progress;
		bool control;
		std::condition_variable cv;
		std::mutex mtx;
		std::vector<std::string> messages;
		StageCallbackType stage_callback;
		std::set<std::string> excluded_stages;
		void giveUpControl();
		std::map<std::string, ReportCallbackType> report_callbacks;
	public:
		Debug();
		void Continue(bool skip_breakpoints=false);
		void DebugThis(std::function<void()> debug_this);
		std::vector<std::string> GetMessages();
		void Enable();
		void Disable();
		void StageCallback(StageCallbackType callback);
		bool Running();
		void ExcludeStage(std::string name);
		
		template<class T>
		void ReportCallback(std::string name, T callback) {
			auto adapter = [&](const void* data) {
				typename function_type_information<T>::tuple t = *(typename function_type_information<T>::tuple *)data;
				call(callback, t);
			};
			report_callbacks[name] = adapter;
		}
};

class Stage {
	private:
		bool excluded;
		bool no_breakpoints;
		std::string name;
		Debug* d;
		
		
		void msg(std::stringstream& msg);
		
		template<class T>
		void msgvariadic(std::stringstream& o, T a) {
			o << a;
		}
		template<class T, class... A>
		void msgvariadic(std::stringstream& o, T a, A... b) {
			o << a;
			msgvariadic(o, b...);
		}
	public:
		Stage(std::string name, std::string msg="", bool no_breakpoints=false);
		~Stage();
		
		template<class... A>
		void Msg(A... m) {
			std::stringstream s;
			msgvariadic(s, m...);
			msg(s);
		}
		
		template<class... A>
		void Report(std::string name, A... a) {
			if(excluded) return;
			std::tuple<A...> t(a...);
			auto it = d->report_callbacks.find(name);
			if(it == d->report_callbacks.end()) return;
			it->second(&t);
		}
		void Break();
		void End();
};
}
#endif
