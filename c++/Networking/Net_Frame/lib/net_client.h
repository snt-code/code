#pragma once
#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"


namespace snt{
  namespace net{
    template<typename T>
    class client_interface{
    public:
      client_interface(){}
      virtual ~client_interface(){
        /// Try to disconnect when object is destroyed
        Disconnect();
      }
    public:
      ///////////////////////////////////////////////////
      /// \brief Connect to a host
      bool Connect(const std::string& host, const uint16_t port){
        try{
          /// Initialize the DNS resolver
          asio::ip::tcp::resolver resolver(m_context);
          /// Resolve hostname/ip-address into physical address
          asio::ip::tcp::resolver::results_type endpoint = resolver.resolve(host, std::to_string(port));
          /// Create connection
          m_connection = std::make_unique<connection<T>>(connection<T>::owner::client,
                                                         m_context,
                                                         asio::ip::tcp::socket(m_context),
                                                         m_qMessagesIn);
          /// Connect to the server
          m_connection->ConnectToServer(endpoint);
          /// Start Context Thread
          auto thrTask = [this](){
              m_context.run();
            };
          m_thrContext = std::thread(thrTask);
        }catch(std::exception& e){
          std::cerr << "[ERROR] Client Exception: " << e.what() << "\n";
          return false;
        }
        return true;
      }
      ///////////////////////////////////////////////////
      /// \brief Disconnect from a host
      void Disconnect(){
        /// If connection, disconnect
        if(IsConnected())
          m_connection->Disconnect();
        /// Stop the context
        m_context.stop();
        /// Stop the dedicated thread
        if(m_thrContext.joinable())
          m_thrContext.join();
        /// Destroy the connection object
        m_connection.release();
      }
      ///////////////////////////////////////////////////
      /// \brief Verify the state of the connection
      bool IsConnected(){
        if(m_connection)
          return m_connection->IsConnected();
        else
          return false;
      }
    public:
      ///////////////////////////////////////////////////
      /// \brief Send a message to the host
      void Send(const message<T>& msg){
        if(IsConnected())
          m_connection->Send(msg);
      }
      ///////////////////////////////////////////////////
      /// \brief Retrieve queue of message from server
      tsqueue<owned_message<T>>& Incoming(){
        return m_qMessagesIn;
      }
    protected:
      /// Asio context to handles the data transfer
      asio::io_context m_context;
      /// Thread to execute its work commands
      std::thread m_thrContext;
      /// Connection object which handles data transfer
      std::unique_ptr<connection<T>> m_connection;

    private:
      /// Thread safe queue of incoming message
      tsqueue<owned_message<T>> m_qMessagesIn;

    };

  }
}
