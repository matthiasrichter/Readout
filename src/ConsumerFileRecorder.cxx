#include "Consumer.h"
#include "ReadoutUtils.h"


class ConsumerFileRecorder: public Consumer {
  public: 
  ConsumerFileRecorder(ConfigFile &cfg, std::string cfgEntryPoint):Consumer(cfg,cfgEntryPoint) {
    counterBytesTotal=0;
    fp=NULL;
    
    fileName=cfg.getValue<std::string>(cfgEntryPoint + ".fileName");
    if (fileName.length()>0) {
      theLog.log("Recording to %s",fileName.c_str());
      fp=fopen(fileName.c_str(),"wb");
      if (fp==NULL) {
        theLog.log("Failed to create file");
      }
    }
    if (fp==NULL) {
      theLog.log("Recording disabled");
    } else {
      theLog.log("Recording enabled");
    }
    
    std::string sMaxBytes;
    if (cfg.getOptionalValue<std::string>(cfgEntryPoint + ".bytesMax",sMaxBytes)==0) {
      counterBytesMax=ReadoutUtils::getNumberOfBytesFromString(sMaxBytes.c_str());
      if (counterBytesMax) {
        theLog.log("Maximum recording size: %lld bytes",counterBytesMax);
      }
    }
  }
  ~ConsumerFileRecorder() {
    closeRecordingFile();
  }
  
  // fwrite function with partial write auto-retry
  int writeToFile(FILE *fp, void *data, size_t numberOfBytes) {
    unsigned char *buffer=(unsigned char *)data;
    for (int i=0;i<1024;i++) {
      //theLog.log("write %ld @ %lp",numberOfBytes,buffer);
      size_t bytesWritten=fwrite(buffer,1,numberOfBytes,fp);
      //if (bytesWritten<0) {break;}
      if (bytesWritten>numberOfBytes) {break;}
      if (bytesWritten==0) {usleep(1000);}
      if (bytesWritten==numberOfBytes) {return 0;}
      numberOfBytes-=bytesWritten;
      buffer=&buffer[bytesWritten];
    }
    return -1;
  }
  
  int pushData(DataBlockContainerReference &b) {

    for(;;) {
      if (fp!=NULL) {
        void *ptr;
        size_t size;
        // write header
        // as-is, some fields like data pointer will not be meaningful in file unless corrected. todo: correct them, e.g. replace data pointer by file offset.
        ptr=&b->getData()->header;
        size=b->getData()->header.headerSize;
        //theLog.log("Writing header: %ld bytes @ %lp",(long)size,ptr);
        if ((counterBytesMax)&&(counterBytesTotal+size>counterBytesMax)) {theLog.log("Maximum file size reached"); closeRecordingFile(); return 0;}
        //if (writeToFile(fp,ptr,size)) {
        if (fwrite(ptr,size,1,fp)!=1) {
          break;
        }
        // write payload data     
        counterBytesTotal+=size;        
        ptr=b->getData()->data;
        size=b->getData()->header.dataSize;
        //theLog.log("Writing payload: %ld bytes @ %lp",(long)size,ptr);
        if ((counterBytesMax)&&(counterBytesTotal+size>counterBytesMax)) {theLog.log("Maximum file size reached"); closeRecordingFile(); return 0;}
        if ((size>0)&&(ptr!=nullptr)) {
          //if (writeToFile(fp,ptr,size)) {
          if (fwrite(ptr,size,1,fp)!=1) {
            break;
          }
        }
        counterBytesTotal+=size;
        //theLog.log("File size: %ld bytes",(long)counterBytesTotal);
      }
      return 0;
    }
    theLog.log("File write error");
    closeRecordingFile();
    return -1;
  }
  private:
    unsigned long long counterBytesTotal;
    unsigned long long counterBytesMax=0;
    FILE *fp;
    int recordingEnabled;
    std::string fileName;
    void closeRecordingFile() {
      if (fp!=NULL) {
        theLog.log("Closing %s",fileName.c_str());
        fclose(fp);
        fp=NULL;
      }
    }
};


std::unique_ptr<Consumer> getUniqueConsumerFileRecorder(ConfigFile &cfg, std::string cfgEntryPoint) {
  return std::make_unique<ConsumerFileRecorder>(cfg, cfgEntryPoint);
}
