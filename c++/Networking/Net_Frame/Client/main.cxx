#include <iostream>
#include <snt_net.h>



enum class CustomMsgTypes : uint32_t{
  ServerAccept,
  ServerDeny,
  ServerPing,
  SendFile,
  ServerMessage,
};

class CustomClient: public snt::net::client_interface<CustomMsgTypes>{
public:
  void PingServer(){
    snt::net::message<CustomMsgTypes> msg;
    msg.header.id = CustomMsgTypes::ServerPing;

    // ??
    std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

    msg << timeNow;
    Send(msg);
  }

  void SendFile(){
    snt::net::message<CustomMsgTypes> msg;
    msg.header.id = CustomMsgTypes::SendFile;
    /// Load File
    std::string fileName = "File";
    std::ifstream inputFile(fileName, std::ios::in | std::ios::binary);
    /// Open file
    std::vector<char> inputBuffer;
    /// Get length of file
    inputFile.seekg(0, inputFile.end);
    size_t length = inputFile.tellg();
    /// Put the pointer back to the begining
    inputFile.seekg(0, inputFile.beg);
    /// Read file
    if (length > 0) {
        inputBuffer.resize(length);
        inputFile.read(&inputBuffer[0], length);
    }
    inputFile.close();
    /// Insert file into message
    msg.payload.resize(inputBuffer.size());
    std::copy(inputBuffer.begin(),inputBuffer.end(),msg.payload.data());
    msg.header.size = inputBuffer.size();
    /// Send message
    Send(msg);
  }

};


int main()
{
  CustomClient c;
  c.Connect("127.0.0.1",60000);

  bool key[3] = {false,false,false};
  bool old_key[3] = {false,false,false};


  bool bQuit = false;
  while(!bQuit){
      if(GetForegroundWindow() == GetConsoleWindow() ){
          key[0] = GetAsyncKeyState('1') & 0X8000;
          key[1] = GetAsyncKeyState('2') & 0X8000;
          key[2] = GetAsyncKeyState('3') & 0X8000;
        }

      if(key[0] && !old_key[0]) c.PingServer();
      if(key[1] && !old_key[1]) c.SendFile();
      if(key[2] && !old_key[2]) bQuit = true;

      for(int i = 0; i < 3; i++) old_key[i] = key[i];

      if(c.IsConnected()){
          if(!c.Incoming().empty()){
              auto msg = c.Incoming().pop_front().msg;

              switch (msg.header.id) {

                case CustomMsgTypes::ServerAccept:{
                    // Server has responded to a ping request
                    std::cout << "Server Accepted Connection\n";
                  }break;

                case CustomMsgTypes::ServerPing:
                  {

                    std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                    std::chrono::system_clock::time_point timeThen;

                    msg >> timeThen;

                    std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";

                  }break;
                case CustomMsgTypes::ServerMessage:{
                    // Server has responded to a ping request
                    uint32_t clientID;
                    msg >> clientID;
                    std::cout << "Hello from ["<< clientID << "]\n";
                  }break;
                default: {

                  }break;
               }

            }
        }else{
          std::cout << "Server Down\n";
          bQuit = true;
        }
    }
  std::cin.get();
  return 0;
}
