#include <cstdlib>
#include <cstring>
#include <cassert>

#include <iostream>
#include <sstream>
#include <string>

#include <algorithm>

#include <unistd.h>

#include "intrusive_set.h"
#include "intrusive_heap.h"
#include "intrusive_table.h"

struct vertex_t;

struct edge_t {
  edge_t(vertex_t* f, vertex_t* t, unsigned w) : from(f), to(t), cost(w) { }

  vertex_t* from;
  vertex_t* to;
  unsigned cost;

  intrusive_heap_link<edge_t> link;
  typedef intrusive_heap<edge_t, &edge_t::link, typeof(edge_t::cost),
                         &edge_t::cost> heap_t;

  bool
  bound() const {
    return false
        || link.bound()
        ;;
  }

  void kill() { if (!bound()) delete this; }
};

struct vertex_t {
  vertex_t(const char * c) : id(strdup(c)) { }
  ~vertex_t() { free(const_cast<char*>(id)); }

  const char* id;

  intrusive_table_link<vertex_t> table_link;
  typedef intrusive_table<vertex_t, &vertex_t::table_link,
                          typeof(vertex_t::id), &vertex_t::id> table_t;

  intrusive_set_link<vertex_t> set_link;
  typedef intrusive_set<vertex_t, &vertex_t::set_link> set_t;

  bool typed() const { return set_t::typed(this); }
  set_t* archetype() const { return set_t::archetype(this); }

  bool
  bound() const {
    return false
        || table_link.bound()
        || set_link.bound()
        ;;
  }

  void kill() { if (!bound()) delete this; }
};

void
expand(vertex_t::table_t & vs, unsigned size) {
  delete [] vs.rehash(new vertex_t::table_t::bucket_t[size], size);
}

int
main(int, char* argv[]) {
  const unsigned load = 2;
  unsigned n_vertices = 0;

  edge_t::heap_t edges;
  vertex_t::table_t vertices;

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty() || '#' == line[0])
      continue;

    std::stringstream ss(line);
    std::string from, to;
    unsigned cost;
    ss >> from >> to >> cost;

    vertex_t* f = vertices.get(from.c_str());
    if (!f) {
      ++n_vertices;
      if (vertices.buckets() < load * n_vertices)
        expand(vertices, (load+1) * (n_vertices+1));
      vertices.set(f = new vertex_t(from.c_str()));
    }

    vertex_t* t = vertices.get(to.c_str());
    if (!t) {
      ++n_vertices;
      if (vertices.buckets() < load * n_vertices)
        expand(vertices, (load+1) * (n_vertices+1));
      vertices.set(t = new vertex_t(to.c_str()));
    }

    edges.inhume(new edge_t(f, t, cost));
  }

  while (!edges.empty()) {
    edge_t* e = edges.exhume();
    if (!e->from->typed() && !e->to->typed()) {
      vertex_t::set_t* s = new vertex_t::set_t;
      s->join(e->from);
      s->join(e->to);
    } else if (!e->from->typed()) {
      vertex_t::set_t::archetype(e->to)->join(e->from);
    } else if (!e->to->typed()) {
      vertex_t::set_t::archetype(e->from)->join(e->to);
    } else {
      vertex_t::set_t* s = vertex_t::set_t::archetype(e->from);
      if (s->contains(e->to))
        continue;
      delete s->conjoin(*vertex_t::set_t::archetype(e->to));
    }

    std::cout
      << e->from->id << " "
      << e->to->id << " "
      << e->cost << std::endl;
  }

  vertex_t* v = vertices.iterator();
  while (v) {
    if (v->typed())
      delete &v->archetype()->dissolve();
    vertex_t* x = vertices.next(v);
    std::swap(x, v);
    vertices.bus(x)->kill();
  }

  delete [] vertices.dehash();
  return EXIT_SUCCESS;
}

//