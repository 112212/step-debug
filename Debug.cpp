#include "Debug.hpp"

#include <map>
#include <thread>
#include <chrono>

namespace sdb {
static std::map<std::thread::id, Debug*> thread_debug;

Debug::Debug() {
	in_progress = false;
	enabled = true;
	skip_breakpoints = false;
	control = false;
}

void Debug::Continue(bool skip_breakpoints) {
	this->skip_breakpoints = skip_breakpoints;
	giveUpControl();
}

void Debug::Enable() {
	enabled = true;
}
void Debug::Disable() {
	enabled = false;
}

void Debug::DebugThis(std::function<void()> debug_this) {
	auto wrap = [&]() {
		mtx.lock();
		thread_debug[std::this_thread::get_id()] = this;
		
		debug_this();
		
		in_progress = false;
		control = false;
		mtx.unlock();
		cv.notify_one();
		
	};
	mtx.lock();
	std::thread t(wrap);
	t.detach();
	in_progress = true;
	giveUpControl();
}

void Debug::giveUpControl() {
	bool my_control = control;
	control = !control;
	std::unique_lock<std::mutex> l(mtx, std::adopt_lock);
	l.unlock();
	cv.notify_one();
	cv.wait(l, [&]{return control == my_control;});
}

std::vector<std::string> Debug::GetMessages() {
	std::vector<std::string> msgs;
	msgs.swap(messages);
	return msgs;
}

void Debug::StageCallback(StageCallbackType callback) {
	stage_callback = callback;
}

bool Debug::Running() {
	return in_progress;
}

void Debug::ExcludeStage(std::string name) {
	excluded_stages.insert(name);
}


// =========== STAGE =============

Stage::Stage(std::string name, std::string msg) {
	auto dbg = thread_debug.find(std::this_thread::get_id());
	if(dbg == thread_debug.end()) {
		excluded = true;
		return;
	}
	d = dbg->second;
	this->name = name;
	excluded = d->excluded_stages.count(name) != 0;
	if(!excluded && d->stage_callback) {
		d->stage_callback(name, msg, true);
	}
}

Stage::~Stage() {
	if(!excluded && d->stage_callback) {
		d->stage_callback(name, "", false);
	}
}

void Stage::Msg(std::string msg) {
	if(excluded) return;
	d->messages.push_back(msg);
}

void Stage::Break() {
	if(excluded || d->skip_breakpoints) return;
	d->giveUpControl();
}

void Stage::End() {
	if(!excluded && d->stage_callback) {
		d->stage_callback(name, "", false);
	}
	excluded = true;
}

}
