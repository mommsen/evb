#ifndef _evb_OneToOneQueue_h_
#define _evb_OneToOneQueue_h_

#include <stdexcept>
#include <stdint.h>
#include <vector>

#include "interface/evb/i2oEVBMsgs.h"
#include "evb/Exception.h"
#include "toolbox/mem/Reference.h"
#include "xgi/Output.h"


namespace evb {

  /**
   * \ingroup xdaqApps
   * \brief A lock-free queue which is threadsafe if 
   * there is only one producer and one consumer.
   */

  template <class T>
  class OneToOneQueue
  {
  public:

    OneToOneQueue(const std::string& name);
    OneToOneQueue(const std::string& name, const uint32_t size);

    /**
     * Enqueue the element.
     * Returns false if the element cannot be enqueued
     */
    bool enq(const T&);

    /**
     * Dequeue an element.
     * Return false if no element can be dequeued.
     */
    bool deq(T&);

    /**
     * Return the number of elements in the queue.
     */
    uint32_t elements() const;

    /**
     * Return the queue size
     */
    uint32_t size() const;

    /**
     * Returns true if the queue is empty.
     */
    bool empty() const;

    /**
     * Resizes the queue.
     * Throws an exception if queue is not empty.
     */
    void resize(const uint32_t size);

    /**
     * Print summary icon as HTML.
     */
    void printHtml(xgi::Output*, const std::string& urn);

    /**
     * Print horizontally all elements as HTML.
     */
    void printHorizontalHtml(xgi::Output*);

    /**
     * Print horizontally upto nbElementsToPrint as HTML.
     */
    void printHorizontalHtml(xgi::Output*, const uint32_t nbElementsToPrint);
 
    /**
     * Print vertically all elements as HTML.
     */
    void printVerticalHtml(xgi::Output*);

    /**
     * Print vertically upto nbElementsToPrint as HTML.
     */
    void printVerticalHtml(xgi::Output*, const uint32_t nbElementsToPrint);
 

  private:
    
    /**
     * Format passed element information into the ostringstream.
     */    
    void formatter(T, std::ostringstream*);
    
    const std::string name_;
    volatile uint32_t readPointer_;
    volatile uint32_t writePointer_;
    std::vector<T> container_;
    volatile uint32_t size_;
  };

  
  //------------------------------------------------------------------
  // Implementation follows
  //------------------------------------------------------------------

  template <class T>
  OneToOneQueue<T>::OneToOneQueue(const std::string& name) :
  name_(name),
  readPointer_(0),
  writePointer_(0)
  {
    resize(1);
  }
  
  
  template <class T>
  OneToOneQueue<T>::OneToOneQueue(const std::string& name, const uint32_t size) :
  name_(name),
  readPointer_(0),
  writePointer_(0)
  {
    resize(size);
  }
  
  
  template <class T>
  inline uint32_t OneToOneQueue<T>::elements() const
  {
    volatile const uint32_t cachedWritePointer = writePointer_; 
    volatile const uint32_t cachedReadPointer = readPointer_; 
    const uint32_t elements = cachedWritePointer < cachedReadPointer ?
      cachedWritePointer + size_ - cachedReadPointer :
      cachedWritePointer - cachedReadPointer;
    return elements;
  }


  template <class T>
  inline uint32_t OneToOneQueue<T>::size() const
  {
    return (size_ - 1);  
  }


  template <class T>
  bool OneToOneQueue<T>::empty() const
  { return (readPointer_ == writePointer_); }
  
  
  template <class T>
  void OneToOneQueue<T>::resize(const uint32_t size)
  {
    if ( !empty() )
    {
      XCEPT_RAISE(evb::exception::FIFO,
        "Cannot resize the non-empty queue " + name_);
    }
    container_.clear();
    readPointer_ = writePointer_ = 0;
    size_ = size+1;
    container_.resize(size_);
  }
  
  
  template <class T>
  bool OneToOneQueue<T>::enq(const T& element)
  {
    volatile const uint32_t nextElement = (writePointer_ + 1) % size_;
    if ( nextElement == readPointer_ ) return false;
    container_[writePointer_] = element;
    writePointer_ = nextElement;
    return true;
  }
  
  
  template <class T>
  bool OneToOneQueue<T>::deq(T& element)
  {
    if ( empty() ) return false;

    volatile const uint32_t nextElement = (readPointer_ + 1) % size_;
    element = container_[readPointer_];
    readPointer_ = nextElement;
    return true;
  }


  template <class T>
  void OneToOneQueue<T>::printHtml(xgi::Output *out, const std::string& urn)
  {
    // cache values which might change during the printout
    const uint32_t cachedSize = size(); 
    const uint32_t cachedElements = elements();
    const double fillFraction = cachedSize > 0 ? 100. * cachedElements / cachedSize : 0;

    *out << "<div class=\"queue\">" << std::endl;
    *out << "<table onclick=\"window.open('/" << urn << "/" << name_ << "','_blank')\" "
      << "onmouseover=\"this.style.cursor = 'pointer'; "
      << "document.getElementById('" << name_ << "').style.visibility = 'visible';\" "
      << "onmouseout=\"document.getElementById('" << name_ << "').style.visibility = 'hidden';\">" << std::endl;
    *out << "<colgroup>" << std::endl;
    *out << "<col width=\"" << static_cast<unsigned int>( fillFraction + 0.5 ) << "%\"/>" << std::endl;
    *out << "<col width=\"" << static_cast<unsigned int>( (100-fillFraction) + 0.5 ) << "%\"/>" << std::endl;
    *out << "</colgroup>" << std::endl;
    
    *out << "<tr>" << std::endl;
    *out << "<th colspan=\"2\">" << name_ << "</th>" << std::endl;
    *out << "</tr>" << std::endl;

    *out << "<tr>" << std::endl;
    *out << "<td style=\"height:1em;background:#8178fe\"></td>" << std::endl;
 
    *out << "<td style=\"background:#feffd6\"></td>" << std::endl;
    *out << "</tr>" << std::endl;

    *out << "<tr>" << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">" << cachedElements << " / " << cachedSize << "</td>" << std::endl;
    *out << "</tr>" << std::endl;

    *out << "</table>" << std::endl;

    *out << "<div id=\"" << name_ << "\" class=\"queuefloat\">" << std::endl;
    printHorizontalHtml(out, 10);
    *out << "</div>" << std::endl;

    *out << "</div>" << std::endl;
  }


