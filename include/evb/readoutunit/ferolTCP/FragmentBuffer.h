#ifndef _evb_readoutunit_ferolTCP_FragmentBuffer_h_
#define _evb_readoutunit_ferolTCP_FragmentBuffer_h_

#include <stdint.h>
//#include <queue>

//#include <tr1/unordered_set>
//#include <boost/unordered_set.hpp>

#include <assert.h>

#include "evb/readoutunit/ferolTCP/Exception.h"
#include "evb/readoutunit/ferolTCP/Tools.h"
#include "evb/readoutunit/ferolTCP/BufferPool.h"


#define DEBUG
//#define DEBUG_DATA
//#define DEBUG_BLOCK
//#define DEBUG_WAIT
//#define DEBUG_RELEASE

#ifndef DEBUG
#  undef LOG_RETURN
#  undef LOG_CALL
#  undef LOG
#  define LOG_RETURN(str, rest...) ;
#  define LOG_CALL(str, rest...) ;
#  define LOG(str, rest...) ;
#endif

#ifdef DEBUG_DATA
#  define DDATA_LOG_CALL          LOG_CALL
#  define DDATA_LOG_RETURN        LOG_RETURN
#  define DDATA_LOG               LOG
#else
#  define DDATA_LOG_RETURN(str, rest...) ;
#  define DDATA_LOG_CALL(str, rest...) ;
#  define DDATA_LOG(str, rest...) ;
#endif

#ifdef DEBUG_BLOCK
#  define DBLOCK_LOG_CALL         LOG_CALL
#  define DBLOCK_LOG_RETURN       LOG_RETURN
#  define DBLOCK_LOG              LOG
#else
#  define DBLOCK_LOG_RETURN(str, rest...) ;
#  define DBLOCK_LOG_CALL(str, rest...) ;
#  define DBLOCK_LOG(str, rest...) ;
#endif

#ifdef DEBUG_WAIT
#  define DWAIT_LOG_CALL          LOG_CALL
#  define DWAIT_LOG_RETURN        LOG_RETURN
#  define DWAIT_LOG               LOG
#else
#  define DWAIT_LOG_RETURN(str, rest...) ;
#  define DWAIT_LOG_CALL(str, rest...) ;
#  define DWAIT_LOG(str, rest...) ;
#endif

#ifdef DEBUG_RELEASE
#  define DRELEASE_LOG_CALL       LOG_CALL
#  define DRELEASE_LOG_RETURN     LOG_RETURN
#  define DRELEASE_LOG            LOG
#else
#  define DRELEASE_LOG_RETURN(str, rest...) ;
#  define DRELEASE_LOG_CALL(str, rest...) ;
#  define DRELEASE_LOG(str, rest...) ;
#endif


namespace evb {
  namespace readoutunit {
    namespace ferolTCP {

      /**
      * Read buffer
      */
      class FragmentBuffer
      {
        public:

        FragmentBuffer(BufferPool& pool) :
          pool_(pool)
        {
          if ( (currentBuffer_ = pool.getBufferFromPool()) == NULL )
          {
            throw Exception(ENOMEM, "ferolTCP::FragmentBuffer::FragmentBuffer(): No available memory in the buffer pool!");
          }

          memset(&stats, 0, sizeof(stats));
          resetBuffer();
        }

        ~FragmentBuffer() {}
        
        /**
         * Reset receive buffer pointers to default values
         */
        void resetBuffer()
        {
          // We are also owner of the buffer and it cannot be released if we have some data in
          currentBuffer_->acquire();

          buffer_ = currentBuffer_->buffer_;
          buffer_size_ = currentBuffer_->buffer_size_;

          tail_ = buffer_;
          read_pointer_ = buffer_;
          end_of_fragment_ = buffer_;
        }
        

        /**
         * Return the size of data stored in the buffer
         */
        inline ptrdiff_t getSizeOfData() const
        {
          return (tail_ - read_pointer_);
        }
        
        
        /**
         * If the requested amount is already in the buffer, return it quickly and fail otherwise (return NULL)
         * Returns the amount of available data
         */
        inline unsigned char* getData(size_t size)
        {
          unsigned char* data = read_pointer_;

          // If we have data in the buffer, return them
          if ( getSizeOfData() >= (ptrdiff_t)size )
          {
            read_pointer_ += size;
            return data;
          }

          // We don't have data in the buffer
          return NULL;
        }
        
        
        /**
         * Return the pointer to the free part of the buffer and return the maximum size which can be filled with new data
         */
        inline unsigned char* getFreeSpace(size_t requestedSize, size_t& availableFreeSpace)
        {
          // Wait until we have free space
          availableFreeSpace = waitForFreeSpace(requestedSize);
          assert(availableFreeSpace > 0);

          return (tail_);
        }


        /**
         * Update the buffer occupancy with the size of the new data stored into the buffer at the location pointer
         */
        inline void updateFreeSpace(unsigned char *pointer, size_t size)
        {
          assert( pointer == tail_ );

          tail_ += size;

          assert( tail_ <= (buffer_ + buffer_size_) );
        }


