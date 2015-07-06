/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include "GraphGenerator.h"

#include "TreeNode.h"
#include <sstream>
#include <exception>
#include <cstring>

// Note calls to QObject::toStdString() have been replaced with QObject::toLocal8Bit().constData()
// to avoid QString-related aborts under windows.

/// Constructor
Monitor::GraphGenerator::GraphGenerator(TreeNode *root, GvOptionsData gvOpt,
    bool honorDisplayFlag) :
  root_(root), gvOpt_(gvOpt), honorDisplayFlag_(honorDisplayFlag)
{
}

void
Monitor::GraphGenerator::draw ()
{
  // setup output file
  if (gvOpt_.getDot() == "") {
      throw "Null GvOpts. Can't open output file";
  }

  // TODO: check for errors and update status if error
 fout.open(gvOpt_.getDot().c_str());

  if (!fout) {
    throw "Error opening file";
  }

  // erase old values
  // currently, we only call draw once for an instance
  // of graphgenerator, but this could change.
  rMap_.clear();
  wMap_.clear();
  nMap_.clear();

  while (!lifo_.empty()) {
    lifo_.pop();
  }

  fout << "digraph OpenDDS {" << std::endl;
  if (gvOpt_.lrLayout()) {
    fout << "  rankdir=LR;" << std::endl;
  }
  fout << "  node [shape=record,height=.08,fontsize=11];" << std::endl;

  generate(root_);

  while (!lifo_.empty()) {
    fout << lifo_.top();
    lifo_.pop();
  }

  connectNodes();

  // close dot file and generate png
  fout << "}" << std::endl;

  fout.close();

  std::string cmd = gvOpt_.getBin();
  cmd += " -Tpng ";
  cmd += gvOpt_.getDot();
  cmd += " > ";
  cmd += gvOpt_.getPng();
  int ret = system(cmd.c_str());

  if (ret == -1) {
    throw "Error converting dot file to png.";
  }

}

void
Monitor::GraphGenerator::connectNodes()
{
  std::map <std::string, std::string>::const_iterator i;

  // connect the writers to readers
  for (i = wMap_.begin(); i != wMap_.end(); ++i) {
    fout << "\"" << nMap_[i->first] << "\" -> \"" << nMap_[i->second] << "\";" << std::endl;
  }

  // connect readers to writers omitting anything already connected
  for (i = rMap_.begin(); i != rMap_.end(); ++i) {
    if (wMap_[i->second] != i->first) {
      fout << "\"" << nMap_[i->second] << "\" -> \"" << nMap_[i->first] << "\";" << std::endl;
    }
  }

}

