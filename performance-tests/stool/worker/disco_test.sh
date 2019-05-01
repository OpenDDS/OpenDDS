#!/bin/bash

num_values=(1 2 4 8)
bool_values=(false true)

for parts in "${num_values[@]}"
do
  for subs in "${num_values[@]}"
  do
    for pubs in "${num_values[@]}"
    do
      for reads in "${num_values[@]}"
      do
        for writes in "${num_values[@]}"
        do
          for part_ae in "${bool_values[@]}"
          do
            for rel_read in "${bool_values[@]}"
            do
              sed -i "s/ips_per_proc_max = \([0-9]*\)/ips_per_proc_max = ${parts}/g" main.cpp
              sed -i "s/subs_per_ip_max = \([0-9]*\)/subs_per_ip_max = ${subs}/g" main.cpp
              sed -i "s/pubs_per_ip_max = \([0-9]*\)/pubs_per_ip_max = ${pubs}/g" main.cpp
              sed -i "s/drs_per_sub_max = \([0-9]*\)/drs_per_sub_max = ${reads}/g" main.cpp
              sed -i "s/dws_per_pub_max = \([0-9]*\)/dws_per_pub_max = ${writes}/g" main.cpp
              sed -i "s/config\.participants\[ip\]\.mask\.entity_factory\.has_autoenable_created_entities = \([a-z]*\)/config\.participants\[ip\]\.mask\.entity_factory\.has_autoenable_created_entities = ${part_ae}/g" main.cpp
              sed -i "s/config\.participants\[ip\]\.subscribers\[sub\]\.datareaders\[dr\]\.mask\.reliability\.has_kind = \([a-z]*\)/config\.participants\[ip\]\.subscribers\[sub\]\.datareaders\[dr\]\.mask\.reliability\.has_kind = ${rel_read}/g" main.cpp

              make
              ./worker empty.cfg | tail -n 6 | tee results_${parts}_${subs}_${pubs}_${reads}_${writes}_${part_ae}_${rel_read}.txt

            done
          done
        done
      done
    done
  done
done
