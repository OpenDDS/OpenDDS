---
layout: page
categories: [opendds]
---

OpenDDS is an open source C++ implementation of the Object Management Group (OMG) Data Distribution Service (DDS). Java applications can use OpenDDS through JNI bindings. OpenDDS was developed and open sourced by [OCI](http://www.objectcomputing.com). We welcome community contributions to the project. Commercial support is also available; see the [support page](support.html) for more information.

### Online Training
OCI is offering the following live online OpenDDS training classes.  Each class consists of two 3-hour sessions.
- February 26-27, 2019: [Intro to OpenDDS Programming with C++](https://objectcomputing.com/training/catalog/middleware/introduction-to-opendds-with-cpp)
- February 28 - March 1, 2019: [Intro to OpenDDS Programming with Java](https://objectcomputing.com/training/catalog/middleware/introduction-to-opendds-with-java)

{% assign sorted_rel = site.github.releases | sort: "created_at" | reverse %}
{% assign latest_rel = sorted_rel | first %}
### Current Release: [{{latest_rel.name}}]({{latest_rel.html_url}})

{{latest_rel.body}}
{% assign assets = latest_rel.assets | sort: "name" | reverse %}
{% for a in assets %}{% capture tn %}Open{{latest_rel.tag_name}}{% endcapture %}
{% assign ext = a.name | remove: tn %}
{% if ext == '.zip' %}
  {% assign note = 'Windows' %}
{% else %}
  {% assign note = 'Linux/Solaris/MacOSX' %}
{% endif %}
[Download {{a.name}}]({{a.browser_download_url}}) ({{note}})
{% endfor %}

{% if site.github.releases.size > 1 %}
### Previous Releases:

{% for r in sorted_rel limit:5 offset:1 %}
- [{{r.name}}]({{r.html_url}})
{% endfor %}
{% if site.github.releases.size > 6 %}
- [See all releases on GitHub]({{site.github.releases_url}})
{% endif %}
The [NEWS](http://raw.githubusercontent.com/objectcomputing/OpenDDS/{{latest_rel.tag_name}}/NEWS.md) file contains release notes for all previous releases.
{% endif %}