  template <class T>
  void OneToOneQueue<T>::printHorizontalHtml
  (
    xgi::Output *out
  )
  {
    printHorizontalHtml( out, size() );
  }
  
  
  template <class T>
  void OneToOneQueue<T>::printHorizontalHtml
  (
    xgi::Output  *out,
    const uint32_t nbElementsToPrint
  )
  {
    // cache values which might change during the printout
    // Note: we do not want to lock the queue for the debug 
    // printout. Thus, be prepared that some elements are no
    // longer there when we want to print them.
    const uint32_t cachedReadPointer = readPointer_;
    const uint32_t cachedWritePointer = writePointer_;
    const uint32_t cachedSize = size();
    const uint32_t nbElements = std::min(nbElementsToPrint, cachedSize);

    *out << "<div class=\"queuedetail\">" << std::endl;
    *out << "<table>" << std::endl;
    
    *out << "<tr>" << std::endl;
    
    for (uint32_t i=cachedReadPointer; i < cachedReadPointer+nbElements; ++i)
    {
      uint32_t pos = i % cachedSize;
      *out << "  <th>" << pos << "</th>" << std::endl;
    }
    
    *out << "</tr>" << std::endl;
    
    *out << "<tr>" << std::endl;

    for (uint32_t i=cachedReadPointer; i < cachedReadPointer+nbElements; ++i)
    {
      uint32_t pos = i % cachedSize;
      
      if ( ( cachedWritePointer >= cachedReadPointer && i < cachedWritePointer ) ||
           ( cachedWritePointer < cachedReadPointer && i < cachedWritePointer + cachedSize) )
      {
        *out << "  <td style=\"background:#51ef9e\">";
        try
        {
          formatter( container_.at(pos), out );
        }
        catch (std::out_of_range&)
        {
          *out << "n/a";
        }
        *out << "</td>" << std::endl;
      }
      else
      {
        *out << "  <td>&nbsp;</td>" << std::endl;
      }
    }
    
    *out << "</tr>" << std::endl;
    *out << "</table>" << std::endl;
    *out << "</div>" << std::endl;
  }

  
  template <class T>
  void OneToOneQueue<T>::printVerticalHtml
  (
    xgi::Output *out
  )
  {
    printVerticalHtml( out, size() );
  }
  
  
  template <class T>
  void OneToOneQueue<T>::printVerticalHtml
  (
    xgi::Output  *out,
    const uint32_t nbElementsToPrint
  )
  {
    // cache values which might change during the printout
    // Note: we do not want to lock the queue for the debug 
    // printout. Thus, be prepared that some elements are no
    // longer there when we want to print them.
    const uint32_t cachedReadPointer = readPointer_;
    const uint32_t cachedWritePointer = writePointer_;
    const uint32_t cachedElements = elements();
    const uint32_t cachedSize = size();
    const uint32_t nbElements = std::min(nbElementsToPrint, cachedSize);
    
    *out << "<div class=\"queuedetail\">" << std::endl;
    *out << "<table>" << std::endl;
    
    *out << "<tr>" << std::endl;
    *out << "  <th colspan=\"2\">";
    *out << name_;
    *out << "<br/>";
    *out << " read="     << cachedReadPointer;
    *out << " write="    << cachedWritePointer;
    *out << " size="     << cachedSize;
    *out << " elements=" << cachedElements;
    *out << "</th>" << std::endl;
    *out << "</tr>" << std::endl;

    for (uint32_t i=cachedReadPointer; i < cachedReadPointer+nbElements; ++i)
    {
      uint32_t pos = i % cachedSize;
      
      *out << "<tr>" << std::endl;
      *out << "  <th>" << pos << "</th>" << std::endl;
      
      if ( ( cachedWritePointer >= cachedReadPointer && i < cachedWritePointer ) ||
           ( cachedWritePointer < cachedReadPointer && i < cachedWritePointer + cachedSize) )
      {
        *out << "  <td style=\"background-color:#51ef9e\">";
        try
        {
          formatter( container_.at(pos), out );
        }
        catch (std::out_of_range&)
        {
          *out << "n/a";
        }
        *out << "</td>" << std::endl;
      }
      else
      {
        *out << "  <td>&nbsp;</td>" << std::endl;
      }
      
      *out << "</tr>" << std::endl;
    }
    
    *out << "</table>" << std::endl;
    *out << "</div>" << std::endl;
  }
  
  
  template <class T>
  inline void OneToOneQueue<T>::formatter(T element, std::ostringstream* out)
  {
    *out << element;
  }
  
  
} // namespace evb

#endif // _evb_OneToOneQueue_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
