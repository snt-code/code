#include <iostream>

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

using namespace std;

int main()
{
   asio::error_code ec;

   // Create a context
   asio::io_context context;

   // Get the adress
   asio::ip::address address = asio::ip::make_address("93.184.216.34",ec);
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
       std::string sRequest =
           "GET /index.html HTTP/1.1\r\n"
           "Host: example.com\r\n"
           "Connection: close\r\n\r\n";


       // Note: Buffer take as input
       //          String.data - Array or characters
       //          String.size - Length of string

       socket.write_some(asio::buffer(sRequest.data(),sRequest.size()),ec);

       // Using asyncio to wait for information
       socket.wait(socket.wait_read);

       // Initialize for any available bytes
       size_t bytes = socket.available();
       std::cout << "Bytes Available: " << bytes << std::endl;

       if( bytes > 0){
            std::vector<char> vBuffer(bytes);
            socket.read_some(asio::buffer(vBuffer.data(),vBuffer.size()),ec);

            for( auto c : vBuffer)
              std::cout << c;
         }

     }

   return 0;

}
