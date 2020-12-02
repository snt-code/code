#include <iostream>
#include <future>

bool s_Finished = false;

int DoThingsA(){
	using namespace std::literals::chrono_literals;
	while(!s_Finished){
		std::cout << "Do Things A \n";
		std::this_thread::sleep_for(1s);
	}
	std::cout << "Do Things A finished\n";
	return 2;	
}

int DoThingsB(){
	using namespace std::literals::chrono_literals;
	while(!s_Finished){
		std::cout << "Do Things B \n";
		std::this_thread::sleep_for(1s);
	}
	std::cout << "Do Things B finished\n";
	return 3;	
}

int main(){
	std::future<int> futA = std::async(std::launch::async,DoThingsA);
	std::future<int> futB = std::async(std::launch::async,DoThingsB);
	
	std::cin.get();
	s_Finished = true;

	int resA = futA.get();
	int resB = futB.get();	
	std::cin.get();

}
