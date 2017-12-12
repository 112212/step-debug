#include "Debug.hpp"


#include <thread>
#include <chrono>

namespace sdb {
static std::map<std::thread::id, Debug*> thread_debug;

Debug::Debug() {
	in_progress = false;
	enabled = true;
	skip_n_breakpoints = 0;
	control = false;
}

void Debug::Continue(int skip_n_breakpoints) {
	this->skip_n_breakpoints = skip_n_breakpoints;
	giveUpControl();
}

void Debug::Enable() {
	enabled = true;
}
void Debug::Disable() {
	enabled = false;
}

void Debug::DebugThis(std::function<void()> debug_this) {
	if(in_progress) return;
	auto wrap = [&, debug_this]() {
		std::unique_lock<std::mutex> l(mtx);
		
		thread_debug[std::this_thread::get_id()] = this;
		
		debug_this();
		
		in_progress = false;
		control = false;
		skip_n_breakpoints = 0;
		l.unlock();
		cv.notify_all();
		
	};
	std::unique_lock<std::mutex> l(mtx);
	
	in_progress = true;
	std::thread t(wrap);
	t.detach();
	giveUpControl();
}

void Debug::giveUpControl() {
	bool my_control = control;
	control = !control;
	std::unique_lock<std::mutex> l(mtx, std::adopt_lock);
	cv.notify_all();
	cv.wait(l, [&]{return control == my_control;});
}

std::vector<Message> Debug::GetMessages() {
	std::vector<Message> msgs;
	msgs.swap(messages);
	return msgs;
}

void Debug::StageCallback(StageCallbackType callback) {
	stage_callback = callback;
}

bool Debug::Running() {
	return in_progress;
}

void Debug::ExcludeStage(std::string name, std::vector<std::string> v) {
	if(!v.empty()) {
		excluded_stage_reports[name].insert(v.begin(), v.end());
	} else {
		excluded_stages.insert(name);
	}
}

void Debug::IncludeStage(std::string name, std::vector<std::string> v) {
	if(!v.empty()) {
		for(auto &s : v) {
			excluded_stage_reports[name].erase(s);
		}
	} else {
		excluded_stages.erase(name);
	}
}


// =========== STAGE =============

Stage::Stage(std::string name, std::string msg, bool no_breakpoints) {
	auto dbg = thread_debug.find(std::this_thread::get_id());
	if(dbg == thread_debug.end() || !dbg->second->in_progress || !dbg->second->enabled) {
		excluded = true;
		return;
	}
	d = dbg->second;
	this->name = name;
	excluded = d->excluded_stages.count(name) != 0;
	if(!excluded && d->stage_callback) {
		d->stage_callback(name, msg, true);
	}
	this->no_breakpoints = no_breakpoints;
}

Stage::~Stage() {
	if(!excluded && d->stage_callback) {
		d->stage_callback(name, "", false);
	}
}

void Stage::msg(std::stringstream& msg) {
	if(excluded) return;
	auto message = std::make_pair(name, msg.str());
	Report("message", message);
	d->messages.push_back(message);
}

void Stage::Break() {
	if(excluded || no_breakpoints) return;
	if(d->skip_n_breakpoints>0) {
		d->skip_n_breakpoints--;
		return;
	}
	d->giveUpControl();
}

void Stage::End() {
	if(!excluded && d->stage_callback) {
		d->stage_callback(name, "", false);
	}
	excluded = true;
}

}
