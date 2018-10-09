// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "XMLWriter.h"
#include "Component.h"
#include <ctime>
#include <queue>
#include <set>
#include <map>
#include <iostream>

XMLWriter::XMLWriter(const std::string& name, Component* const startPoint) :
  m_startPoint(startPoint)
{
  const std::string filename = name + "_Definition.xml";
  m_outputFile.open(filename.c_str());
  if ( m_outputFile.fail() )
  {
    std::cout << "Unable to create output file " << filename << ". Exiting." << std::endl;
    exit(EXIT_FAILURE);
  }

  time_t now = time(0);
  char timestamp[64] = { 0 };
  strftime(timestamp, 32, "%Y-%m-%dT%H:%M:%S", localtime(&now));

  m_outputFile << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl; 
  m_outputFile << "<instrument name=\"" << name << "\" date-time=\"" << timestamp << "\">" << std::endl;
  m_outputFile << "<!-- Generated by the ArielToMantidXML tool -->" << std::endl;
  this->writeDefaults();
  this->writeSourceSample();
}

XMLWriter::~XMLWriter(void)
{
  m_outputFile << std::endl << "</instrument>" << std::endl;
  m_outputFile.close();
}

void XMLWriter::writeDetectors()
{
  m_outputFile << "<!-- LIST OF PHYSICAL COMPONENTS (which the instrument consists of) -->" << std::endl;
  m_outputFile << "<!-- detector components -->" << std::endl;

  m_outputFile << "<component type=\"monitors\" idlist=\"monitors\">" << std::endl;
  m_outputFile << "  <location />" << std::endl;
  m_outputFile << "</component>" << std::endl;

  // For this we want to do a level traversal, so a queue is what's needed
  std::queue<Component*> trav;
  trav.push(m_startPoint);
  std::set<std::string> done;
  while ( ! trav.empty() )
  {
    // Get the component from the front of the queue and then pop it off
    Component *current = trav.front();
    trav.pop();
    std::vector<Component *> children = current->getChildren();
    std::set<std::string> types;
    std::multimap<std::string, Component*> compsByType;
    // Go through the children of the current component
    for (const Component *child : children) {
      // A particular type can appear more than once in the tree - only want it once though
      if (done.find(child->type()) == done.end()) {
        // Push the child onto the queue
        trav.push(child);
        // And add it to the list of 'done' components
        done.insert(child->type());
      }

      // Need to see if the type is the same for all children (in which case we can compactise the output)
      types.insert(child->type());
      compsByType.emplace(child->type(), child);
        }

    // Now do the output for the current component (if it has children)
    if ( current != m_startPoint )
    {
      m_outputFile << "<type name=\"" << current->type() << "\"";
      if ( ! current->hasChildren() ) m_outputFile << " is=\"detector\"";  
      m_outputFile << ">" << std::endl;
      m_outputFile << "<properties />" << std::endl;
    }

    // Have to go through children again, printing them out
    std::set<std::string>::iterator iter;
    for (iter = types.begin(); iter != types.end(); ++iter)
    {
      if ( compsByType.count(*iter) > 1 )
      {
        if ( (*iter) == "monitor" )
        {
          m_outputFile << "<type name=\"monitors\">" << std::endl;
          m_outputFile << "<component type=\"monitor\" mark-as=\"monitor\">" << std::endl;
        }
        else
        {
          m_outputFile << "<component type=\"" << (*iter) << "\">" << std::endl;
        }

        std::multimap<std::string, Component*>::iterator iter2;
        for (iter2 = compsByType.lower_bound(*iter); iter2 != compsByType.upper_bound(*iter); ++iter2)
        {
          m_outputFile << "  <location " << (*iter2).second->printPos() << " name=\"" << (*iter2).second->name() << "\"/>" << std::endl;
        }
        m_outputFile << "</component>" << std::endl;
        if ( (*iter) == "monitor" ) m_outputFile << "</type>" << std::endl;
      }
      else
      {
        Component* current = compsByType.find(*iter)->second;
        std::string property2 = "name";
        // Want to print out "idlist" instead of name for first layer (to link to udets)
        if ( ! current->parent()->parent() ) property2 = "idlist";
        m_outputFile << "<component type=\"" << current->type() << "\" " << property2 << "=\"" << current->name() << "\">" << std::endl;
        m_outputFile << "  <location " << current->printPos() << " />" << std::endl;
        m_outputFile << "</component>" << std::endl;
      }
    }

    if ( current != m_startPoint )
    {
      m_outputFile << "</type>" << std::endl;
    }

  }
}

void XMLWriter::writeDefaults()
{
  m_outputFile << "<defaults>" << std::endl;
  m_outputFile << "  <length unit=\"meter\" />" << std::endl;
  m_outputFile << "  <angle unit=\"degree\" />" << std::endl;

  m_outputFile << "  <reference-frame>" << std::endl;
  m_outputFile << "    <along-beam axis=\"z\" />" << std::endl;
  m_outputFile << "    <pointing-up axis=\"y\" />" << std::endl;
  m_outputFile << "    <handedness val=\"right\" />" << std::endl;
  m_outputFile << "    <origin val=\"sample\" />" << std::endl;
  m_outputFile << "  </reference-frame>" << std::endl;

  m_outputFile << "  <offsets spherical=\"delta\" />" << std::endl;
  m_outputFile << "</defaults>" << std::endl;
}

void XMLWriter::writeSourceSample()
{
  m_outputFile << "<!--  source and sample-position components -->" << std::endl;
  m_outputFile << "<component type=\"undulator\">" << std::endl;

  // This next line gets the primary flightpath and inserts it
  m_outputFile << "  <location z=\"-" << m_startPoint->findL1() << "\" />" << std::endl; 

  m_outputFile << "</component>" << std::endl;
  m_outputFile << "<component type=\"nickel-holder\">" << std::endl;
  m_outputFile << "  <location />"  << std::endl;
  m_outputFile << "</component>" << std::endl;
  m_outputFile << "<!--  DEFINITION OF TYPES -->"  << std::endl;
  m_outputFile << "<!--  Source types -->" << std::endl;
  m_outputFile << "<type name=\"undulator\" is=\"Source\">" << std::endl;
  m_outputFile << "  <properties />"  << std::endl;
  m_outputFile << "</type>" << std::endl;
  m_outputFile << "<!--  Sample-position types -->" << std::endl; 
  m_outputFile << "<type name=\"nickel-holder\" is=\"SamplePos\">" << std::endl;
  m_outputFile << "  <properties />"  << std::endl;
  m_outputFile << "</type>" << std::endl;
}
