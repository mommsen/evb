#ifndef _evb_OneToOneQueue_h_
#define _evb_OneToOneQueue_h_

#include <stdexcept>
#include <stdint.h>
#include <vector>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/utility/enable_if.hpp>

#include "cgicc/HTMLClasses.h"
#include "evb/Exception.h"
#include "toolbox/AllocPolicy.h"
#include "toolbox/PolicyFactory.h"
#include "toolbox/net/URN.h"
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

    template <class C>
    OneToOneQueue(C* evbApplication, const std::string& name);

    ~OneToOneQueue();

    /**
     * Enqueue the element.
     * Return false if the element cannot be enqueued.
     */
    bool enq(const T&);

    /**
     * Enqueue the element.
     * If the queue is full wait until it becomes non-full.
     */
    void enqWait(const T&);

    /**
     * Dequeue an element.
     * Return false if no element can be dequeued.
     */
    bool deq(T&);

    /**
     * Dequeue an element.
     * If the queue is empty wait until is has become non-empty.
     */
    void deqWait(T&);

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
     * Returns true if the queue is full.
     */
    bool full() const;

    /**
     * Resizes the queue.
     * Throws an exception if queue is not empty.
     */
    void resize(const uint32_t size);

    /**
     * Remove all elements from the queue
     */
    void clear();

    /**
     * Return a cgicc snipped representing the queue
     */
    cgicc::div getHtmlSnipped() const;

    /**
     * Return a cgicc snipped showing all elements horizontally
     */
    cgicc::div getHtmlSnippedHorizontal() const;

    /**
     * Return a cgicc snipped showing horizontally upto nbElementsToPrint
     */
    cgicc::div getHtmlSnippedHorizontal(const uint32_t nbElementsToPrint) const;

    /**
     * Return a cgicc snipped showing all elements vertically
     */
    cgicc::div getHtmlSnippedVertical() const;

    /**
     * Return a cgicc snipped showing vertically upto nbElementsToPrint
     */
    cgicc::div getHtmlSnippedVertical(const uint32_t nbElementsToPrint) const;


  private:

    const std::string name_;
    const toolbox::net::URN urn_;
    volatile uint32_t readPointer_;
    volatile uint32_t writePointer_;
    T* container_;
    volatile uint32_t size_;
    mutable volatile bool printingElements_;
  };


  //------------------------------------------------------------------
  // Implementation follows
  //------------------------------------------------------------------

  namespace detail
  {
    template <class U>
    struct is_shared_ptr
      : boost::mpl::false_ {};
    template <class U>
    struct is_shared_ptr<boost::shared_ptr<U> >
      : boost::mpl::true_ {};

    template<typename T>
    typename boost::disable_if<is_shared_ptr<T> >::type
    inline formatter(const T element, std::ostringstream* out)
    {
      *out << element;
    }

    template<typename T>
    typename boost::enable_if<is_shared_ptr<T> >::type
    inline formatter(const T element, std::ostringstream* out)
    {
      if ( element.get() )
        *out << element;
    }
  } // namespace detail


  template <class T>
  template <class C>
  OneToOneQueue<T>::OneToOneQueue(C* evbApplication,const std::string& name) :
    name_(name),
    urn_(evbApplication->getURN()),
    readPointer_(0),
    writePointer_(0),
    container_(0),
    size_(1),
    printingElements_(false)
  {
    evbApplication->registerQueueCallback(name,
                                          boost::bind(&OneToOneQueue<T>::getHtmlSnippedVertical,this));
  }


  template <class T>
  OneToOneQueue<T>::~OneToOneQueue()
  {
    clear();
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
  bool OneToOneQueue<T>::full() const
  {
    volatile const uint32_t nextElement = (writePointer_ + 1) % size_;
    return ( nextElement == readPointer_ );
  }


  template <class T>
  void OneToOneQueue<T>::resize(const uint32_t size)
  {
    while ( printingElements_ ) {};

    if ( !empty() )
    {
      XCEPT_RAISE(exception::FIFO,
                  "Cannot resize the non-empty queue " + name_);
    }

    toolbox::net::URN urn(name_, "alloc");
    toolbox::PolicyFactory* factory = toolbox::getPolicyFactory();
    toolbox::AllocPolicy* policy = static_cast<toolbox::AllocPolicy*>(factory->getPolicy(urn, "alloc"));

    if ( container_ )
      policy->free(container_, sizeof(container_));

    readPointer_ = writePointer_ = 0;
    size_ = size+1;

    try
    {
      container_ = static_cast<T*>( policy->alloc(sizeof(T) * size_) );
    }
    catch (toolbox::exception::Exception& e)
    {
      std::ostringstream msg;
      msg << "Failed to allocate memory for " << size << " elements in queue " << name_;
      XCEPT_RETHROW (exception::FIFO, msg.str(), e);
    }
  }


  template <class T>
  bool OneToOneQueue<T>::enq(const T& element)
  {
    while ( printingElements_ ) {};
    volatile const uint32_t nextElement = (writePointer_ + 1) % size_;
    if ( nextElement == readPointer_ ) return false;
    new (&container_[writePointer_]) T(element);
    writePointer_ = nextElement;
    return true;
  }


  template <class T>
  void OneToOneQueue<T>::enqWait(const T& element)
  {
    while ( ! enq(element) ) ::usleep(1000);
  }


  template <class T>
  bool OneToOneQueue<T>::deq(T& element)
  {
    if ( empty() ) return false;

    while ( printingElements_ ) {};
    volatile const uint32_t nextElement = (readPointer_ + 1) % size_;
    element = container_[readPointer_];
    container_[readPointer_].~T();
    readPointer_ = nextElement;
    return true;
  }


  template <class T>
  void OneToOneQueue<T>::deqWait(T& element)
  {
    while ( ! deq(element) ) ::usleep(1000);
  }


  template <class T>
  void OneToOneQueue<T>::clear()
  {
    while ( printingElements_ ) {};
    T element;
    while ( deq(element) ) {};
  }


  template <class T>
  cgicc::div OneToOneQueue<T>::getHtmlSnipped() const
  {
    // cache values which might change during the printout
    const uint32_t cachedSize = size();
    const uint32_t cachedElements = elements();
    const double fillFraction = cachedSize > 0 ? 100. * cachedElements / cachedSize : 0;
    const uint16_t fullWidth = static_cast<uint8_t>(fillFraction + 0.5);
    const uint16_t emptyWidth = static_cast<uint8_t>((100-fillFraction) + 0.5);

    using namespace cgicc;

    table queueTable;
    queueTable
      .set("onclick","window.open('/"+urn_.toString()+"/"+name_ +"','_blank')")
      .set("onmouseover","this.style.cursor = 'pointer'; document.getElementById('"+name_+"').style.visibility = 'visible';")
      .set("onmouseout","document.getElementById('"+name_+"').style.visibility = 'hidden';");
    queueTable.add(colgroup()
                   .add(col().set("style","width:"+boost::lexical_cast<std::string>(fullWidth)+"%"))
                   .add(col().set("style","width:"+boost::lexical_cast<std::string>(emptyWidth)+"%")));
    queueTable.add(tr()
                   .add(th(name_).set("colspan","2")));
    queueTable.add(tr()
                   .add(td(" ").set("class","xdaq-evb-queue-full"))
                   .add(td(" ").set("class","xdaq-evb-queue-empty")));
    queueTable.add(tr()
                   .add(td(boost::lexical_cast<std::string>(cachedElements)+" / "+boost::lexical_cast<std::string>(cachedSize))
                        .set("colspan","2")));

    cgicc::div queueFloat;
    queueFloat
      .set("id",name_)
      .set("class","xdaq-evb-queuefloat");
    queueFloat.add(getHtmlSnippedHorizontal(10));

    cgicc::div div;
    div.set("class","xdaq-evb-queue");
    div.add(queueTable);
    div.add(queueFloat);

    return div;
  }


  template <class T>
  cgicc::div OneToOneQueue<T>::getHtmlSnippedHorizontal() const
  {
    return getHtmlSnippedHorizontal( size() );
  }


  template <class T>
  cgicc::div OneToOneQueue<T>::getHtmlSnippedHorizontal(const uint32_t nbElementsToPrint) const
  {
    printingElements_ = true;
    const uint32_t nbElements = std::min(nbElementsToPrint, size());

    using namespace cgicc;

    cgicc::div queueDetail;
    queueDetail.set("class","xdaq-evb-queuedetail");

    if ( nbElements > 0 )
    {
      tr headerRow,tableRow;

      for (uint32_t i=readPointer_; i < readPointer_+nbElements; ++i)
      {
        uint32_t pos = i % size();
        headerRow.add(th(boost::lexical_cast<std::string>(pos)).set("style","width:5em"));

        if ( ( writePointer_ >= readPointer_ && i < writePointer_ ) ||
             ( writePointer_ < readPointer_ && i < writePointer_ + size()) )
        {
          std::ostringstream content;
          try
          {
            detail::formatter(container_[pos], &content);
          }
          catch(...)
          {
            content << "n/a";
          }

          tableRow.add(td(content.str()));
        }
        else
        {
          tableRow.add(td("&nbsp;"));
        }
      }

      queueDetail.add(table()
                      .add(headerRow)
                      .add(tableRow));
    }

    printingElements_ = false;
    return queueDetail;
  }


  template <class T>
  cgicc::div OneToOneQueue<T>::getHtmlSnippedVertical() const
  {
    return getHtmlSnippedVertical( size() );
  }


  template <class T>
  cgicc::div OneToOneQueue<T>::getHtmlSnippedVertical(const uint32_t nbElementsToPrint) const
  {
    printingElements_ = true;
    const uint32_t nbElements = std::min(nbElementsToPrint, size());

    using namespace cgicc;

    table table;
    table.set("class","xdaq-table-vertical").set("style","width:100%");
    table.add(colgroup()
              .add(col().set("style","width:5em"))
              .add(col()));

    std::ostringstream str;
    str << name_ << "<br/>";
    str << " read="     << readPointer_;
    str << " write="    << writePointer_;
    str << " size="     << size();
    str << " elements=" << elements();

    table.add(tr()
              .add(th(str.str()).set("colspan","2")));


    for (uint32_t i=readPointer_; i < readPointer_+nbElements; ++i)
    {
      tr tr;
      uint32_t pos = i % size();
      tr.add(th(boost::lexical_cast<std::string>(pos)));

      if ( ( writePointer_ >= readPointer_ && i < writePointer_ ) ||
           ( writePointer_ < readPointer_ && i < writePointer_ + size()) )
      {
        std::ostringstream content;
        try
        {
          detail::formatter(container_[pos], &content);
        }
        catch(...)
        {
          content << "n/a";
        }

        tr.add(td(content.str()));
      }
      else
      {
        tr.add(td("&nbsp;"));
      }
      table.add(tr);
    }
    printingElements_ = false;

    cgicc::div queueDetail;
    queueDetail.set("class","xdaq-evb-queuedetail");
    queueDetail.add(table);

    return queueDetail;
  }

} // namespace evb

#endif // _evb_OneToOneQueue_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
