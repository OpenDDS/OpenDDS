---
layout: page
title: Quick Start
order: 10
categories: [opendds]
---
- [Quick Start Guides in the DevGuide](https://opendds.readthedocs.io/en/latest-release/devguide/quickstart/index.html)
{% assign sorted_pages = site.pages | sort:"order" %}
{% assign thisdir = page.url | split: '/' %}
{% assign dir = thisdir[1] %}
{% for p in sorted_pages %}
  {% assign splt = p.url | split: dir %}
  {% if splt.size == 2 and splt[1] != '/' %}
- <a class="page-link" href="{{p.url | prepend: site.baseurl}}">{{p.title}}</a>
  {% endif %}
{% endfor %}
- [Detailed build instructions](../documentation/building.html)
