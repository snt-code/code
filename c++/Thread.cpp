#include <iostream>
#include <thread>

static bool s_Finished = false;

void DoThings(){

	using namespace std::literals::chrono_literals;
	
	while(!s_Finished){
		std::cout << "Working... \n";
		std::this_thread::sleep_for(1s);
	}
}

int main(){
	std::thread Thread(DoThings);
	
	std::cin.get();
	s_Finished = true;
	
	Thread.join();
	std::cout << "Finished. \n";
	
	std::cin.get();
}
