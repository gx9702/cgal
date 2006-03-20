// Copyright (c) 1997-2000  Max-Planck-Institute Saarbruecken (Germany).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// 
//
// Author(s)     : Miguel Granados <granados@mpi-sb.mpg.de>

#ifndef SNC_POINT_LOCATOR_H
#define SNC_POINT_LOCATOR_H

#include <CGAL/basic.h>
#include <CGAL/Nef_3/SNC_intersection.h>
#ifdef CGAL_NEF3_POINT_LOCATOR_NAIVE
#include <CGAL/Nef_3/SNC_ray_shooter.h>
#endif
#include <CGAL/Nef_3/SNC_k3_tree_traits.h>
#include <CGAL/Nef_3/K3_tree.h>
#include <CGAL/Unique_hash_map.h>
#include <CGAL/Timer.h>

#ifdef CGAL_NEF3_TRIANGULATE_FACETS
#include <CGAL/Constrained_triangulation_2.h>
#include <CGAL/Triangulation_data_structure_2.h>
#include <CGAL/Triangulation_euclidean_traits_xy_3.h>
#include <CGAL/Triangulation_euclidean_traits_yz_3.h>
#include <CGAL/Triangulation_euclidean_traits_xz_3.h>
#include <CGAL/Constrained_triangulation_face_base_2.h>
#endif

#undef CGAL_NEF_DEBUG
#define CGAL_NEF_DEBUG 509
#include <CGAL/Nef_2/debug.h>

#undef _CGAL_NEF_TRACEN
#define _CGAL_NEF_TRACEN(msg) CGAL_NEF_TRACEN( "SNC_point_locator: " << msg);

// TODO: find out the proper CGAL replacement for this macro and remove it
#define CGAL_for_each( i, C) for( i = C.begin(); i != C.end(); ++i)

// #define TIMER(instruction) instruction
#define TIMER(instruction)

// #define CLOG(t) std::clog <<" "<<t<<std::endl; std::clog.flush()
#define CLOG(t)

CGAL_BEGIN_NAMESPACE

template <typename SNC_decorator>
class SNC_point_locator
{
 public:
  class Intersection_call_back;
  typedef SNC_point_locator<SNC_decorator> Self;
  typedef typename SNC_decorator::Decorator_traits Decorator_traits;
  typedef typename SNC_decorator::SNC_structure SNC_structure;
protected:
  char version_[64];
  // time for construction, point location, ray shooting and intersection test
  mutable Timer ct_t, pl_t, rs_t, it_t; 

public: 
  typedef typename SNC_structure::Object_handle Object_handle;

  typedef typename SNC_structure::Point_3 Point_3;
  typedef typename SNC_structure::Segment_3 Segment_3;
  typedef typename SNC_structure::Ray_3 Ray_3;
  typedef typename SNC_structure::Vector_3 Vector_3;
  typedef typename SNC_structure::Aff_transformation_3 
                                  Aff_transformation_3;

  typedef typename Decorator_traits::Vertex_handle Vertex_handle;
  typedef typename Decorator_traits::Halfedge_handle Halfedge_handle;
  typedef typename Decorator_traits::Halffacet_handle Halffacet_handle;
  typedef typename Decorator_traits::Volume_handle Volume_handle;
  typedef typename Decorator_traits::Vertex_iterator Vertex_iterator;
  typedef typename Decorator_traits::Halfedge_iterator Halfedge_iterator;
  typedef typename Decorator_traits::Halffacet_iterator Halffacet_iterator;

  const char* version() { return version_; }

  virtual Object_handle locate(const Point_3& p) const = 0;

  virtual Object_handle shoot(const Ray_3& s, int mask=255) const = 0;

  virtual void intersect_with_edges( Halfedge_handle edge,
                                     const Intersection_call_back& call_back) 
    const = 0;

  virtual void intersect_with_facets( Halfedge_handle edge,
                                      const Intersection_call_back& call_back)
    const = 0;

  virtual void intersect_with_edges_and_facets( Halfedge_handle edge,
	const Intersection_call_back& call_back) const = 0;

  class Intersection_call_back 
  {
  public:
    virtual void operator()( Halfedge_handle edge, Object_handle object, 
                             const Point_3& intersection_point) const = 0;
    
    virtual ~Intersection_call_back() {}
  };

  virtual void initialize(SNC_structure* W) = 0;

  virtual Self* clone() const = 0;

  virtual void transform(const Aff_transformation_3& t) = 0;

  //virtual bool update( Unique_hash_map<Vertex_handle, bool>& V, 
  //                   Unique_hash_map<Halfedge_handle, bool>& E, 
  //                   Unique_hash_map<Halffacet_handle, bool>& F) = 0;

  virtual void add_facet(Halffacet_handle f) {}

  virtual void add_edge(Halfedge_handle e) {}

  virtual void add_vertex(Vertex_handle v) {}

  virtual ~SNC_point_locator() {
    CLOG("");
    CLOG("construction_time:  "<<ct_t.time());
    CLOG("pointlocation_time: "<<pl_t.time());
    CLOG("rayshooting_time:   "<<rs_t.time());
    CLOG("intersection_time:  "<<it_t.time());
    // warning: the total time showed here could be actually larger
    // that the real time used by this class, since point location
    // and intersection test use the ray shooter and so the same time 
    // could be account to more than one timer
    CLOG("pltotal_time:       "<<
      ct_t.time()+pl_t.time()+rs_t.time()+it_t.time());
  };
};

