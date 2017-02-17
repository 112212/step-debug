#ifndef STEP_DEBUG_HPP
#define STEP_DEBUG_HPP
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <set>
#include <functional>
#include <condition_variable>
namespace sdb {

class Stage;
class Debug {
	typedef std::function<void(std::string, std::string, bool)> StageCallbackType;
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
};

class Stage {
	private:
		bool excluded;
		std::string name;
		Debug* d;
	public:
		Stage(std::string name, std::string msg="");
		~Stage();
		void Msg(std::string msg);
		void Break();
		void End();
};
}
#endif
