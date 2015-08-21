#include "MantidQtCustomInterfaces/Tomography/StackOfImagesDirs.h"

#include <boost/algorithm/string.hpp>

#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>

namespace MantidQt {
namespace CustomInterfaces {

const std::string StackOfImagesDirs::g_descr =
    "A directory (folder) that contains subdirectories with names "
    "starting with:\n- 'Data' (for sample images),\n- 'Flat' (for white "
    "bean images),\n- 'Dark' (for dark images)\nThe first one is "
    "mandatory whereas the other two are optional.";

const std::string StackOfImagesDirs::g_sampleNamePrefix = "data";
const std::string StackOfImagesDirs::g_flatNamePrefix = "flat";
const std::string StackOfImagesDirs::g_darkNamePrefix = "dark";
const std::string StackOfImagesDirs::g_processedNamePrefix = "processed";
const std::string StackOfImagesDirs::g_prefilteredNamePrefix = "pre_filtered";

StackOfImagesDirs::StackOfImagesDirs(const std::string &path) : m_valid(false) {
  findStackDirs(path);
}

std::string StackOfImagesDirs::status() const {
  if (m_valid)
    return "Stack of images is correct";
  else
    return "There are errors in the directories and/or files";
}

std::vector<std::string> StackOfImagesDirs::sampleFiles() const {
  return findImgFiles(m_sampleDir);
}
std::vector<std::string> StackOfImagesDirs::flatFiles() const {
  return findImgFiles(m_flatDir);
}
std::vector<std::string> StackOfImagesDirs::darkFiles() const {
  return findImgFiles(m_darkDir);
}

std::vector<std::string> StackOfImagesDirs::findImgFiles(const std::string &path) const {
  std::vector<std::string> fnames;
  Poco::File dir(path);
  if (!dir.isDirectory() || !dir.exists())
    return fnames;

  // as an alternative could also use Poco::Glob to find the files
  Poco::DirectoryIterator it(dir);
  Poco::DirectoryIterator end;
  while (it != end) {
    const std::string name = it.name();
    // TODO: filter names by extension?
    if (it->isFile()) {
      fnames.push_back(it.path().toString());
    }

    ++it;
  }

  return fnames;
}

void StackOfImagesDirs::findStackDirs(const std::string &path) {
  Poco::File dir(path);
  if (!dir.isDirectory() || !dir.exists())
    return;

  Poco::DirectoryIterator end;
  for (Poco::DirectoryIterator it(dir); it != end; ++it) {
    if (!it->isDirectory()) {
      continue;
    }

    const std::string name = it.name();

    if (boost::iequals(name.substr(0, g_sampleNamePrefix.length()), g_sampleNamePrefix)) {
      m_sampleDir = it.path().toString();
    } else if (boost::iequals(name.substr(0, g_flatNamePrefix.length()), g_flatNamePrefix)) {
      m_flatDir = name;
    } else if (boost::iequals(name.substr(0, g_darkNamePrefix.length()), g_darkNamePrefix)) {
      m_darkDir = name;
    }
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
