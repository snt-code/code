#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace snt{
  namespace net{

    /// Forward declaration
    template<typename T>
    class server_interface;

    template<typename T>
    class connection : public std::enable_shared_from_this<connection<T>>{
    public:
      enum class owner{
        server,
        client
      };

    public:
      ///////////////////////////////////////////////////
      /// \brief Constructor
      connection(owner parent,
                 asio::io_context& asioContext,
                 asio::ip::tcp::socket socket,
                 tsqueue<owned_message<T>>& qMessagesIn):
        m_asioContext(asioContext),
        m_socket(std::move(socket)),
        m_qMessagesIn(qMessagesIn)
      {
        m_nOwnerType = parent;

        // Construct validation check data
        if(m_nOwnerType == owner::server){
            // Connection is Server -> Client, construct random data for the client
            // to transform and send back for validation
            m_nHandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

            // Pre-calculate the result for checking when the client responds
            m_nHandshakeCheck = scramble(m_nHandshakeOut);
          }else{
            m_nHandshakeIn = 0;
            m_nHandshakeOut = 0;
          }
      }

      ///////////////////////////////////////////////////
      /// \brief Destructor
      virtual ~connection(){}

      ///////////////////////////////////////////////////
      /// \brief Return ID of the connection
      uint32_t GetID() const{
        return m_id;
      }

    public:
      ///////////////////////////////////////////////////
      /// \brief Connect to a client
      void ConnectToClient(snt::net::server_interface<T>* server, uint32_t uid = 0){
        if(m_nOwnerType == owner::server){
            m_id = uid;
            WriteValidation();
            ReadValidation(server);
          }
      }

      ///////////////////////////////////////////////////
      /// \brief Connect to a server
      void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoint){
        /// ASYNC Establish connection
        asio::async_connect(m_socket,endpoint,[this](std::error_code ec,asio::ip::tcp::endpoint endpoint){
          if(!ec){
              ReadValidation();
            }else{
              std::cout << "[" << m_id << "] [ERROR] ConnectToServer " << endpoint.address().to_string() <<  std::endl;
            }
        });
      }

    public:
      ///////////////////////////////////////////////////
      /// \brief Close the connection
      void Disconnect(){
        if(IsConnected()){
            /// Insert task in the ASIO context
            auto taskHandler = [this](){
                m_socket.close();
              };
            asio::post(m_asioContext,taskHandler);
          }
        return;
      }
      ///////////////////////////////////////////////////
      /// \brief True if connection is established
      bool IsConnected() const{
        return m_socket.is_open();
      }

    public:
      ///////////////////////////////////////////////////
      /// \brief Send the message to the remote
      void Send(const message<T>& msg){
        /// Insert task in the ASIO context
        asio::post(m_asioContext,[this,msg](){
            bool bWritingMessage = !m_qMessagesOut.empty();
            m_qMessagesOut.push_back(msg);
            if(!bWritingMessage){
                WriteHeader();
              }
          });
      }

    public:
      ///////////////////////////////////////////////////
      /// \brief ASYNC - Read header of message
      void ReadHeader(){
        /// ASYNC - Read the socket, Read() will wait for message.
        asio::async_read(m_socket,asio::buffer(&m_msgTemporaryIn.header,sizeof(message_header<T>)),[this](std::error_code ec, std::size_t len){
          if(!ec){
              if(m_msgTemporaryIn.header.size > 0 ){
                  /// Resize the payload section of the message
                  /// to read the body from the socket.
                  m_msgTemporaryIn.payload.resize(m_msgTemporaryIn.header.size);
                  /// Read the payload
                  ReadPayload();
                }else{
                  /// Add message to queue
                  AddIncomingMessageToQueue();
                }
            }else{
              std::cout << "[" << m_id << "] [ERROR] Reader Header Data Size: "
                        << sizeof(message_header<T>)
                        << " Data Sent: "
                        << len
                        << std::endl;
              /// Close socket
              m_socket.close();
            }
        });
      }
      ///////////////////////////////////////////////////
      /// \brief ASYNC - Read payload of message
      void ReadPayload(){
        /// ASYNC - Read the socket
        asio::async_read(m_socket,asio::buffer(m_msgTemporaryIn.payload.data(),m_msgTemporaryIn.header.size),[this](std::error_code ec, std::size_t len){
          if(!ec){
              /// Add message to queue
              AddIncomingMessageToQueue();
            }else{
              std::cout << "[" << m_id << "] [ERROR] Reader Payload Data Size: "
                        << sizeof(message_header<T>)
                        << " Data Sent: "
                        << len
                        << std::endl;
              /// Close socket
              m_socket.close();
            }
        });
      }
      ///////////////////////////////////////////////////
      /// \brief ASYNC - Prime context to write header
      void WriteHeader(){
        /// ASYNC - Write in the socket
        auto taskHandler = [this](std::error_code ec, std::size_t len){
          if(!ec){
              if(m_qMessagesOut.front().payload.size() > 0){
                  /// If payload > 0, write payload
                  WritePayload();
                }else{
                  /// Pop the sent message
                  m_qMessagesOut.pop_front();
                  /// If queue not empty, prime the handler to send
                  /// all message in the queue.
                  if(!m_qMessagesOut.empty())
                    WriteHeader();
                }
            }else{
              std::cout << "[" << m_id << "] [ERROR] Write Header Data Size: "
                        << sizeof(message_header<T>)
                        << " Data Sent: "
                        << len
                        << std::endl;
              /// Close socket
              m_socket.close();
            }
        };
        asio::async_write(m_socket,asio::buffer(&m_qMessagesOut.front().header,sizeof(message_header<T>)),taskHandler);
      }
      ///////////////////////////////////////////////////
      /// \brief ASYNC - Prime context to write payload
      void WritePayload(){
        /// ASYNC - Write in the socket
        auto taskHandler = [this](std::error_code ec, std::size_t len){
          if(!ec){
              /// Pop the sent message
              m_qMessagesOut.pop_front();
              /// If queue not empty, prime the handler to send
              /// all message in the queue.
              if(!m_qMessagesOut.empty())
                WriteHeader();
            }else{
              std::cout << "[" << m_id
                        << "] [ERROR] Write Payload Data Size: "
                        << m_qMessagesOut.front().header.size
                        << " Data Sent: "
                        << len
                        << std::endl;
              /// Close socket
              m_socket.close();
            }
        };
        asio::async_write(m_socket,
                          asio::buffer(m_qMessagesOut.front().payload.data(),m_qMessagesOut.front().payload.size()),
                          taskHandler);
      }
      ///////////////////////////////////////////////////
      /// \brief ASYNC - Add message to queue
      void AddIncomingMessageToQueue(){
        snt::net::owned_message<T> msg;
        if(m_nOwnerType == owner::server){
            msg = {this->shared_from_this(),m_msgTemporaryIn};
          }else{
            msg = {nullptr,m_msgTemporaryIn};
          }
        m_qMessagesIn.push_back(msg);
        /// Prime the Read Header to wait for a msg
        ReadHeader();
      }
      ///////////////////////////////////////////////////
      /// \brief Scramble data
      uint64_t scramble(uint64_t nInput){
        uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
        out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
        return out ^ 0xC0DEFACE12345678;
      }
      ///////////////////////////////////////////////////
      /// \brief ASYNC - Used by both client and server to write validation packet
      void WriteValidation(){
        auto taskHandler = [this](std::error_code ec, std::size_t len){
          if( !ec)
            {
              // Validation data sent, clients should sit and wait
              // for a response or closure
              if(m_nOwnerType == owner::client)
                ReadHeader();
            }else{
              std::cout << "[" << m_id << "] [ERROR] WriteValidation length: " << len << std::endl;
              m_socket.close();
            }
        };
        asio::async_write(m_socket,
                          asio::buffer(&m_nHandshakeOut,sizeof(uint64_t)),
                          taskHandler);
      }
      ///////////////////////////////////////////////////
      /// \brief ASYNC - Used by both client and server to read validation packet
      void ReadValidation(snt::net::server_interface<T>* server = nullptr){
        auto taskHandler = [this,server](std::error_code ec, std::size_t len){
            if(!ec){
                if( m_nOwnerType == owner::server){
                    if( m_nHandshakeIn == m_nHandshakeCheck){
                        /// Client has provided a valid solution, so allow it to connect properly
                        std::cout << "[" << m_id << "] Client Validated" << std::endl;
                        server->OnClientValidated(this->shared_from_this());

                        /// Sit waiting to recieve data now
                        ReadHeader();
                      }else{
                        std::cout << "[" << m_id << "] Client Disconnected (Fail Validation)" << std::endl;
                        m_socket.close();
                      }
                  }else{
                    /// Connection is a client, so solve puzzle
                    m_nHandshakeOut = scramble(m_nHandshakeIn);
                    /// Write result
                    WriteValidation();
                  }
              }else{
                std::cout  << "[" << m_id << "] Client Disconnected (ReadValidation) length: "<< len << std::endl;
                m_socket.close();
              }
          };
        asio::async_read(m_socket,
                         asio::buffer(&m_nHandshakeIn,sizeof(uint64_t)),
                         taskHandler);


      }

    protected:
      /// Connection ID
      uint32_t m_id = 0;
      /// ASIO instance context
      asio::io_context& m_asioContext;
      /// Socket Connection
      asio::ip::tcp::socket m_socket;

      /// Queue holding message to be sended
      tsqueue<message<T>> m_qMessagesOut;
      /// Addres of recieved message queue
      tsqueue<owned_message<T>>& m_qMessagesIn;
      /// Temporary cache of incoming message
      message<T> m_msgTemporaryIn;
      /// Owner of the connection
      owner m_nOwnerType = owner::server;

      // Handshake Validation
      uint64_t m_nHandshakeOut   = 0;
      uint64_t m_nHandshakeIn    = 0;
      uint64_t m_nHandshakeCheck = 0;

    };

  }
}
