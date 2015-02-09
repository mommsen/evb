#ifndef _evb_readoutunit_ferolTCP_BufferPool_h_
#define _evb_readoutunit_ferolTCP_BufferPool_h_

#include <stdint.h>
#include <assert.h>

#include <evb/readoutunit/ferolTCP/Exception.h>
#include <evb/readoutunit/ferolTCP/Tools.h>


namespace evb {
  namespace readoutunit {
    namespace ferolTCP {

      // Forward declaration
      class Buffer;


      /**
       * Very simple memory pool for two buffers. If more than two buffers necessary, consider using boost pool interface for memory management
       */
      class BufferPool
      {
      public:
        
        BufferPool()//(size_t items, size_t block_size)
        {
          bufferCount_ = 0;
        }
        
        
        ~BufferPool()
        {}


        void addBuffer(Buffer* buffer)
        {
          assert(bufferCount_ < 2);
          buffers_[bufferCount_] = buffer;
          isFree_[bufferCount_] = true;
          bufferCount_++;
        }


        inline void setBufferAsFree(Buffer* buffer)
        {
          //It must be thread safe with respect to getObjectFromPool
          //As far as it is simple producer - consumer scheme (two threads), it is OK on x86

          // Find buffer (can be done directly with some index), but let's do it in this way, it is safer
          for (int i = 0; i < bufferCount_; i++)
          {
            if (buffers_[i] == buffer) {
              isFree_[i] = true;
              return;
            }
          }

          // Object was not found -> ERROR
          //FIXME: We should throw an exception
          assert(false);
        }


        /*
         * Returns a free buffer from pool
         * Thread safety: It is not thread safe, therefore this function must be executed within only one thread.
         * TODO: We can introduce a semaphore and when there is nothing free (semaphore==0), the process asking for it will go sleep...
         */
        inline Buffer* getBufferFromPool()
        {
          for (int i = 0; i < bufferCount_; i++)
          {
            if (isFree_[i]) {
              isFree_[i] = false;
              return buffers_[i];
            }
          }

          if (bufferCount_ == 0) {
            throw Exception(ENOMEM, "ferolTCP::BufferPool::getBufferFromPool(): The buffer pool is empty!");
          }

          // Nothing is free
          return NULL;
        }

      private:
        Buffer* buffers_[2];
        int bufferCount_;

        mutable volatile bool isFree_[2];
      };


      /*
       * Buffer stored in the pool.
       */
      class Buffer
      {
      public:
        Buffer(unsigned char* buffer, size_t size, BufferPool& pool) :
          pool_(pool),
          buffer_(buffer),
          buffer_size_(size),
          refCount_(0)
        {}

        inline void acquire()
        {
          //FIXME: We don't have boost atomic!!!
          __sync_add_and_fetch(&refCount_, 1);

          //LOG("Buffer acquire, count = %i", refCount_);
        }

        inline void release(unsigned char* payload)
        {
          // Test if the payload is in this buffer
          assert( payload < (buffer_ + buffer_size_) );

          int ref = __sync_sub_and_fetch(&refCount_, 1);

          //FIXME: We don't have boost atomic!!!
          //LOG("Buffer release, count = %i", ref);

          assert( ref >= 0);
          if (ref == 0) {
            // Issue a full memory barrier before we set this buffer as free
            __sync_synchronize();
            //LOG("Buffer is free");
            pool_.setBufferAsFree(this);
          }
        }

      public:
        BufferPool& pool_;
        unsigned char* buffer_;
        size_t buffer_size_;

      private:
        mutable volatile int refCount_;
      };

} } }  // namespace evb::readoutunit::ferolTCP


#endif // _evb_readoutunit_ferolTCP_BufferPool_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