template <typename SNC_decorator>
class SNC_point_locator_by_spatial_subdivision : 
  public SNC_point_locator<SNC_decorator>,
  public SNC_decorator
{ 
  typedef SNC_decorator Base;
  typedef CGAL::SNC_point_locator<SNC_decorator> SNC_point_locator;
  typedef CGAL::SNC_point_locator_by_spatial_subdivision<SNC_decorator> Self;
  typedef typename SNC_decorator::SNC_structure SNC_structure;
  typedef typename SNC_decorator::Decorator_traits Decorator_traits;
  typedef typename Decorator_traits::SM_decorator SM_decorator;
  typedef CGAL::SNC_intersection<SNC_structure> SNC_intersection;
  typedef typename SNC_decorator::SM_point_locator SM_point_locator;
  typedef typename SNC_decorator::Kernel Kernel;

public:
  typedef typename CGAL::SNC_k3_tree_traits<SNC_decorator> K3_tree_traits;
  typedef typename CGAL::K3_tree<K3_tree_traits> K3_tree;
  typedef K3_tree SNC_candidate_provider;
  
  typedef typename SNC_structure::Object_handle Object_handle;
 #ifdef CGAL_NEF3_TRIANGULATE_FACETS
  typedef typename Decorator_traits::Halffacet_triangle_handle 
                                     Halffacet_triangle_handle;
#endif
#ifdef CGAL_NEF3_FACET_WITH_BOX
  typedef typename SNC_structure::Partial_facet Partial_facet;
#endif
  typedef typename SNC_structure::Point_3 Point_3;
  typedef typename SNC_structure::Segment_3 Segment_3;
  typedef typename SNC_structure::Ray_3 Ray_3;
  typedef typename SNC_structure::Vector_3 Vector_3;
  typedef typename SNC_structure::Triangle_3 Triangle_3;
  typedef typename SNC_structure::Aff_transformation_3 
                                  Aff_transformation_3;

  typedef typename SNC_structure::Infi_box Infi_box;

  typedef typename Decorator_traits::Vertex_handle Vertex_handle;
  typedef typename Decorator_traits::Halfedge_handle Halfedge_handle;
  typedef typename Decorator_traits::Halffacet_handle Halffacet_handle;
  typedef typename Decorator_traits::SHalfedge_handle SHalfedge_handle;
  typedef typename Decorator_traits::SFace_handle SFace_handle;
  typedef typename Decorator_traits::Vertex_iterator Vertex_iterator;
  typedef typename Decorator_traits::Halfedge_iterator Halfedge_iterator;
  typedef typename Decorator_traits::Halffacet_iterator Halffacet_iterator;
  typedef typename Decorator_traits::Volume_handle Volume_handle;

  typedef typename Decorator_traits::Halffacet_cycle_iterator 
                                     Halffacet_cycle_iterator;
  typedef typename Decorator_traits::SHalfedge_around_facet_circulator 
	                             SHalfedge_around_facet_circulator;

  typedef typename SNC_candidate_provider::Object_list Object_list;
  typedef typename Object_list::iterator Object_list_iterator;
  typedef typename SNC_candidate_provider::Objects_along_ray Objects_along_ray;
  typedef typename Objects_along_ray::Iterator Objects_along_ray_iterator;

public:
  SNC_point_locator_by_spatial_subdivision() : 
    initialized(false), candidate_provider(0) {}

#ifdef CGAL_NEF3_TRIANGULATE_FACETS	
  template<typename Kernel>
  class Triangulation_handler {

    typedef typename CGAL::Triangulation_vertex_base_2<Kernel>               Vb;
    typedef typename CGAL::Constrained_triangulation_face_base_2<Kernel>     Fb;
    typedef typename CGAL::Triangulation_data_structure_2<Vb,Fb>             TDS;
    typedef typename CGAL::No_intersection_tag                               Itag;
    typedef typename CGAL::Constrained_triangulation_2<Kernel,TDS,Itag>      CT;

    typedef typename CT::Face_handle           Face_handle;
    typedef typename CT::Finite_faces_iterator Finite_face_iterator;
    typedef typename CT::Edge                  Edge;

    CT ct;
    CGAL::Unique_hash_map<Face_handle, bool> visited;
    Finite_face_iterator fi;

  public:
    template<typename Halffacet_handle>
    Triangulation_handler(Halffacet_handle f) : visited(false) {

      Halffacet_cycle_iterator fci;
      for(fci=f->facet_cycles_begin(); fci!=f->facet_cycles_end(); ++fci) {
	if(fci.is_shalfedge()) {
          SHalfedge_around_facet_circulator sfc(fci), send(sfc);
	  CGAL_For_all(sfc,send) {
            CGAL_NEF_TRACEN("  insert constraint" << sfc->source()->source()->point()
	                     << "->" << sfc->source()->twin()->source()->point());
	    ct.insert_constraint(sfc->source()->source()->point(),
	                         sfc->source()->twin()->source()->point());
          }
        }
      }
      CGAL_assertion(ct.is_valid());

      CGAL_NEF_TRACEN("number of finite triangles " << ct.number_of_faces());

      typename CT::Face_handle infinite = ct.infinite_face();
      typename CT::Vertex_handle ctv = infinite->vertex(1);
      if(ct.is_infinite(ctv)) ctv = infinite->vertex(2);
      CGAL_assertion(!ct.is_infinite(ctv));

      typename CT::Face_handle opposite;
      typename CT::Face_circulator vc(ctv,infinite);
      do { opposite = vc++;
      } while(!ct.is_constrained(CT::Edge(vc,vc->index(opposite))));
      typename CT::Face_handle first = vc;

      CGAL_assertion(!ct.is_infinite(first));
      traverse_triangulation(first, first->index(opposite));

      /*
      for(fi = ct.finite_faces_begin(); fi != ct.finite_faces_end(); ++fi)
        CGAL_NEF_TRACEN("  finite face " 
	  << Triangle_3(fi->vertex(0)->point(), fi->vertex(1)->point(), fi->vertex(2)->point())
	   << "was visited " << visited[fi]);
      */

      fi = ct.finite_faces_begin();
    }

    void traverse_triangulation(Face_handle f, int parent) {
      visited[f] = true;
      if(!ct.is_constrained(Edge(f,ct.cw(parent))) && !visited[f->neighbor(ct.cw(parent))]) {
	Face_handle child(f->neighbor(ct.cw(parent)));
	traverse_triangulation(child, child->index(f));
      } 
      if(!ct.is_constrained(Edge(f,ct.ccw(parent))) && !visited[f->neighbor(ct.ccw(parent))]) {
	Face_handle child(f->neighbor(ct.ccw(parent)));
	traverse_triangulation(child, child->index(f));
      } 
    } 
 
    template<typename Triangle_3>
    bool get_next_triangle(Triangle_3& tr) {
      while(fi != ct.finite_faces_end() && visited[fi] == false) ++fi;
      if(fi == ct.finite_faces_end()) return false;
      tr = Triangle_3(fi->vertex(0)->point(), fi->vertex(1)->point(), fi->vertex(2)->point());
      ++fi;
      return true;
    }
  };
#endif

  virtual void initialize(SNC_structure* W) {
#ifdef CGAL_NEF_LIST_OF_TRIANGLES
    initialized = true;
    set_snc(*W);
    candidate_provider = new SNC_candidate_provider(W);
#else // CGAL_NEF_LIST_OF_TRIANGLES
    TIMER(ct_t.start());
    strcpy( this->version_, "Point Locator by Spatial Subdivision (tm)");
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
    CLOG(version()<<" (with triangulated facets)");
#else
    CLOG(version());
#endif
    CGAL_assertion( W != NULL);
//    (Base) *this = SNC_decorator(*W);
	set_snc(*W);
    initialized = true;
    Object_list objects;
    Vertex_iterator v;
    Halfedge_iterator e;
    Halffacet_iterator f;
    CGAL_forall_vertices( v, *this->sncp())
      objects.push_back(Object_handle(Vertex_handle(v)));
    typename Object_list::difference_type v_end = objects.size();
    CGAL_forall_edges( e, *this->sncp())
      objects.push_back(Object_handle(Halfedge_handle(e)));
    CGAL_forall_facets( f, *this->sncp()) {
#ifdef CGAL_NEF3_TRIANGULATE_FACETS

#ifndef CGAL_NEF3_TRIANGULATION_MINIMUM
#define CGAL_NEF3_TRIANGULATION_MINIMUM 25
#endif

      Halffacet_cycle_iterator fci = f->facet_cycles_begin();
      CGAL_assertion(fci.is_shalfedge());
      SHalfedge_around_facet_circulator safc(fci), send(safc);
      int length = 0;
      int stop = CGAL_NEF3_TRIANGULATION_MINIMUM;
      while(++length < stop && ++safc != send);
      if(length >= stop) {

	CGAL_NEF_TRACEN("triangulate facet " << f->plane());
      
	typedef typename CGAL::Triangulation_euclidean_traits_xy_3<Kernel>       XY;
	typedef typename CGAL::Triangulation_euclidean_traits_yz_3<Kernel>       YZ;
	typedef typename CGAL::Triangulation_euclidean_traits_xz_3<Kernel>       XZ;
	
	Triangle_3 tr;
	
	Vector_3 orth = f->plane().orthogonal_vector();
	int c = CGAL::abs(orth[0]) > CGAL::abs(orth[1]) ? 0 : 1;
	c = CGAL::abs(orth[2]) > CGAL::abs(orth[c]) ? 2 : c;
	
	if(c == 0) {
	  Triangulation_handler<YZ> th(f);
	  while(th.get_next_triangle(tr)) {
	    Halffacet_triangle_handle th( f, tr);
	    objects.push_back(Object_handle(th));
	    CGAL_NEF_TRACEN("add triangle " << tr);
	  }
	} else if(c == 1) {
	  Triangulation_handler<XZ> th(f);
	  while(th.get_next_triangle(tr)) {
	    Halffacet_triangle_handle th( f, tr);
	    objects.push_back(Object_handle(th));
	    CGAL_NEF_TRACEN("add triangle " << tr);
	  }
	} else if(c == 2) {
	  Triangulation_handler<XY> th(f);
	  while(th.get_next_triangle(tr)) {
	    Halffacet_triangle_handle th( f, tr);
	    objects.push_back(Object_handle(th));
	    CGAL_NEF_TRACEN("add triangle " << tr);
	  }
	} else
	  CGAL_assertion_msg(false, "wrong value");
      } else {
        CGAL_NEF_TRACEN("add facet " << f->plane());
        objects.push_back(Object_handle(Halffacet_handle(f)));
      }
#elif defined CGAL_NEF3_FACET_WITH_BOX
#ifndef CGAL_NEF3_PARTITION_MINIMUM
#define CGAL_NEF3_PARTITION_MINIMUM 6
#endif
      Halffacet_cycle_iterator fci = f->facet_cycles_begin();
      CGAL_assertion(fci.is_shalfedge());
      SHalfedge_around_facet_circulator safc(fci), send(safc);
      int length = 0;
      int stop = CGAL_NEF3_PARTITION_MINIMUM;
      while(++length < stop && ++safc != send);
      if(length >= stop) {
	CGAL_NEF_TRACEN("use Partial facets ");
	Partial_facet pf(f);
	objects.push_back(Object_handle(pf));
      } else {
        objects.push_back(Object_handle(Halffacet_handle(f)));
      }
#else
      objects.push_back(Object_handle(Halffacet_handle(f)));
#endif
    }
    Object_list_iterator oli=objects.begin()+v_end;
    candidate_provider = new SNC_candidate_provider(objects,oli);
    // CGAL_NEF_TRACEN(*candidate_provider);
    TIMER(ct_t.stop());
#endif // CGAL_NEF_LIST_OF_TRIANGLES
  }

  virtual Self* clone() const { 
    return new Self; 
  }

  virtual void transform(const Aff_transformation_3& t) {
    candidate_provider->transform(t);
  }

  virtual bool update( Unique_hash_map<Vertex_handle, bool>& V, 
                       Unique_hash_map<Halfedge_handle, bool>& E, 
                       Unique_hash_map<Halffacet_handle, bool>& F) {
    TIMER(ct_t.start());
    CGAL_assertion( initialized);
    bool updated = candidate_provider->update( V, E, F);
    TIMER(ct_t.stop());
    return updated;
  }

  virtual ~SNC_point_locator_by_spatial_subdivision() {
    CGAL_warning( initialized); // required?
    delete candidate_provider;
  }

  virtual Object_handle shoot(const Ray_3& ray, int mask=255) const {
    TIMER(rs_t.start());
    CGAL_assertion( initialized);
    _CGAL_NEF_TRACEN( "shooting: "<<ray);
    Object_handle result;
    Vertex_handle v;
    Halfedge_handle e;
    Halffacet_handle f;
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
    Halffacet_triangle_handle t;
#endif
#ifdef CGAL_NEF3_FACET_WITH_BOX
    Partial_facet pf;
#endif
    bool hit = false;
    Point_3 eor; // 'end of ray', the latest ray's hit point
    Objects_along_ray objects = candidate_provider->objects_along_ray(ray);
    Objects_along_ray_iterator objects_iterator = objects.begin();
    while( !hit && objects_iterator != objects.end()) {
      Object_list candidates = *objects_iterator;
      Object_list_iterator o;
      CGAL_for_each( o, candidates) {
        if( CGAL::assign( v, *o) && ((mask&1) != 0)) {
          _CGAL_NEF_TRACEN("trying vertex on "<<v->point());
          if( ray.source() != v->point() && ray.has_on(v->point())) {
            _CGAL_NEF_TRACEN("the ray intersects the vertex");
            _CGAL_NEF_TRACEN("prev. intersection? "<<hit);
            if( hit) _CGAL_NEF_TRACEN("prev. intersection on "<<eor);
            if( hit && !Segment_3( ray.source(), eor).has_on(v->point()))
              continue;
            eor = v->point();
            result = Object_handle(v);
            hit = true;
            _CGAL_NEF_TRACEN("the vertex becomes the new hit object");
          }
        }
        else if( CGAL::assign( e, *o) && ((mask&2) != 0)) {
          Point_3 q;
          _CGAL_NEF_TRACEN("trying edge on "<< Segment_3(e->source()->point(),e->twin()->source()->point()));
          if( is.does_intersect_internally( ray, Segment_3(e->source()->point(),
                                                           e->twin()->source()->point()), q)) {
            _CGAL_NEF_TRACEN("ray intersects edge on "<<q);
            _CGAL_NEF_TRACEN("prev. intersection? "<<hit);
            if( hit) _CGAL_NEF_TRACEN("prev. intersection on "<<eor);
            if( hit && !has_smaller_distance_to_point( ray.source(), q, eor))
              continue;
            _CGAL_NEF_TRACEN("is the intersection point on the current cell? "<<
                    candidate_provider->is_point_on_cell( q, objects_iterator));
            if( !candidate_provider->is_point_on_cell( q, objects_iterator))
                continue;
            eor = q; 
            result = Object_handle(e);
            hit = true;
            _CGAL_NEF_TRACEN("the edge becomes the new hit object");
          }
        }
        else if( CGAL::assign( f, *o) && ((mask&4) != 0)) {
          Point_3 q;
          _CGAL_NEF_TRACEN("trying facet with on plane "<<f->plane()<<
                  " with point on "<<f->plane().point());
          if( is.does_intersect_internally( ray, f, q) ) {
            _CGAL_NEF_TRACEN("ray intersects facet on "<<q);
            _CGAL_NEF_TRACEN("prev. intersection? "<<hit);
            if( hit) _CGAL_NEF_TRACEN("prev. intersection on "<<eor);
            if( hit && !has_smaller_distance_to_point( ray.source(), q, eor))
	      continue;
            _CGAL_NEF_TRACEN("is the intersection point on the current cell? "<<
                    candidate_provider->is_point_on_cell( q, objects_iterator));
            if( !candidate_provider->is_point_on_cell( q, objects_iterator))
	      continue;
            eor = q;
            result = Object_handle(f);
            hit = true; 
            _CGAL_NEF_TRACEN("the facet becomes the new hit object");
          }
        }
#ifdef CGAL_NEF3_FACET_WITH_BOX
	else if( CGAL::assign(pf, *o) && ((mask&4) != 0)) {
	  CGAL_NEF_TRACEN("new ray shooting");
          Point_3 q;
          if( is.does_intersect_internally( ray, pf, q) ) {
            if( hit && !has_smaller_distance_to_point( ray.source(), q, eor))
	      continue;
            if( !candidate_provider->is_point_on_cell( q, objects_iterator))
	      continue;
            eor = q;
            result = Object_handle(pf.f);
            hit = true; 
          }
	}
#endif
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
        else if( CGAL::assign( t, *o) && ((mask&8) != 0)) {
          Point_3 q;
          Triangle_3 tr = t.get_triangle();
          _CGAL_NEF_TRACEN("trying triangle "<<tr);
          if( is.does_intersect( ray, tr, q) && normalized(q) != ray.source()) {
            _CGAL_NEF_TRACEN("ray intersect triangle on "<< normalized(q));
            _CGAL_NEF_TRACEN("prev. intersection? "<<hit);
            if( hit) _CGAL_NEF_TRACEN("prev. intersection on "<< normalized(eor));
            if( hit && !has_smaller_distance_to_point( ray.source(), q, eor))
              continue;
            _CGAL_NEF_TRACEN("is the intersection point on the boundary of the facet? "<<
                    is.does_contain_on_boundary( t, q));
            if( is.does_contain_on_boundary( t, q))
              continue;
            _CGAL_NEF_TRACEN("is the intersection point on the current cell? "<<
                    candidate_provider->is_point_on_cell(q,objects_iterator));
            if( !candidate_provider->is_point_on_cell( q, objects_iterator))
                continue;
            eor = q;
            result = Object_handle(Halffacet_handle(t));
            hit = true; 
            _CGAL_NEF_TRACEN("the facet becomes the new hit object");
          }
        }
#endif
        else if((mask&15) == 15)
          CGAL_assertion_msg( 0, "wrong handle");
      }
      if(!hit)
        ++objects_iterator;
    }
    TIMER(rs_t.stop());
    return result;
  }

  virtual Object_handle locate( const Point_3& p) const {
    if(Infi_box::extended_kernel()) {
    TIMER(pl_t.start());
    CGAL_assertion( initialized);
    _CGAL_NEF_TRACEN( "locate "<<p);
    Object_handle result;
    Vertex_handle v;
    Halfedge_handle e;
    Halffacet_handle f;
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
    Halffacet_triangle_handle t;
#endif
#ifdef CGAL_NEF3_FACET_WITH_BOX
    Partial_facet pf;
#endif
    Object_list candidates = candidate_provider->objects_around_point(p);
    Object_list_iterator o = candidates.begin();
    bool found = false;
    while( !found && o != candidates.end()) {
      if( CGAL::assign( v, *o)) {
        if ( p == v->point()) {
          _CGAL_NEF_TRACEN("found on vertex "<<v->point())          
          result = Object_handle(v);
          found = true;
        }
      }
      else if( CGAL::assign( e, *o)) {
        if ( is.does_contain_internally(Segment_3(e->source()->point(),e->twin()->source()->point()), p) ) {
          _CGAL_NEF_TRACEN("found on edge "<<Segment_3(e->source()->point(),e->twin()->source()->point()));
          result = Object_handle(e);
          found = true;
        }
      }
      else if( CGAL::assign( f, *o)) {
        if ( is.does_contain_internally( f, p) ) {
          _CGAL_NEF_TRACEN("found on facet...");
          result = Object_handle(f);
          found = true;
        }
      }
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
      else if( CGAL::assign( t, *o)) {
        Triangle_3 tr = t.get_triangle();
        if( tr.has_on(p)) {
          _CGAL_NEF_TRACEN("found on triangle "<<tr);
	  if(is.does_contain_on_boundary( t, p)) {
	    _CGAL_NEF_TRACEN("but located on the facet's boundary");
	    continue;
	  }
          result = Object_handle(Halffacet_handle(t));
	  found = true;
        }
      }
#endif
#ifdef CGAL_NEF3_FACET_WITH_BOX 
      else if( CGAL::assign(pf, *o)) {
	CGAL_NEF_TRACEN("new locate ");
        if ( is.does_contain_internally( pf, p) ) {
          _CGAL_NEF_TRACEN("found on partial facet...");
          result = Object_handle(pf.f);
          found = true;
        }
      }
#endif
      o++;
    }
    if( !found) {
      _CGAL_NEF_TRACEN("point not found in 2-skeleton");
      _CGAL_NEF_TRACEN("shooting ray to determine the volume");
      Ray_3 r( p, Vector_3( -1, 0, 0));
      result = Object_handle(determine_volume(r));
    }    TIMER(pl_t.start());
    TIMER(pl_t.stop());
    return result;
  } else {   // standard kernel
    CGAL_assertion( initialized);
    _CGAL_NEF_TRACEN( "locate "<<p);
    typename SNC_structure::FT min_distance;
    typename SNC_structure::FT tmp_distance;
    Object_handle result;
    Vertex_handle v;
    Halfedge_handle e;
    Halffacet_handle f;
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
    Halffacet_triangle_handle t;
#endif
#ifdef CGAL_NEF3_FACET_WITH_BOX
    Partial_facet pf;
#endif
    Object_list candidates = candidate_provider->objects_around_point(p);
    Object_list_iterator o = candidates.begin();

    if(candidates.empty())
      return Base(*this).volumes_begin();

    CGAL::assign(v,*o);
    CGAL_assertion(CGAL::assign(v,*o));
    if(p==v->point())
      return Object_handle(v);

    min_distance = CGAL::squared_distance(v->point(),p);
    result = Object_handle(v);
    ++o;
    while(o!=candidates.end() && CGAL::assign(v,*o)) {
      if ( p == v->point()) {
        _CGAL_NEF_TRACEN("found on vertex "<<v->point());
        return Object_handle(v);
      }
      tmp_distance = CGAL::squared_distance(v->point(),p);
      if(tmp_distance < min_distance) {
	result = Object_handle(v);
        min_distance = tmp_distance;
      }
      ++o;
    }     

    CGAL::assign(v, result);
    Segment_3 s(p,v->point());
    Point_3 ip;

//    Object_list_iterator ox(o);
    for(;o!=candidates.end();++o) {
      if( CGAL::assign( e, *o)) {
	Segment_3 ss(e->source()->point(),e->twin()->source()->point());
	CGAL_NEF_TRACEN("test edge " << e->source()->point() << "->" << e->twin()->source()->point());
        if(is.does_contain_internally(ss, p) ) {
          _CGAL_NEF_TRACEN("found on edge "<< ss);
          return Object_handle(e);
        }
	if(is.does_intersect_internally(s, ss, ip)) {
	  s = Segment_3(p, normalized(ip));
	  result = Object_handle(e);
        }
      } else if( CGAL::assign( f, *o)) {
	CGAL_NEF_TRACEN("test facet " << f->plane());
        if (is.does_contain_internally(f,p) ) {
          _CGAL_NEF_TRACEN("found on facet...");
          return Object_handle(f);
        }
        if( is.does_intersect_internally(s,f,ip)) {	
          s = Segment_3(p, normalized(ip));
	  result = Object_handle(f);
        }
#ifdef CGAL_NEF3_FACET_WITH_BOX
      } else if( CGAL::assign(pf, *o)) {
	CGAL_NEF_TRACEN("new locate ");
	if ( is.does_contain_internally( pf, p) ) {
	  _CGAL_NEF_TRACEN("found on partial facet...");
	  return Object_handle(pf.f);
	}
        if( is.does_intersect_internally(s,pf,ip)) {	
          s = Segment_3(p, normalized(ip));
	  result = Object_handle(pf.f);
        }
#endif
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
      } else if( CGAL::assign(t, *o)) {
        CGAL_NEF_TRACEN("test triangle of facet " << t->plane());
        Triangle_3 tr = t.get_triangle();
        CGAL_NEF_TRACEN("trying triangle "<<tr);	
	if(tr.has_on(p)) {
	  f = t;
	  return Object_handle(f);
        }
        if( is.does_intersect( s, tr, ip)) {
	  CGAL_assertion(!is.does_contain_on_boundary( t, ip));
          s = Segment_3(p, normalized(ip));
	  result = Object_handle(t);
        }
#endif
      } 
      else CGAL_assertion_msg(false, "wrong handle type");
    }

    if( CGAL::assign( v, result)) {
      _CGAL_NEF_TRACEN("vertex hit, obtaining volume...");
      SM_point_locator L(&*v);
      Object_handle so = L.locate(s.source()-s.target());
      SFace_handle sf;
      if(CGAL::assign(sf,so))
        return sf->volume();

      CGAL_assertion_msg(false, "wrong handle type");
/*
      SHalfedge_handle se;
      CGAL_assertion(CGAL::assign(se,so));
      CGAL_NEF_TRACEN("intersect segment " << s << " with edges");
      for(;ox!=candidates.end();++ox) {
	if(!CGAL::assign(e,*ox)) continue;
	CGAL_NEF_TRACEN("test edge " << e->source()->point() << "->" << e->twin()->source()->point());
	if(is.does_intersect_internally(s,Segment_3(e->source()->point(),e->twin()->source()->point()),ip)) {
	  s = Segment_3(p, normalized(ip));
	  result = Object_handle(e);
        }
      }
      CGAL_assertion(CGAL::assign(e,result));
      CGAL::assign(e,result);
      f = get_visible_facet(e, Ray_3(p, s.target()));	
      if( f != Halffacet_handle())
	return f->incident_volume();
      SM_decorator SD(&*v); // now, the vertex has no incident facets
      CGAL_assertion( SD.number_of_sfaces() == 1);
      return SD.sfaces_begin()->volume();      
*/
    } else if( CGAL::assign( f, result)) {
      _CGAL_NEF_TRACEN("facet hit, obtaining volume...");
      if(f->plane().oriented_side(p) == ON_NEGATIVE_SIDE)
	f = f->twin();
      return Object_handle(f->incident_volume());
#ifdef CGAL_NEF3_FACET_WITH_BOX
    } else if( CGAL::assign(pf, *o)) {
      CGAL_assertion_msg(false, "should not be executed");
      Halffacet_handle f = pf.f;
      if(f->plane().oriented_side(p) == ON_NEGATIVE_SIDE)
	f = f->twin();
      return Object_handle(f->incident_volume()); 
#endif
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
    } else if( CGAL::assign(t, result)) {
      f = t;
      _CGAL_NEF_TRACEN("facet hit, obtaining volume...");
      if(f->plane().oriented_side(p) == ON_NEGATIVE_SIDE)
	f = f->twin();
      return Object_handle(f->incident_volume());
#endif
    } else if( CGAL::assign(e, result)) {
      SM_decorator SD(&*e->source());
      if( SD.is_isolated(e))
        return Object_handle(e->incident_sface()->volume());	
      return get_visible_facet(e,Ray_3(s.source(),s.to_vector()))->incident_volume();
    }
    CGAL_assertion_msg(false, "wrong handle type");
    return Object_handle();
  }
  }

  virtual void intersect_with_edges_and_facets( Halfedge_handle e0,
	const typename SNC_point_locator::Intersection_call_back& call_back) const {

    TIMER(it_t.start());
    CGAL_assertion( initialized);
    _CGAL_NEF_TRACEN( "intersecting edge: "<<&*e0<<' '<<Segment_3(e0->source()->point(),
                                                         e0->twin()->source()->point()));

#ifdef CGAL_NEF3_TRIANGULATE_FACETS
    Unique_hash_map< Halffacet_triangle_handle, bool> f_mark(false);
#endif // CGAL_NEF3_TRIANGULATE_FACETS

    Segment_3 s(Segment_3(e0->source()->point(),e0->twin()->source()->point()));
    Vertex_handle v;
    Halfedge_handle e;
    Halffacet_handle f;
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
    Halffacet_triangle_handle t;
#endif
#ifdef CGAL_NEF3_FACET_WITH_BOX
    Partial_facet pf;
#endif
    Object_list_iterator o;
    Object_list objects = candidate_provider->objects_around_segment(s);
    CGAL_for_each( o, objects) {
      if( CGAL::assign( v, *o)) {
        /* do nothing */
      }
      else if( CGAL::assign( e, *o)) {

#ifdef CGAL_NEF3_DUMP_STATISTICS
      ++number_of_intersection_candidates;
#endif

        Point_3 q;
        if( is.does_intersect_internally( s, Segment_3(e->source()->point(),
	                                               e->twin()->source()->point()), q)) {
          q = normalized(q);
          call_back( e0, Object_handle(Halfedge_handle(e)), q);
          _CGAL_NEF_TRACEN("edge intersects edge "<<' '<<&*e<< Segment_3(e->source()->point(),
                                                                e->twin()->source()->point())<<" on "<<q);
        }
      }
      else if( CGAL::assign( f, *o)) {
#ifdef CGAL_NEF3_DUMP_STATISTICS
      ++number_of_intersection_candidates;
#endif

        Point_3 q;
        if( is.does_intersect_internally( s, f, q) ) {
          q = normalized(q);
          call_back( e0, Object_handle(Halffacet_handle(f)), q);
          _CGAL_NEF_TRACEN("edge intersects facet on plane "<<f->plane()<<" on "<<q);
        }
      }
#ifdef CGAL_NEF3_FACET_WITH_BOX
	else if( CGAL::assign(pf, *o)) {
	  CGAL_assertion_msg(false, "not implemented yet");
	}
#endif
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
      else if( CGAL::assign( t, *o)) {
        Point_3 q;
        Triangle_3 tr = t.get_triangle();

	if( f_mark[t])
	  continue;
	_CGAL_NEF_TRACEN("trying with triangle "<<tr);
        if( is.does_intersect( s, tr, q) &&
            !is.does_contain_on_boundary( t, q)) {
          q = normalized(q);
          call_back( e0, Object_handle(Halffacet_handle(t)), q);
          _CGAL_NEF_TRACEN("edge intersects facet triangle on plane "<<t->plane()<<" on "<<q);
	  f_mark[t] = true;
        }
      }
#endif // CGAL_NEF3_TRIANGULATE_FACETS
      else
        CGAL_assertion_msg( 0, "wrong handle");
    }
    TIMER(it_t.stop());
  }

  virtual void intersect_with_edges( Halfedge_handle e0,
    const typename SNC_point_locator::Intersection_call_back& call_back) const {
    TIMER(it_t.start());
    CGAL_assertion( initialized);
    _CGAL_NEF_TRACEN( "intersecting edge: "<<&*e0<<' '<<Segment_3(e0->source()->point(),
                                                         e0->twin()->source()->point()));
    Segment_3 s(Segment_3(e0->source()->point(),e0->twin()->source()->point()));
    Vertex_handle v;
    Halfedge_handle e;
    Halffacet_handle f;
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
    Halffacet_triangle_handle t;
#endif
#ifdef CGAL_NEF3_FACET_WITH_BOX
    Partial_facet pf;
#endif
    Object_list_iterator o;
    Object_list objects = candidate_provider->objects_around_segment(s);
    CGAL_for_each( o, objects) {
      if( CGAL::assign( v, *o)) {
        /* do nothing */
      }
      else if( CGAL::assign( e, *o)) {

#ifdef CGAL_NEF3_DUMP_STATISTICS
      ++number_of_intersection_candidates;
#endif

        Point_3 q;
        if( is.does_intersect_internally( s, Segment_3(e->source()->point(),
	                                               e->twin()->source()->point()), q)) {
          q = normalized(q);
          call_back( e0, Object_handle(Halfedge_handle(e)), q);
          _CGAL_NEF_TRACEN("edge intersects edge "<<' '<<&*e<< Segment_3(e->source()->point(),
                                                                e->twin()->source()->point())<<" on "<<q);
        }
      }
      else if( CGAL::assign( f, *o)) {
        /* do nothing */
      }
#ifdef CGAL_NEF3_FACET_WITH_BOX
	else if( CGAL::assign(pf, *o)) {
	  CGAL_assertion_msg(false, "not implemented yet");
	}
#endif
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
      else if( CGAL::assign( t, *o)) {
        /* do nothing */
      }
#endif
      else
        CGAL_assertion_msg( 0, "wrong handle");
    }
    TIMER(it_t.stop());
  }

  virtual void intersect_with_facets( Halfedge_handle e0, 
    const typename SNC_point_locator::Intersection_call_back& call_back) const {
    TIMER(it_t.start());
    CGAL_assertion( initialized);
    _CGAL_NEF_TRACEN( "intersecting edge: "<< Segment_3(e0->source()->point(),
                                               e0->twin()->source()->point()));
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
    Unique_hash_map< Halffacet_triangle_handle, bool> f_mark(false);
#endif // CGAL_NEF3_TRIANGULATE_FACETS
    Segment_3 s(Segment_3(e0->source()->point(),e0->twin()->source()->point()));
    Vertex_handle v;
    Halfedge_handle e;
    Halffacet_handle f;
#ifdef CGAL_NEF3_FACET_WITH_BOX
    Partial_facet pf;
#endif
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
    Halffacet_triangle_handle t;
#endif
    Object_list_iterator o;
    Object_list objects = candidate_provider->objects_around_segment(s);
    CGAL_for_each( o, objects) {
      if( CGAL::assign( v, *o)) {
        /* do nothing */
      }
      else if( CGAL::assign( e, *o)) {
        /* do nothing */
      }
      else if( CGAL::assign( f, *o)) {

#ifdef CGAL_NEF3_DUMP_STATISTICS
      ++number_of_intersection_candidates;
#endif

        Point_3 q;
        if( is.does_intersect_internally( s, f, q) ) {
          q = normalized(q);
          call_back( e0, Object_handle(Halffacet_handle(f)), q);
          _CGAL_NEF_TRACEN("edge intersects facet on plane "<<f->plane()<<" on "<<q);
        }
      }
#ifdef CGAL_NEF3_FACET_WITH_BOX
      else if( CGAL::assign(pf, *o)) {
	CGAL_assertion_msg(false, "not implemented yet");
      }
#endif
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
      else if( CGAL::assign( t, *o)) {
        Point_3 q;
        Triangle_3 tr = t.get_triangle();
	if( f_mark[t])
	  continue;
	_CGAL_NEF_TRACEN("trying with triangle "<<tr);
        if( is.does_intersect( s, tr, q) &&
            !is.does_contain_on_boundary( t, q)) {
          q = normalized(q);
          call_back( e0, Object_handle(Halffacet_handle(t)), q);
          _CGAL_NEF_TRACEN("edge intersects facet triangle on plane "<<t->plane()<<" on "<<q);
	  f_mark[t] = true;
        }
      }
#endif // CGAL_NEF3_TRIANGULATE_FACETS
      else
        CGAL_assertion_msg( 0, "wrong handle");
    }
    TIMER(it_t.stop());
  }

private:
  Volume_handle determine_volume( const Ray_3& ray) const {
    Halffacet_handle f_below;
    Object_handle o = shoot(ray);
    Vertex_handle v;
    Halfedge_handle e;
    Halffacet_handle f;
    if( CGAL::assign( v, o)) {
      _CGAL_NEF_TRACEN("vertex hit, obtaining volume...");
      f_below = get_visible_facet( v, ray);
      if( f_below != Halffacet_handle())
        return f_below->incident_volume();
      SM_decorator SD(&*v); // now, the vertex has no incident facets
      CGAL_assertion( SD.number_of_sfaces() == 1);
      return SD.sfaces_begin()->volume();
    }
    else if( CGAL::assign( e, o)) {
      _CGAL_NEF_TRACEN("edge hit, obtaining volume...");
      f_below = get_visible_facet( e, ray);
      if( f_below != Halffacet_handle())
        return f_below->incident_volume();
      CGAL_assertion_code(SM_decorator SD(&*e->source())); // now, the edge has no incident facets
      CGAL_assertion(SD.is_isolated(e));
      return e->incident_sface()->volume();
    }
    else if( CGAL::assign( f, o)) {
      _CGAL_NEF_TRACEN("facet hit, obtaining volume...");
      f_below = get_visible_facet(f, ray);
      CGAL_assertion( f_below != Halffacet_handle());
      return f_below->incident_volume();
    }
    return Base(*this).volumes_begin(); // TODO: Comment this hack!
  }

public:
  void add_facet(Halffacet_handle f) {
    candidate_provider->add_facet(f);
  }

  void add_edge(Halfedge_handle e) {
    candidate_provider->add_edge(e);	
  }

  void add_vertex(Vertex_handle v) {
    candidate_provider->add_vertex(v);
  }

private:
  bool initialized;
  SNC_candidate_provider* candidate_provider;
  SNC_intersection is;
#ifdef CGAL_NEF3_TRIANGULATE_FACETS
  std::list<Halffacet_triangle_handle> triangulation;
#endif
};

