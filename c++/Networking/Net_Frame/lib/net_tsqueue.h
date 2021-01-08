#pragma once
#include "net_common.h"


namespace snt{
  namespace net{
    template<typename T>
    class tsqueue{
    public:
      ///////////////////////////////////////////////////
      /// \brief Default constructor and destructor
      tsqueue() = default;
      tsqueue(const tsqueue<T>&) = delete;
      virtual ~tsqueue(){clear();}

    public:
      ///////////////////////////////////////////////////
      /// \brief Return and maintains item at front of the queue
      const T& front(){
        std::scoped_lock lock(muxQueue);
        return deQueue.front();
      }
      ///////////////////////////////////////////////////
      /// \brief Return and maintains item at back of the queue
      const T& back(){
        std::scoped_lock lock(muxQueue);
        return deQueue.back();
      }
      ///////////////////////////////////////////////////
      /// \brief Return true if queue has no item
      bool empty(){
        std::scoped_lock(muxQueue);
        return deQueue.empty();
      }
      ///////////////////////////////////////////////////
      /// \brief Return the number of items in queue
      size_t size(){
        std::scoped_lock lock(muxQueue);
        return deQueue.size();
      }
      ///////////////////////////////////////////////////
      /// \brief Clear queue
      void clear(){
        std::scoped_lock lock(muxQueue);
        deQueue.clear();
      }
      ///////////////////////////////////////////////////
      /// \brief Remove and returns item from front of queue
      T pop_front(){
        std::scoped_lock lock(muxQueue);
        auto t = std::move(deQueue.front()); // Hard copy the obj
        deQueue.pop_front();
        return t;
      }
      ///////////////////////////////////////////////////
      /// \brief Remove and returns item from back of queue
      T pop_back(){
        std::scoped_lock lock(muxQueue);
        auto t = std::move(deQueue.back()); // Hard copy the obj
        deQueue.pop_back();
        return t;
      }
      ///////////////////////////////////////////////////
      /// \brief Add an item to front of queue
      void push_front(const T& item){
        std::scoped_lock lock(muxQueue);
        deQueue.push_front(std::move(item));

        std::unique_lock<std::mutex> ul(muxBlocking);
        cvBlocking.notify_one();
      }
      ///////////////////////////////////////////////////
      /// \brief Add an item to back of queue
      void push_back(const T& item){
        std::scoped_lock lock(muxQueue);
        deQueue.push_back(std::move(item));

        std::unique_lock<std::mutex> ul(muxBlocking);
        cvBlocking.notify_one();
      }
      ///////////////////////////////////////////////////
      /// \brief Wait until the queue get to be push back
      void wait(){
        while(empty()){
            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.wait(ul);
          }
      }


    protected:
      std::mutex muxQueue;
      std::deque<T> deQueue;

      std::mutex muxBlocking;
      std::condition_variable cvBlocking;

    };


  }
}
