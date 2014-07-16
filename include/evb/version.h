#ifndef _evb_version_h_
#define _evb_version_h_

#include "config/PackageInfo.h"

#define EVB_VERSION_MAJOR 1
#define EVB_VERSION_MINOR 20
#define EVB_VERSION_PATCH 5
#define EVB_PREVIOUS_VERSIONS "1.20.0,1.20.1,1.20.2,1.20.4"


#define EVB_VERSION_CODE PACKAGE_VERSION_CODE(EVB_VERSION_MAJOR,EVB_VERSION_MINOR,EVB_VERSION_PATCH)
#ifndef EVB_PREVIOUS_VERSIONS
#define EVB_FULL_VERSION_LIST  PACKAGE_VERSION_STRING(EVB_VERSION_MAJOR,EVB_VERSION_MINOR,EVB_VERSION_PATCH)
#else
#define EVB_FULL_VERSION_LIST  EVB_PREVIOUS_VERSIONS "," PACKAGE_VERSION_STRING(EVB_VERSION_MAJOR,EVB_VERSION_MINOR,EVB_VERSION_PATCH)
#endif

namespace evb
{
  const std::string package = "evb";
  const std::string versions = EVB_FULL_VERSION_LIST;
  const std::string version = PACKAGE_VERSION_STRING(EVB_VERSION_MAJOR,EVB_VERSION_MINOR,EVB_VERSION_PATCH);
  const std::string description = "The CMS event builder";
  const std::string summary = "Event builder library";
  const std::string authors = "Remi Mommsen";
  const std::string link = "";

  config::PackageInfo getPackageInfo();

  void checkPackageDependencies()
    throw (config::PackageInfo::VersionException);

  std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif
