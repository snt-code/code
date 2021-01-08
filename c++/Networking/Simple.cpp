#include <iostream>

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

using namespace std;

std::vector<char> vBuffer(1*1024);

void GrabSomeData(asio::ip::tcp::socket& socket){

  socket.async_read_some(asio::buffer(vBuffer.data(),vBuffer.size()),
                         [&](std::error_code ec, std::size_t length){
                                if(!ec){
                                    std::cout<< "\n\nRead "<< length << " bytes\n\n";
                                    for(int i =0; i < length; i++)
                                      std::cout << vBuffer[i];

                                    // Call back
                                    GrabSomeData(socket);
                                  }
                         }
                        );
}

int main()
{
   asio::error_code ec;

   // Create a context
   asio::io_context context;

   // Give some fake task to asio so context doesn't finish
   asio::io_context::work idleWork(context);

   // Start the context
   std::thread thrContext = std::thread([&](){context.run();});

   // Get the adress
   asio::ip::address address = asio::ip::make_address("51.38.81.49",ec);
   unsigned port = 80;
   asio::ip::tcp::endpoint endpoint(address,port);

   // Create socket
   asio::ip::tcp::socket socket(context);

   // Connect socket
   socket.connect(endpoint,ec);

   if(!ec){
       std::cout << "Connected" << std::endl;
     }
   else{
       std::cout << "Failed: \n" << ec.message() << std::endl;
     }


   if(socket.is_open()){
       // Prime the socket
       GrabSomeData(socket);

       std::string sRequest =
           "GET /index.html HTTP/1.1\r\n"
           "Host: example.com\r\n"
           "Connection: close\r\n\r\n";


       // Note: Buffer take as input
       //          String.data - Array or characters
       //          String.size - Length of string

       socket.write_some(asio::buffer(sRequest.data(),sRequest.size()),ec);

       // Program does somethings else, while asio handles data transfer in background
       using namespace std::chrono_literals;
       std::this_thread::sleep_for(20000ms);

       context.stop();
       if(thrContext.joinable()) thrContext.join();

     }

   return 0;

}
