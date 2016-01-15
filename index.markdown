---
layout: page
categories: [opendds]
---

OpenDDS is an open source C++ implementation of the Object Management Group (OMG) Data Distribution Service (DDS). OpenDDS also supports Java bindings through JNI. OpenDDS is supported by [OCI](www.ociweb.com) and the source code is hosted on [GitHub]({{site.github.repository_url}}).

{% assign sorted_rel = site.github.releases | sort: "created_at" | reverse %}
{% assign latest_rel = sorted_rel | first %}
### Current Release: [{{latest_rel.name}}]({{latest_rel.html_url}})

{{latest_rel.body}}

The [NEWS](http://raw.githubusercontent.com/objectcomputing/OpenDDS/{{latest_rel.tag_name}}/NEWS) file contains release notes for all previous releases.

{% if site.github.releases.size > 1 %}
### Previous releases

{% for r in sorted_rel limit:5 offset:1 %}
- [{{r.name}}]({{r.html_url}})
{% endfor %}
{% if site.github.releases.size > 6 %}
- [See all on GitHub]({{site.github.releases_url}})
{% endif %}
{% endif %}
