#pragma once
#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"


namespace snt{
  namespace net {
    template<typename T>
    class server_interface{
    public:
      server_interface(uint16_t port): m_asioAcceptor(m_asioContext,
                                                      asio::ip::tcp::endpoint(asio::ip::tcp::v6(),port))
      {}
      virtual ~server_interface(){
        /// Stop before server_interface object is destroyed
        Stop();
      }
    public:
      ///////////////////////////////////////////////////
      /// \brief Start the server
      bool Start(){
        try{
          WaitForClientConnection();
          /// Run the ASIO context in its own thread
          auto thrTask = [this](){
              m_asioContext.run();
            };
          m_thrContext = std::thread(thrTask);
        }catch(std::exception& e){
          std::cerr << "[SERVER] Exception: "<< e.what() << "\n";
          return false;
        }
        std::cerr << "[SERVER] Started.\n";
        return true;
      }
      ///////////////////////////////////////////////////
      /// \brief Stop the server
      bool Stop(){
        // Request the context to close
        m_asioContext.stop();
        // Close the thread
        if(m_thrContext.joinable())
          m_thrContext.join();
        std::cout << "[SERVER] Stopped.\n";
        return true;
      }
      ///////////////////////////////////////////////////
      /// \brief ASYNC - Wait for connection
      void WaitForClientConnection(){
        auto taskHandler = [this](std::error_code ec, asio::ip::tcp::socket socket){
          if(!ec){
              std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;
              /// Create a new connection object with the socket
              std::shared_ptr<connection<T>> newConnection = std::make_shared<connection<T>>(connection<T>::owner::server,
                                                                                             m_asioContext,
                                                                                             std::move(socket),
                                                                                             m_qMessagesIn);
              if(OnClientConnect(newConnection)){
                  /// Push the new connection to the connection container
                  m_deqConnections.push_back(std::move(newConnection));
                  /// Connect to client with ID
                  m_deqConnections.back()->ConnectToClient(this,nIDCounter++);
                  std::cout << "[" << m_deqConnections.back()->GetID() << "] Connection Approved\n";
                }else{
                  std::cout << "[SERVER] New Connection Error: " << ec.message() <<"\n";
                }
            }else{
              std::cout << "[SERVER] New connection Error: " << ec.message() << std::endl;
            }
          /// Prime the WaitForClientConnection
          WaitForClientConnection();
        };
        m_asioAcceptor.async_accept(taskHandler);
      }
      ///////////////////////////////////////////////////
      /// \brief Send message to a specific client
      void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg){
        if(client && client->IsConnected()){
            client->Send(msg);
          }else{
            /// Client cannot be contacted, assume disconnected
            OnClientDisconnect(client);
            client.reset();
            /// Remove it from the container
            m_deqConnections.erase(std::remove(m_deqConnections.begin(), m_deqConnections.end(), client),
                                   m_deqConnections.end());
          }
      }
      ///////////////////////////////////////////////////
      /// \brief Send message to all client
      void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr){
        bool bInvalidClientExists = false;
        /// Iterate all client in the container
        for(auto& client : m_deqConnections){
            /// Check if client is connected
            if(client && client->IsConnected()){
                if(client != pIgnoreClient){
                    client->Send(msg);
                  }else{
                    /// If client cannot be contacted, assume disconnected
                    OnClientDisconnect(client);
                    /// Reset pointer
                    client.reset();
                    bInvalidClientExists = true;
                  }
              }

            if(bInvalidClientExists == true)
              /// Remove dead clients
              m_deqConnections.erase(std::remove(m_deqConnections.begin(),m_deqConnections.end(),nullptr),
                                     m_deqConnections.end());

          }
      }
      ///////////////////////////////////////////////////
      /// \brief Update server message handling
      void Update(size_t nMaxMessages = -1, bool bWait = false){
        if(bWait)
          m_qMessagesIn.wait();

        size_t nMessagesCount = 0;
        while(nMessagesCount < nMaxMessages && !m_qMessagesIn.empty()){
            /// Grab the front of message
            auto msg = m_qMessagesIn.pop_front();
            /// Handle the message
            OnMessage(msg.remote,msg.msg);
            nMessagesCount++;
          }
      }

    protected:
      ///////////////////////////////////////////////////
      /// \brief Called when a client connect
      ///  Need to be overrided
      virtual bool OnClientConnect(std::shared_ptr<connection<T>>){return false;}
      ///////////////////////////////////////////////////
      /// \brief Called when a client appears to have disconnected
      ///  Need to be overrided
      virtual void OnClientDisconnect(std::shared_ptr<connection<T>>){}
      ///////////////////////////////////////////////////
      /// \brief Called when a message is recieved
      ///  Need to be overrided
      virtual void OnMessage(std::shared_ptr<connection<T>>, message<T>&){}
    public:
      ///////////////////////////////////////////////////
      /// \brief Called when a client is validated
      ///  Need to be overrided
      virtual void OnClientValidated(std::shared_ptr<connection<T>>){}

    protected:
      /// Thread safe queue for handling incoming messages
      tsqueue<owned_message<T>> m_qMessagesIn;
      /// Container of active validated connections
      std::deque<std::shared_ptr<connection<T>>> m_deqConnections;
      /// Asio context
      asio::io_context m_asioContext;
      /// Thread containing the ASIO context;
      std::thread m_thrContext;
      /// Acceptor object to accept incoming connection
      asio::ip::tcp::acceptor m_asioAcceptor;
      /// ID Counter
      uint32_t nIDCounter= 10000;


    };

  }
}
