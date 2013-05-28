#include <fstream>
#include <sstream>
#include <iomanip>

#include "evb/bu/FileHandler.h"
#include "evb/bu/LumiHandler.h"
#include "evb/bu/StateMachine.h"
#include "evb/Exception.h"


evb::bu::LumiHandler::LumiHandler
(
  const uint32_t buInstance,
  const boost::filesystem::path& rawDataDir,
  const boost::filesystem::path& metaDataDir,
  const uint32_t lumiSection,
  const uint32_t maxEventsPerFile,
  const uint32_t numberOfWriters
) :
buInstance_(buInstance),
rawDataDir_(rawDataDir),
metaDataDir_(metaDataDir),
lumiSection_(lumiSection),
maxEventsPerFile_(maxEventsPerFile),
numberOfWriters_(numberOfWriters),
index_(0),
nextFileHandler_(0),
eventsPerLS_(0),
filesPerLS_(0)
{
  fileHandlers_.resize(numberOfWriters_);
}


evb::bu::LumiHandler::~LumiHandler()
{}


evb::bu::FileHandlerPtr evb::bu::LumiHandler::getFileHandler(boost::shared_ptr<StateMachine> stateMachine)
{
  FileHandlerPtr fileHandler = fileHandlers_[nextFileHandler_];

  if ( fileHandler.get() == 0 || fileHandler->getAllocatedEventCount() >= maxEventsPerFile_ )
  {
    fileHandler = FileHandlerPtr(
      new FileHandler(stateMachine, buInstance_, rawDataDir_, metaDataDir_, lumiSection_, index_++)
    );
    fileHandlers_[nextFileHandler_] = fileHandler;
    ++filesPerLS_;
  }

  fileHandler->incrementAllocatedEventCount();
  ++eventsPerLS_;
  
  ++nextFileHandler_;
  if ( nextFileHandler_ == numberOfWriters_ )
    nextFileHandler_ = 0;

  return fileHandler;
}


void evb::bu::LumiHandler::close()
{
  for (FileHandlers::iterator it = fileHandlers_.begin(), itEnd = fileHandlers_.end();
       it != itEnd; ++it)
  {
    if (*it) (*it)->close();
  }
  fileHandlers_.clear();

  writeJSON();
}


void evb::bu::LumiHandler::writeJSON() const
{
  const boost::filesystem::path jsonDefFile = metaDataDir_ / "EoLS.jsd";
  defineJSON(jsonDefFile);

  std::ostringstream fileNameStream;
  fileNameStream
    << "EoLS_" << std::setfill('0') << std::setw(4) << lumiSection_ << ".jsn";
  const boost::filesystem::path jsonFile = metaDataDir_ / fileNameStream.str();

  if ( boost::filesystem::exists(jsonFile) )
  {
    std::ostringstream oss;
    oss << "The JSON file " << jsonFile.string() << " already exists.";
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }
  
  std::ofstream json(jsonFile.string().c_str());
  json << "{"                                                         << std::endl;
  json << "   \"Data\" : [ \""     << eventsPerLS_  << "\", \""
                                   << filesPerLS_   << "\" ],"        << std::endl;
  json << "   \"Definition\" : \"" << jsonDefFile.string()  << "\","  << std::endl;
  json << "   \"Source\" : \"BU-"  << buInstance_   << "\""           << std::endl;
  json << "}"                                                         << std::endl;
  json.close();
}


void evb::bu::LumiHandler::defineJSON(const boost::filesystem::path& jsonDefFile) const
{
  std::ofstream json(jsonDefFile.string().c_str());
  json << "{"                                                 << std::endl;
  json << "   \"legend\" : ["                                 << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NEvents\","                  << std::endl;
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NFiles\","                   << std::endl;
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      }"                                           << std::endl;
  json << "   ],"                                             << std::endl;
  json << "   \"file\" : \"" << jsonDefFile.string() << "\""  << std::endl;
  json << "}"                                                 << std::endl;
  json.close();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
