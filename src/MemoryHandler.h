#include <string>
#include <memory>
#include "ReadoutUtils.h"
#include <Common/DataBlockContainer.h>
#include <Common/Fifo.h>

#include <stdint.h>
#include <mutex>

struct MemoryRegion {
  unsigned long long size;  // size of memory block
  void *ptr; // pointer to memory block
  std::string name; // name of the region
  
  // todo: add callback to release region afterwards
  
  unsigned long long usedSize; // amount of memory in use
};


extern std::unique_ptr<MemoryRegion> bigBlock;


// todo: named list of big blocks, with get/set functions

// todo: big blocks: one different per equipment to avoid lock


// a big block of memory for I/O

class MemoryHandler {

  public:
    MemoryHandler(int pageSize, int numberOfPages);
    ~MemoryHandler();
  
    void *getPage();
    void freePage(void *);
   
  private:
    size_t memorySize;     // total size of buffer
    size_t pageSize;       // size of each superpage in buffer (not the one of getpagesize())
    size_t numberOfPages;  // number of pages allocated
    uint8_t * baseAddress; // base address of buffer
    std::unique_ptr<AliceO2::Common::Fifo<long>> pagesAvailable;  // a buffer to keep track of individual pages. storing offset (with respect to base address) of pages available
};



class DataBlockContainerFromMemoryHandler : public DataBlockContainer {
  private:
  std::shared_ptr<MemoryHandler> mMemoryHandler;
  
  public:
  DataBlockContainerFromMemoryHandler(std::shared_ptr<MemoryHandler> const & h) {

    mMemoryHandler=h;

    data=nullptr;
    try {
      data=new DataBlock;
    } catch (...) {
      throw __LINE__;
    }
    data->data=(char *)h->getPage();
    if (data->data==nullptr) {
      delete data;
      throw __LINE__;
    }
  }
  
  ~DataBlockContainerFromMemoryHandler() {
    if (data!=nullptr) {
      //printf("~DataBlockContainerFromMemoryHandler(%p) : header=%p data=%p\n",this,data,data->data);
    
      if (data->data!=nullptr) {
        mMemoryHandler->freePage(data->data);
      }
      
      delete data;
    }
  }
};
