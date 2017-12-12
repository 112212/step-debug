#include "Debug.hpp"
#include <iostream>

int main() {
	
	sdb::Debug dbg;
	dbg.StageCallback([](std::string n, std::string msg, bool isnew){
		std::cout << "STAGE: " << n  << " : \"" << msg << "\"" << (isnew ? " BEGIN " : " END ") << "\n";
	});
	
	dbg.ExcludeStage("first");
	
	dbg.DebugThis([&]{
		sdb::Stage s("first", "helloworld debug");
		
		int sum = 0;
		for(int i=0; i < 100; i++) {
			sum += i;
			if(i % 10 == 0) {
				s.Msg("value is: " + std::to_string(i));
				s.Break();
			}
		}
		
		s.Msg("Hello World\n");
		
		s.Break();
		
		s.Msg("break me again");
		
		s.Break();
		
		s.End();
		
		sdb::Stage s1("second", "second stage description");
		s1.Msg("blablabla");
		s1.Break();
		
	});
	
	
	while(dbg.Running()) {
		for(const sdb::Message& s : dbg.GetMessages()) {
			
			std::cout << s.first << ": " << s.second << "\n";
		}
		std::cout << "breakpoint! continue?\n";
		std::string s;
		std::cin >> s;
		dbg.Continue();
	}
	
}