void
Monitor::GraphGenerator::generate(TreeNode *t)
{
  if (t == 0) {
    return;
  }

  bool hasSubG = false; // used to know when to push '}' to end subgraphs
  bool stopDescent = false; // flag to stop visiting children.

  // print this node
  // skip the root and make sure we have a key AND a value
  if (t != root_ && t->width() >= 2) {
    if (!honorDisplayFlag_ || (honorDisplayFlag_ && t->display())) {
    std::string key = t->column(0).toString().toLocal8Bit().constData();
    std::string val = t->column(1).toString().toLocal8Bit().constData();
    htmlEncode(key);
    htmlEncode(val);
    QColor kc = (t->color(0)).value<QColor>();
    QColor vc = (t->color(1)).value<QColor>();

    // host and process nodes
    if ((key == "Host" && !gvOpt_.ignoreHosts()) ||
      (key == "Process" && !gvOpt_.ignoreProcs()) ||
      (key == "DomainParticipant")) {

        if (key == "DomainParticipant" && gvOpt_.abbrGUIDs()) {
          val = abbr(val);
        }

        hasSubG = true;

        std::string id("\"cluster_");
        id += nodeName();
        id += "\"";
        fout << "subgraph " << id << " {" << std::endl;
        fout << "  label=\"" << key << ":" << val << "\";" << std::endl;
        lifo_.push("}\n");
    }
    else if ((key == "Publisher" && !gvOpt_.ignorePubs()) ||
      (key == "Subscriber" && !gvOpt_.ignoreSubs())) {
        hasSubG = true;

        std::string id("\"cluster_");
        id += nodeName();

        fout << "subgraph " << id << "\" {" << std::endl;
        fout << "  label=\"" << key <<  ":" << val << "\";" << std::endl;

        lifo_.push("}\n");
    }
    else if (key == "Topic" && !gvOpt_.hideTopics()) {
      // TODO: The Topic and Transport logic is ugly. It needs to be refactored
      //       Perhaps break this long if..else clause into seaprate methods
      //       for each type of node.

      // Topic is a node has GUID and TopicName
      stopDescent = true; // no need to go below a topic
      if (gvOpt_.abbrGUIDs()) {
        val = abbr(val);
      }

      // get the topicname
      std::string name;
      QColor nameColor;
      std::string dType;
      QColor dTypeColor;
      std::string tmp;
      TreeNode *c;
      TreeNode *q = 0; // qos
      for (int i = 0; i < t->size(); ++i) {
        c = t->operator[](i);
        tmp = c->column(0).toString().toLocal8Bit().constData();
        if (tmp == "Topic Name") {
          name = c->column(1).toString().toLocal8Bit().constData();
          htmlEncode(name);
          nameColor = c->color(1).value<QColor>();
        }
        else if (tmp == "Qos" && !gvOpt_.ignoreQos()) {
          q = c;
        }
        else if (tmp == "Data Type") {
          dType = c->column(1).toString().toLocal8Bit().constData();
          htmlEncode(dType);
          dTypeColor = c->color(1).value<QColor>();
        }
      }

      if (!gvOpt_.ignoreBuiltinTopics() || name.substr(0, 4) != "DCPS") {

        //fout << "   \"" << nodeName() << "\" [label=\"" << key << "|" << val << "|" << name << "|" << dType << "\"];" << std::endl;

        fout << "   \"" << nodeName() << "\" [shape=none, margin=0, label=";
        fout << "<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"3\">" << std::endl;
        fout << "<tr>" << std::endl;
        fout << "  <td bgcolor=\"" << qcolorToHex(kc) << "\">" << std::endl;
        fout << "    " << key << std::endl;
        fout << "  </td>" << std::endl;
        fout << "  <td bgcolor=\"" << qcolorToHex(vc) << "\">" << std::endl;
        fout << "    " << val << std::endl;
        fout << "  </td>" << std::endl;
        fout << "</tr> " << std::endl;
        if (name != "") {
          fout << "<tr>" << std::endl;
          fout << "  <td>Topic Name</td>" << std::endl;
          fout << "  <td bgcolor=\"" << qcolorToHex(nameColor) << "\">" << std::endl;
          fout << "    " << name << std::endl;
          fout << "  </td>" << std::endl;
          fout << "</tr>" << std::endl;
        }
        if (dType != "") {
          fout << "<tr>" << std::endl;
          fout << "  <td>Data Type</td>" << std::endl;
          fout << "  <td bgcolor=\"" << qcolorToHex(dTypeColor) << "\">" << std::endl;
          fout << "    " << dType << std::endl;
          fout << "  </td>" << std::endl;
          fout << "</tr> " << std::endl;
        }
      if (q) { // we have qos node
          for (int i = 0; i < q->size(); ++i) {
            c = q->operator[](i);
            if (c->width() >= 2) {
              key = c->column(0).toString().toLocal8Bit().constData();
              val = c->column(1).toString().toLocal8Bit().constData();
              htmlEncode(key);
              htmlEncode(val);
              kc = (c->color(0)).value<QColor>();
              vc = (c->color(1)).value<QColor>();

              fout << "<tr>" << std::endl;
              fout << "  <td bgcolor=\"" << qcolorToHex(kc) << "\">" << std::endl;
              fout << "    " << key << std::endl;
              fout << "  </td>" << std::endl;
              fout << "  <td bgcolor=\"" << qcolorToHex(vc) << "\">" << std::endl;
              fout << "    " << val << std::endl;
              fout << "  </td>" << std::endl;
              fout << "</tr> " << std::endl;
            }
          }
        }

        fout << "</table>>];" << std::endl;  // end table.

      }
    }
    else if ((key == "Transport" && !gvOpt_.ignoreTransports()) ||
        (key == "Qos" && !gvOpt_.ignoreQos())) {
      fout << "   \"" << nodeName() << "\" [shape=none, margin=0, label=";
      fout << "<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"3\">" << std::endl;
      fout << "<tr>" << std::endl;
      fout << "  <td bgcolor=\"" << qcolorToHex(kc) << "\">" << std::endl;
      fout << "    " << key << std::endl;
      fout << "  </td>" << std::endl;
      fout << "  <td bgcolor=\"" << qcolorToHex(vc) << "\">" << std::endl;
      fout << "    " << val << std::endl;
      fout << "  </td>" << std::endl;
      fout << "</tr> " << std::endl;

      // get and child data
      TreeNode *c;
      for (int i = 0; i < t->size(); ++i) {
        c = t->operator[](i);
        if (c->width() >= 2) {
          key = c->column(0).toString().toLocal8Bit().constData();
          val = c->column(1).toString().toLocal8Bit().constData();
          htmlEncode(key);
          htmlEncode(val);
          kc = (c->color(0)).value<QColor>();
          vc = (c->color(1)).value<QColor>();

          fout << "<tr>" << std::endl;
          fout << "  <td bgcolor=\"" << qcolorToHex(kc) << "\">" << std::endl;
          fout << "    " << key << std::endl;
          fout << "  </td>" << std::endl;
          fout << "  <td bgcolor=\"" << qcolorToHex(vc) << "\">" << std::endl;
          fout << "    " << val << std::endl;
          fout << "  </td>" << std::endl;
          fout << "</tr> " << std::endl;
        }
      }

      fout << "</table>>];" << std::endl;  // end the table.

      // we only traverse 1-level of children of transport & QoS nodes.
      // anything below we don't visit
      // TODO: visit ALL node children
      stopDescent = true;
    }
    else if (key == "Reader" || key == "Writer") {
      // Reader/Writer is a node that has GUID and TopicName
      stopDescent = true; // no need to go below a reader or writer
      if (gvOpt_.abbrGUIDs()) {
        val = abbr(val);
      }

      // get the topicname and
      std::string name;
      QColor nameColor;
      std::string conn;
      std::string connLabel;
      QColor connColor;
      std::string tmp;
      TreeNode *c;
      TreeNode *q = 0; // qos
      for (int i = 0; i < t->size(); ++i) {
        c = t->operator[](i);
        tmp = c->column(0).toString().toLocal8Bit().constData();
        if (tmp == "Topic Name") {
          name = c->column(1).toString().toLocal8Bit().constData();
          htmlEncode(name);
          nameColor = c->color(1).value<QColor>();
        }
        else if (tmp == "Qos" && !gvOpt_.ignoreQos()) {
          q = c;
        }
        else if (tmp == "Writer" || tmp == "Reader") {
          connLabel = tmp;
          htmlEncode(connLabel);
          if (gvOpt_.abbrGUIDs()) {
            conn = abbr(c->column(1).toString().toLocal8Bit().constData());
          }
          else {
            conn = c->column(1).toString().toLocal8Bit().constData();
            connColor = c->color(1).value<QColor>();
          }
          htmlEncode(conn);
        }
      }

      if (!gvOpt_.ignoreBuiltinTopics() || name.substr(0, 4) != "DCPS") {
        fout << "   \"" << nodeName() << "\" [shape=none, margin=0, label=";
        fout << "<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"3\">" << std::endl;
        fout << "<tr>" << std::endl;
        fout << "  <td bgcolor=\"" << qcolorToHex(kc) << "\">" << std::endl;
        fout << "    " << key << std::endl;
        fout << "  </td>" << std::endl;
        fout << "  <td bgcolor=\"" << qcolorToHex(vc) << "\">" << std::endl;
        fout << "    " << val << std::endl;
        fout << "  </td>" << std::endl;
        fout << "</tr> " << std::endl;
        if (name != "") {
          fout << "<tr>" << std::endl;
          fout << "  <td>Topic Name</td>" << std::endl;
          fout << "  <td bgcolor=\"" << qcolorToHex(nameColor) << "\">" << std::endl;
          fout << "    " << name << std::endl;
          fout << "  </td>" << std::endl;
          fout << "</tr>" << std::endl;
        }
        if (conn != "") {
          fout << "<tr>" << std::endl;
          if (connLabel != "") {
            fout << "<td>" << connLabel << "</td>" << std::endl;
          }
          fout << "  <td bgcolor=\"" << qcolorToHex(connColor) << "\">" << std::endl;
          fout << "    " << conn << std::endl;
          fout << "  </td>" << std::endl;
          fout << "</tr> " << std::endl;
        }
        if (q) { // we have qos node
          for (int i = 0; i < q->size(); ++i) {
            c = q->operator[](i);
            if (c->width() >= 2) {
              key = c->column(0).toString().toLocal8Bit().constData();
              val = c->column(1).toString().toLocal8Bit().constData();
              htmlEncode(key);
              htmlEncode(val);
              kc = (c->color(0)).value<QColor>();
              vc = (c->color(1)).value<QColor>();

              fout << "<tr>" << std::endl;
              fout << "  <td bgcolor=\"" << qcolorToHex(kc) << "\">" << std::endl;
              fout << "    " << key << std::endl;
              fout << "  </td>" << std::endl;
              fout << "  <td bgcolor=\"" << qcolorToHex(vc) << "\">" << std::endl;
              fout << "    " << val << std::endl;
              fout << "  </td>" << std::endl;
              fout << "</tr> " << std::endl;
            }
          }
        }

        fout << "</table>>];" << std::endl;  // end the table.

        // store this writer-->reader connection info
        if (!conn.empty()) {
          nMap_[val] = nodeName(-1);
          if (key == "Reader") {
            rMap_[val] = conn;
          }
          else {
            wMap_[val] = conn;
          }
        }
      }
    }
  }
  }

  // print the children
  if (!stopDescent) {
    for (int i = 0; i < t->size(); ++i) {
      generate(t->operator[](i));
    }
  }

  // close the subgraphs
  if (hasSubG) {
    fout << lifo_.top();
    lifo_.pop();
  }

}

