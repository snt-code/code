#include <iostream>
#include <snt_net.h>


enum class CustomMsgTypes : uint32_t{
  ServerAccept,
  ServerDeny,
  ServerPing,
  SendFile,
  ServerMessage,
};


class CustomServer : public snt::net::server_interface<CustomMsgTypes>{
  public:
    CustomServer(uint16_t nPort):
      snt::net::server_interface<CustomMsgTypes>(nPort){

    }
  protected:
    virtual bool OnClientConnect(std::shared_ptr<snt::net::connection<CustomMsgTypes>> client){
      snt::net::message<CustomMsgTypes> msg;
      msg.header.id = CustomMsgTypes::ServerAccept;
      client->Send(msg);
      return true;
    }

    // Called when a client appears to have disconnected
    virtual void OnClientDisconnect(std::shared_ptr<snt::net::connection<CustomMsgTypes>> client){
      std::cout << "Removing client [" << client->GetID() << "]\n";
    }

    // Called when a message arrives
    virtual void OnMessage(std::shared_ptr<snt::net::connection<CustomMsgTypes>> client,
                           snt::net::message<CustomMsgTypes>& msg){
      switch(msg.header.id){
        case CustomMsgTypes::ServerPing:{
            std::cout << "[" << client->GetID() << "]: Server Ping\n";

            // Bounce-Back
            client->Send(msg);
          }break;
        case CustomMsgTypes::SendFile:{
            std::cout << "[" << client->GetID() << "]: Recieve File \n";
            std::vector<char> outBuffer;
            outBuffer.resize(msg.header.size);
            std::copy(msg.payload.begin(),msg.payload.end(),outBuffer.data());

            std::string fileName2 = "Output";
            std::ofstream output(fileName2, std::ios::out | std::ios::binary);
            output.write(&outBuffer[0],outBuffer.size());
            output.close();

          }break;
        default: {

          }break;
        }


    }

};


int main()
{
  CustomServer server(60000);
  server.Start();

  while(1){
      server.Update(-1,true);
  }
  return 0;
}
