#include <sstream>

#include "evb/bu/StreamHandler.h"
#include "evb/Exception.h"


evb::bu::StreamHandler::StreamHandler
(
  const std::string& streamFileName,
  ConfigurationPtr configuration
) :
streamFileName_(streamFileName),
configuration_(configuration),
index_(0),
fileStatisticsFIFO_("currentFileStatisticsFIFO")
{
  fileStatisticsFIFO_.resize(configuration->fileStatisticsFIFOCapacity);
}


void evb::bu::StreamHandler::writeEvent(const EventPtr& event)
{
  const FileHandlerPtr fileHandler = getFileHandlerForLumiSection(event->lumiSection());

  event->writeToDisk(fileHandler, configuration_->calculateAdler32);
  fileHandler->eventWritten( event->eventNumber() );

  ::usleep(configuration_->sleepBetweenEvents);
}


evb::bu::FileHandlerPtr evb::bu::StreamHandler::getFileHandlerForLumiSection(const uint32_t lumiSection)
{
  boost::mutex::scoped_lock sl(fileHandlersMutex_);

  FileHandlers::iterator pos = fileHandlers_.lower_bound(lumiSection);
  if ( pos == fileHandlers_.end() || fileHandlers_.key_comp()(lumiSection,pos->first) )
  {
    // new lumi section
    pos = fileHandlers_.insert(pos,
      FileHandlers::value_type(lumiSection,getNewFileHandler(lumiSection))
    );
  }
  else if ( pos->second->getNumberOfEventsWritten() >= configuration_->maxEventsPerFile )
  {
    // too many events
    closeFileHandler(pos->second);
    fileHandlers_.erase(pos);
    const std::pair<FileHandlers::iterator,bool> result =
      fileHandlers_.insert(
        FileHandlers::value_type(lumiSection,getNewFileHandler(lumiSection))
      );
    if ( result.second )
    {
      pos = result.first;
    }
    else
    {
      std::ostringstream oss;
      oss << "Failed to insert new file handler for stream " << streamFileName_
        << " and lumi section " << lumiSection;
      XCEPT_RAISE(exception::DiskWriting, oss.str());
    }
  }
  return pos->second;
}


evb::bu::FileHandlerPtr evb::bu::StreamHandler::getNewFileHandler(const uint32_t lumiSection)
{
  std::ostringstream fileName;
  fileName << streamFileName_ << "_" << std::hex << static_cast<unsigned int>(++index_);
  const FileStatisticsPtr fileStatistics( new FileStatistics(lumiSection,fileName.str()) );
  return FileHandlerPtr( new FileHandler(fileStatistics) );
}


bool evb::bu::StreamHandler::closeFilesIfOpenedBefore(const time_t& time)
{
  boost::mutex::scoped_lock sl(fileHandlersMutex_);

  bool fileClosed = false;

  FileHandlers::iterator it = fileHandlers_.begin();
  while ( it != fileHandlers_.end() )
  {
    if ( it->second->getCreationTime() < time )
    {
      closeFileHandler(it->second);
      fileHandlers_.erase(it++);
      fileClosed = true;
    }
    else
    {
      ++it;
    }
  }
  return fileClosed;
}


void evb::bu::StreamHandler::closeFiles()
{
  boost::mutex::scoped_lock sl(fileHandlersMutex_);

  for(FileHandlers::const_iterator it = fileHandlers_.begin(), itEnd = fileHandlers_.end();
      it != itEnd; ++it)
  {
    closeFileHandler(it->second);
  }
  fileHandlers_.clear();
}


void evb::bu::StreamHandler::closeFileHandler(const FileHandlerPtr& fileHandler)
{
  FileStatisticsPtr fileStatistics = fileHandler->closeAndGetFileStatistics();
  fileStatisticsFIFO_.enqWait(fileStatistics);
}


bool evb::bu::StreamHandler::getFileStatistics(FileStatisticsPtr& fileStatistics)
{
  return fileStatisticsFIFO_.deq(fileStatistics);
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