std::string
Monitor::GraphGenerator::qcolorToHex(const QColor& c)
{
  std::string s = "#";
  char b[3]; // 2 digits and \0
  memset (b, 0, 3);
  sprintf (b, "%02x", c.red());
  s += b;
  sprintf (b, "%02x", c.green());
  s += b;
  sprintf (b, "%02x", c.blue());
  s += b;

  // invalid color or if black convert to white.
  if (s == "#000000") {
    s = "#FFFFFF";
  }

  return s;

}

void
Monitor::GraphGenerator::htmlEncode(std::string &s) {
  char c[] = "\"<>&";

  for (unsigned int i = 0; i < strlen (c); ++i) {
    size_t pos = s.find(c[i]);

    while ( pos != std::string::npos )
    {
      s.replace( pos, 1, " " );
      pos = s.find(c[i], pos + 1 );
    }
  }
}

std::string
Monitor::GraphGenerator::abbr(const std::string &s)
{
  std::string ret = s;

  const char *t = s.c_str();
  while (*t++) {
    if (*t == '(') {
      ret = t;
        break;
      }
  }

  return ret;
}

std::string
Monitor::GraphGenerator::nodeName(int offset)
{
  static long int num = 0;
  std::stringstream s;
  s << "node_";

  if (offset == 0) {
    s << num++;
  }
  else {
    s << num + offset;
  }

  return s.str();
}

