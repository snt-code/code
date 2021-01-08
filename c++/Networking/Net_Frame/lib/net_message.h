#pragma once
#include "net_common.h"

namespace snt{
  namespace net{
    ///////////////////////////////////////////////////
    ///////////////////////////////////////////////////
    /*
     * Header of the Message
     *
     */
    template <typename T>
    struct message_header{
      T id{};
      uint32_t size = 0;
      uint64_t checksum = 0;
    };


    ///////////////////////////////////////////////////
    ///////////////////////////////////////////////////
    /*
     * Message
     *
     */
    template <typename T>
    struct message{
      message_header<T> header{};
      std::vector<uint8_t> payload;

      ///////////////////////////////////////////////////
      /// \brief size of the entire message
      size_t size() const{
        return payload.size() + sizeof(message_header<T>);
      }
      ///////////////////////////////////////////////////
      /// \brief operator << override for std::cout compatibility
      friend std::ostream& operator << (std::ostream& os, const message<T> msg){
          os << "ID:"<< int(msg.header.id) << " Size:"<<msg.header.size;
          return os;
      }
      ///////////////////////////////////////////////////
      /// \brief operator << override for POD-like data into message buffer
      template<typename DataType>
      friend message<T>& operator << (message<T>& msg, const DataType& data){
         /// Check that the type of the data is trivially copyable
         static_assert(std::is_standard_layout<DataType>::value,"Data Too Complex");
         /// Cache the current size
         size_t t_posSize = msg.payload.size();
         /// Resize the payload by the size of the data being pushed
         msg.payload.resize(msg.payload.size() + sizeof(DataType));
         /// Copy the data into the freshly allocated space
         std::memcpy(msg.payload.data()+t_posSize, &data, sizeof(DataType));
         /// Update size
         msg.header.size = msg.payload.size();

         return msg;
      }
      ///////////////////////////////////////////////////
      /// \brief operator >> override for POD-like data extraction
      template<typename DataType>
      friend message<T>& operator >> (message<T>& msg, DataType& data){
         /// Check that the type of the data is trivially copyable
         static_assert(std::is_standard_layout<DataType>::value,"Data Too Complex");
         /// Cache the location towards the end of the vector
         size_t t_posSize = msg.payload.size() - sizeof(DataType);
         /// Copy the data from the payload to the datatype
         std::memcpy(&data, msg.payload.data()+t_posSize, sizeof(DataType));
         /// Shrink the vector to remove read bytes, and reset position
         msg.payload.resize(t_posSize);
         /// Update header
         msg.header.size = msg.payload.size();

         return msg;
      }
    };


    ///////////////////////////////////////////////////

    /// Forward declaration
    template<typename T>
    class connection;

    template<typename T>
    struct owned_message{
      std::shared_ptr<connection<T>> remote = nullptr;
      message<T> msg;

      /// Friendly string maker
      friend std::ostream& operator << (std::ostream& os, const owned_message<T>& msg){
        os << msg.msg;
        return os;
      }

    };

  }
}
