---
layout: page
title: OpenDDS
categories: [opendds]
---
{% assign sorted_pages = site.pages | sort: "order" %}
{% for p in sorted_pages %}
  {% if p.url != '/index.html' and p.url != '/css/main.css' %}
    {% assign slash = p.url | split: '/' %}
    {% if slash.size == 2 or slash[2] == 'index.html' %}
- <a class="page-link" href="{{p.url | prepend: site.baseurl}}">{{p.title}}</a>
    {% else %}
   - <a class="page-link" href="{{p.url | prepend: site.baseurl}}">{{p.title}}</a>
    {% endif %}
  {% endif %}
{% endfor %}
{% for r in site.github.releases %}

### Release {{r.name}}

{{r.body}}
{% endfor %}