        /**
         * Set end of block and update data pointer in case it was moved. Also returns an associated Buffer instance, so it can be released later
         * Any following data can be moved if necessary
         */
        inline unsigned char* setEndOfFragment(size_t size, Buffer*& buffer)
        {
          unsigned char* pointer = end_of_fragment_;

          assert( read_pointer_ - end_of_fragment_ == (ptrdiff_t) size );

          // Update stats
          stats.fragments++;

          // Set a new end of fragment
          end_of_fragment_ = read_pointer_;

//          std::ostringstream out;
//          out << "After EoF: ";
//          printStats(&out);
//          out << std::endl;
//          std::cerr << out.str();

          // Return current buffer instance
          currentBuffer_->acquire();
          buffer = currentBuffer_;

          return pointer;
        }

        
        inline ptrdiff_t getPointerOffset(unsigned char *pointer) const
        {
          return (pointer - buffer_);
        }


        void printStats(std::ostringstream* out) const
        {
          *out << "Fragments:        " << stats.fragments << ", "
               << "Memory copies:    " << stats.memcpy << ", "
               << "Zero copies:      " << stats.memcpy_zero << ", "
               << "Bytes copied:     " << stats.memcpy_bytes << ", "
               << "Bytes total:      " << stats.total_bytes << ", "
               << "Waiting for pool: " << stats.pool_wait << ", "
               << "tail =            *" << getPointerOffset(tail_) << ", "
               << "read =            *" << getPointerOffset(read_pointer_) << ", "
               << "end_of_fragment = *" << getPointerOffset(end_of_fragment_);
        }


      private:

        /**
         * Return the free space from right (how much data can be stored)
         */
        inline ptrdiff_t getSizeOfFreeSpace() const
        {
          return (buffer_size_ + buffer_ - tail_);
        }


        /**
         * Wait until there is size free bytes available in the buffer and return the total free size
         */
        inline ptrdiff_t waitForFreeSpace(size_t size)
        {
          ptrdiff_t free = getSizeOfFreeSpace();

          if ( free >= (ptrdiff_t) size )
          {
            return free;
          }

          //LOG_ERROR("FATAL: The fragment is not fitting into the buffer.  tail = *%p, read = *%p, requested size = %lu, buffer size = %lu", tail_, read_pointer_, size, buffer_size_);
          //throw Exception(EIO, "TCPFerolBuf::waitForFreeSpaceFromRight(): FATAL: The fragment is not fitting into the buffer");

          // We don't have enough memory in current buffer for storing a new fragment
          // Let's find a new buffer

          Buffer* newBuffer;

          //LOG("buffer space required is %lu, looking for a new buffer...", size);
          while ( (newBuffer = pool_.getBufferFromPool()) == NULL )
          {
            stats.pool_wait++;
            //TODO: Make getBufferFromPool a blocking call...?
            //LOG("waiting for free buffer");
            //::usleep(3000000);
            ::usleep(50);
          }
          //LOG("got it!");

          // OK, we have a new buffer
          // But what to do with it?

          // How much data we have to move
          size_t dataToMove = tail_ - end_of_fragment_;

          if ( (dataToMove + size) > newBuffer->buffer_size_ )
          {
            LOG_ERROR("FATAL: The incomplete fragment (length = %lu) is too big and will not fit into the buffer", dataToMove);
            throw Exception(EIO, "ferolTCP::FragmentBuffer::waitForFreeSpace(): FATAL: The incomplete fragment is too big and will not fit into the buffer");
          }

          if (dataToMove)
          {
            // OK, we can move data to the beggining of the buffer
            memcpy(newBuffer->buffer_, end_of_fragment_, dataToMove);

            // Update stats
            stats.memcpy++;
            stats.memcpy_bytes += dataToMove;
          }
          else
          {
            stats.memcpy_zero++;
          }

          // Update the new buffer

          // Make a backup of read pointer
          ptrdiff_t readPointer = read_pointer_ - end_of_fragment_;

          // Now we can release the current buffer because we don't have nay data in
          currentBuffer_->release(end_of_fragment_);

          currentBuffer_ = newBuffer;
          resetBuffer();

          // Set pointers to the correct values
          tail_+= dataToMove;
          read_pointer_ += readPointer;

          free = getSizeOfFreeSpace();
          assert( free >= (ptrdiff_t) size );

          //LOG("The new buffer size %lu bytes, free is %lu bytes", buffer_size_, free);

//          std::ostringstream out;
//          out.str("");
//          out << "Buffer stats: ";
//          printStats(&out);
//          out << std::endl;
//          std::cerr << out.str();

          return free;
        }


      private:

        BufferPool& pool_;
        Buffer* currentBuffer_;             // Current buffer object from pool


        size_t max_allowed_fragment_;       // Size of the maximum allowed fragment

        unsigned char* buffer_;             // Pointer to the allocated memory for the buffer
        size_t buffer_size_;                // Size of the allocated memory

        unsigned char* read_pointer_;       // Pointer to the next available data in the buffer (will be returned in the next getData call)
        unsigned char* tail_;               // Pointer to the end of data in the buffer

        unsigned char* end_of_fragment_;    // Pointer to the end of fragment


      private:

        // Collect some statistics
        struct {
          uint64_t memcpy;                        // How many memory copies was done
          uint64_t memcpy_zero;                   // How many times copy was not necessary
          uint64_t memcpy_bytes;                  // Total amount of bytes copies
          uint64_t fragments;                     // How many fragments seen in the buffer
          uint64_t pool_wait;                     // How long we spent in waiting for a new buffer
          uint64_t total_bytes;                   // Total amount of bytes passed through the buffer
        } stats;
      };

      typedef boost::shared_ptr<FragmentBuffer> FragmentBufferSharedPtr;

    
} } }  // namespace evb::readoutunit::ferolTCP


#endif // _evb_readoutunit_ferolTCP_FragmentBuffer_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -



