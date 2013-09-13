#include <boost/filesystem/convenience.hpp>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string.h>
#include <iomanip>
#include <unistd.h>

#include "evb/Exception.h"
#include "evb/bu/FileHandler.h"
#include "xcept/tools.h"


evb::bu::FileHandler::FileHandler
(
  const uint32_t buInstance,
  const uint32_t runNumber,
  const boost::filesystem::path& runRawDataDir,
  const boost::filesystem::path& runMetaDataDir,
  const uint32_t lumiSection
) :
buInstance_(buInstance),
runRawDataDir_(runRawDataDir),
runMetaDataDir_(runMetaDataDir),
fileDescriptor_(0),
fileSize_(0),
eventCount_(0),
adlerA_(1),
adlerB_(0)
{
  std::ostringstream fileNameStream;
  fileNameStream << std::setfill('0') <<
    "run"<< std::setw(6) << runNumber <<
    "_ls" << std::setw(4) << lumiSection <<
    "_index" << std::setw(6) << getNextIndex(lumiSection) <<
    ".raw";
  fileName_ = fileNameStream.str();
  const boost::filesystem::path rawFile = runRawDataDir_ / fileName_;

  if ( boost::filesystem::exists(rawFile) )
  {
    std::ostringstream oss;
    oss << "The output file " << rawFile.string() << " already exists";
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  fileDescriptor_ = open(rawFile.string().c_str(), O_RDWR|O_CREAT|O_TRUNC, S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
  if ( fileDescriptor_ == -1 )
  {
    std::ostringstream oss;
    oss << "Failed to open output file " << rawFile.string()
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }
}


evb::bu::FileHandler::~FileHandler()
{
  close();
}


void* evb::bu::FileHandler::getMemMap(const size_t length)
{
  boost::mutex::scoped_lock sl(mutex_);

  const int result = lseek(fileDescriptor_, fileSize_+length-1, SEEK_SET);
  if ( result == -1 )
  {
    std::ostringstream oss;
    oss << "Failed to stretch the output file " << fileName_.string()
      << " by " << length << " Bytes: " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  // Something needs to be written at the end of the file to
  // have the file actually have the new size.
  // Just writing an empty string at the current file position will do.
  if ( ::write(fileDescriptor_, "", 1) != 1 )
  {
    std::ostringstream oss;
    oss << "Failed to write last byte to output file " << fileName_.string()
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  void* map = mmap(0, length, PROT_WRITE, MAP_SHARED, fileDescriptor_, fileSize_);
  if (map == MAP_FAILED)
  {
    std::ostringstream oss;
    oss << "Failed to mmap the output file " << fileName_.string()
      << ": " << strerror(errno);
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  fileSize_ += length;

  return map;
}


void evb::bu::FileHandler::close()
{
  boost::mutex::scoped_lock sl(mutex_);

  std::string msg = "Failed to close the output file " + fileName_.string();

  try
  {
    if ( fileDescriptor_ )
    {
      if ( ::close(fileDescriptor_) < 0 )
      {
        std::ostringstream oss;
        oss << msg << ": " << strerror(errno);
        XCEPT_RAISE(exception::DiskWriting, oss.str());
      }
      fileDescriptor_ = 0;

      boost::filesystem::rename(runRawDataDir_ / fileName_, runRawDataDir_.parent_path() / fileName_);

      writeJSON();
    }
  }
  catch(std::exception& e)
  {
    msg += ": ";
    msg += e.what();
    XCEPT_RAISE(exception::DiskWriting, msg);
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_RAISE(exception::DiskWriting, msg);
  }
}


void evb::bu::FileHandler::writeJSON() const
{
  const boost::filesystem::path jsonDefFile = runMetaDataDir_ / "rawData.jsd";
  if ( ! boost::filesystem::exists(jsonDefFile) ) defineJSON(jsonDefFile);

  boost::filesystem::path jsonFile = runMetaDataDir_ / fileName_;
  jsonFile.replace_extension("jsn");

  if ( boost::filesystem::exists(jsonFile) )
  {
    std::ostringstream oss;
    oss << "The JSON file " << jsonFile.string() << " already exists";
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  std::ofstream json(jsonFile.string().c_str());
  json << "{"                                                         << std::endl;
  json << "   \"data\" : [ \""     << eventCount_   << "\" ],"        << std::endl;
  json << "   \"definition\" : \"" << jsonDefFile.string()  << "\","  << std::endl;
  json << "   \"source\" : \"BU-"  << buInstance_   << "\""           << std::endl;
  json << "}"                                                         << std::endl;
  json.close();
}


void evb::bu::FileHandler::defineJSON(const boost::filesystem::path& jsonDefFile) const
{
  std::ofstream json(jsonDefFile.string().c_str());
  json << "{"                                                 << std::endl;
  json << "   \"legend\" : ["                                 << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NEvents\","                  << std::endl;
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      }"                                           << std::endl;
  json << "   ],"                                             << std::endl;
  json << "   \"file\" : \"" << jsonDefFile.string() << "\""  << std::endl;
  json << "}"                                                 << std::endl;
  json.close();
}


void evb::bu::FileHandler::calcAdler32(const unsigned char* address, size_t len)
{
  #define MOD_ADLER 65521

  while (len > 0) {
    size_t tlen = (len > 5552 ? 5552 : len);
    len -= tlen;
    do {
      adlerA_ += *address++ & 0xff;
      adlerB_ += adlerA_;
    } while (--tlen);

    adlerA_ %= MOD_ADLER;
    adlerB_ %= MOD_ADLER;
  }

  #undef MOD_ADLER
}


uint16_t evb::bu::FileHandler::getNextIndex(const uint32_t lumiSection)
{
  boost::mutex::scoped_lock sl(indexMutex_);

  if ( lumiSection > lastLumiSection_ )
  {
    index_ = 0;
    lastLumiSection_ = lumiSection;
  }
  else if ( lumiSection < lastLumiSection_ )
  {
    std::ostringstream oss;
    oss << "Received an event from an earlier lumi section " << lumiSection;
    oss << " while processing lumi section " << lastLumiSection_;
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  return index_++;
}


boost::mutex evb::bu::FileHandler::indexMutex_;
uint16_t evb::bu::FileHandler::index_(0);
uint32_t evb::bu::FileHandler::lastLumiSection_(0);


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
