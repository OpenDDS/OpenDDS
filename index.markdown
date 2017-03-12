---
layout: page
categories: [opendds]
---

OpenDDS is an open source C++ implementation of the Object Management Group (OMG) Data Distribution Service (DDS). Java applications can use OpenDDS through JNI bindings. OpenDDS is supported by [OCI](http://www.objectcomputing.com) and the source code is hosted on [GitHub]({{site.github.repository_url}}).

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
The [NEWS](http://raw.githubusercontent.com/objectcomputing/OpenDDS/{{latest_rel.tag_name}}/NEWS) file contains release notes for all previous releases.
{% endif %}