#ifdef CGAL_NEF3_POINT_LOCATOR_NAIVE
template <typename SNC_decorator>
class SNC_point_locator_naive : 
  public SNC_ray_shooter<SNC_decorator>, 
  public SNC_point_locator<SNC_decorator>
{
  typedef typename SNC_decorator::SNC_structure SNC_structure;
  typedef SNC_ray_shooter<SNC_decorator> Base;
  typedef SNC_point_locator_naive<SNC_decorator> Self;
  typedef SNC_point_locator<SNC_decorator> SNC_point_locator;
  typedef CGAL::SNC_intersection<SNC_structure> SNC_intersection;
  typedef typename SNC_decorator::Decorator_traits Decorator_traits;
  typedef typename Decorator_traits::SM_decorator SM_decorator;

public:
  typedef typename SNC_decorator::Object_handle Object_handle;
  typedef typename SNC_decorator::Point_3 Point_3;
  typedef typename SNC_decorator::Segment_3 Segment_3;
  typedef typename SNC_decorator::Ray_3 Ray_3;
  typedef typename SNC_structure::Aff_transformation_3 
                                  Aff_transformation_3;

  typedef typename Decorator_traits::Vertex_handle Vertex_handle;
  typedef typename Decorator_traits::Halfedge_handle Halfedge_handle;
  typedef typename Decorator_traits::Halffacet_handle Halffacet_handle;
  typedef typename Decorator_traits::Volume_handle Volume_handle;
  typedef typename Decorator_traits::Vertex_iterator Vertex_iterator;
  typedef typename Decorator_traits::Halfedge_iterator Halfedge_iterator;
  typedef typename Decorator_traits::Halffacet_iterator Halffacet_iterator;

public:
  SNC_point_locator_naive() : initialized(false) {}
  virtual void initialize(SNC_structure* W) { 
    TIMER(ct_t.start());
    strcpy(this->version_, "Naive Point Locator (tm)");
    CLOG(version());
    CGAL_assertion( W != NULL);
    Base::initialize(W); 
    initialized = true;
    TIMER(ct_t.stop());
  }

  virtual Self* clone() const { 
    return new Self; 
  }

  virtual bool update( Unique_hash_map<Vertex_handle, bool>& V, 
                       Unique_hash_map<Halfedge_handle, bool>& E, 
                       Unique_hash_map<Halffacet_handle, bool>& F) {
    TIMER(ct_t.start());
    CGAL_assertion( initialized);
    TIMER(ct_t.stop());
    return false;
  }

  virtual ~SNC_point_locator_naive() {}

  virtual Object_handle locate(const Point_3& p) const {
    TIMER(pl_t.start());
    CGAL_assertion( initialized);
    TIMER(pl_t.stop());
    return Base::locate(p);
  }

  virtual Object_handle shoot(const Ray_3& r, int mask=0) const {
    TIMER(rs_t.start());
    CGAL_assertion( initialized);
    TIMER(rs_t.stop());
    return Base::shoot(r);
  }

  virtual void transform(const Aff_transformation_3& aff) {}

  virtual void intersect_with_edges_and_facets(Halfedge_handle e0,
    const typename SNC_point_locator::Intersection_call_back& call_back) const {
	intersect_with_edges(e0,call_back);
	intersect_with_facets(e0,call_back);
  }

  virtual void intersect_with_edges( Halfedge_handle e0, 
    const typename SNC_point_locator::Intersection_call_back& call_back) const {
    TIMER(it_t.start());
    CGAL_assertion( initialized);
    CGAL_NEF_TRACEN( "intersecting edge: "<< Segment_3(e0->source()->point(),
                                              e0->twin()->source()->point()));
    SNC_intersection is(*this->sncp());
    Segment_3 s(Segment_3(e0->source()->point(),e0->twin()->source()->point()));
    Halfedge_iterator e;
    CGAL_forall_edges( e, *this->sncp()) {

#ifdef CGAL_NEF3_DUMP_STATISTICS
      ++number_of_intersection_candidates;
#endif

      Point_3 q;
      if( is.does_intersect_internally( s, Segment_3(e->source()->point(),
                                                     e->twin()->source()->point()), q)) {
        q = normalized(q);
        CGAL_NEF_TRACEN("edge intersects edge "<< Segment_3(e->source()->point(),
                                                   e->twin()->source()->point()) <<" on "<<q);
        call_back( e0, Object_handle(e), q);
      }
    }
    TIMER(it_t.stop());
  }

  virtual void intersect_with_facets( Halfedge_handle e0, 
    const typename SNC_point_locator::Intersection_call_back& call_back) const {
    TIMER(it_t.start());
    CGAL_assertion( initialized);
    CGAL_NEF_TRACEN( "intersecting edge: "<< Segment_3(e0->source()->point(),
                                              e0->twin()->source()->point()));
    SNC_intersection is(*this->sncp());
    Segment_3 s(Segment_3(e0->source()->point(),
                          e0->twin()->source()->point()));
    Halffacet_iterator f;
    CGAL_forall_facets( f, *this->sncp()) {

#ifdef CGAL_NEF3_DUMP_STATISTICS
      ++number_of_intersection_candidates;
#endif

      Point_3 q;
      if( is.does_intersect_internally( s, f, q) ) {
        q = normalized(q);
        CGAL_NEF_TRACEN("edge intersects facet on plane "<<f->plane()<<" on "<<q);
        call_back( e0, Object_handle(f), q);
      }
    }
    TIMER(it_t.stop());
  }

private:
  bool initialized;
};
#endif

CGAL_END_NAMESPACE
#endif // SNC_POINT_LOCATOR_H

