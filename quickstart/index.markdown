---
layout: page
title: Quick Start
order: 10
categories: [opendds]
---
{% assign sorted_pages = site.pages | sort:"order" %}
{% assign thisdir = page.url | split: '/' %}
{% assign dir = thisdir[1] %}
{% for p in sorted_pages %}
  {% assign splt = p.url | split: dir %}
  {% if splt.size == 2 and splt[1] != '/' %}
- <a class="page-link" href="{{p.url | prepend: site.baseurl}}">{{p.title}}</a>
  {% endif %}
{% endfor %}
- [Detailed build instructions](../documents/building.html)
