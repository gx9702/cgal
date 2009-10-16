#define CGAL_PROFILE
#undef CGAL_NO_STATIC_FILTERS


#include <CGAL/MP_Float.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Filtered_kernel.h>
#include <CGAL/Kernel_checker.h>
#include <CGAL/Cartesian_converter.h>
#include <CGAL/Random.h>
#include <CGAL/Regular_triangulation_euclidean_traits_3.h>


typedef CGAL::Simple_cartesian<CGAL::Quotient<CGAL::MP_Float> >          Exact_kernel;
typedef CGAL::Simple_cartesian<double>                                   Double_kernel;
typedef CGAL::Filtered_kernel<Double_kernel>                             Filtered_kernel;

typedef CGAL::Regular_triangulation_euclidean_traits_3<Exact_kernel>     Exact_traits;
typedef CGAL::Regular_triangulation_euclidean_traits_3<Filtered_kernel>  Filtered_traits;
typedef std::pair<double,Exact_traits::FT>                               NT_pair;

template < class K1, class K2, class Cmp = CGAL::dont_check_equal >
class Regular_traits_checker : public CGAL::Kernel_checker<K1,K2,Cmp>
{
public:

    typedef CGAL::Comparison_result   Comparison_result;
    typedef CGAL::Oriented_side       Oriented_side;

    typedef K1                        Kernel1;
    typedef K2                        Kernel2;
    typedef Cmp                       Comparator;

    // Kernel objects are defined as pairs, with primitives run in parallel.
#define CGAL_kc_pair(X) typedef std::pair<typename K1::X, typename K2::X> X;

    CGAL_kc_pair(Weighted_point_3)

#undef CGAL_kc_pair

#define CGAL_Kernel_pred(X, Y) \
    typedef CGAL::Primitive_checker<typename K1::X, typename K2::X, Cmp> X; \
    X Y() const { return X(this->k1.Y(), this->k2.Y(), this->cmp); }

#define CGAL_Kernel_cons(Y,Z) CGAL_Kernel_pred(Y,Z)

public:
  
  CGAL_Kernel_pred(Compare_weighted_squared_radius_3,compare_weighted_squared_radius_3_object)
  CGAL_Kernel_pred(Power_test_3,power_test_3_object)
};


typedef Regular_traits_checker<Filtered_traits,Exact_traits>           K3;

typedef K3::Weighted_point_3                    Weighted_point_3;


CGAL::Random *r;

double rand_base()
{
  return r->get_double(0, 1);
}

// Random double almost in [0;1].
double my_rand()
{
  // Ensure 53 random bits, not 48.
  return rand_base() + rand_base()/1024;
}

// Random point in unit cube.
Weighted_point_3 my_rand_wp3()
{
  double x = my_rand(), y = my_rand(), z = my_rand(), r=my_rand();
  return Weighted_point_3( Filtered_traits::Weighted_point_3(Filtered_traits::Bare_point(x, y, z),r) , Exact_traits::Weighted_point_3(Exact_traits::Bare_point(x, y, z),r) );
}

// Perturbation with given maximum relative epsilon.
void perturb(Weighted_point_3 &p, double rel_eps)
{
  double x = p.first.x()*(1+rand_base()*rel_eps);
  double y = p.first.y()*(1+rand_base()*rel_eps);
  double z = p.first.z()*(1+rand_base()*rel_eps);
  double r= p.first.weight()*(1+rand_base()*rel_eps);
  p=Weighted_point_3( Filtered_traits::Weighted_point_3(Filtered_traits::Bare_point(x, y, z),r) , Exact_traits::Weighted_point_3(Exact_traits::Bare_point(x, y, z),r) );
}

void test_compare_weighted_squared_radius_3(){
  Weighted_point_3 p=my_rand_wp3();
  Weighted_point_3 q=my_rand_wp3();
  Weighted_point_3 r=my_rand_wp3();
  Weighted_point_3 s=my_rand_wp3();
  double alpha=my_rand();
  
  
  //test with random points + random alpha
  K3().compare_weighted_squared_radius_3_object()(p,q,r,s,NT_pair(alpha,alpha));
  K3().compare_weighted_squared_radius_3_object()(p,q,r,  NT_pair(alpha,alpha));
  K3().compare_weighted_squared_radius_3_object()(p,q,    NT_pair(alpha,alpha));
  K3().compare_weighted_squared_radius_3_object()(p,      NT_pair(alpha,alpha));

  
  Exact_traits::Compute_squared_radius_smallest_orthogonal_sphere_3 radius=Exact_traits().compute_squared_radius_smallest_orthogonal_sphere_3_object();
  double alpha_pqrs=CGAL::to_double( radius(p.second,q.second,r.second,s.second) );
  double alpha_pqr=CGAL::to_double( radius(p.second,q.second,r.second) );
  double alpha_pq=CGAL::to_double( radius(p.second,q.second) );
  double alpha_p=p.first.weight();
  
  //test with random points + alpha limit
  K3().compare_weighted_squared_radius_3_object()(p,q,r,s,NT_pair(alpha_pqrs,alpha_pqrs));
  K3().compare_weighted_squared_radius_3_object()(p,q,r,  NT_pair(alpha_pqr,alpha_pqr));
  K3().compare_weighted_squared_radius_3_object()(p,q,    NT_pair(alpha_pq,alpha_pq));
  K3().compare_weighted_squared_radius_3_object()(p,      NT_pair(alpha_p,alpha_p));
  //test correct result
  assert( K3().compare_weighted_squared_radius_3_object()(p,q,r,s,NT_pair(alpha_pqrs-0.1,alpha_pqrs-0.1)) ==CGAL::POSITIVE );
  assert( K3().compare_weighted_squared_radius_3_object()(p,q,r,  NT_pair(alpha_pqr-0.1,alpha_pqr-0.1)) ==CGAL::POSITIVE );
  assert( K3().compare_weighted_squared_radius_3_object()(p,q,    NT_pair(alpha_pq-0.1,alpha_pq-0.1)) ==CGAL::POSITIVE );
  assert( K3().compare_weighted_squared_radius_3_object()(p,      NT_pair(alpha_p-0.1,alpha_p-0.1)) ==CGAL::POSITIVE );

  // Then with some perturbation on coordinates and weight.
  perturb(p, 1.0/(1<<25)/(1<<20)); // 2^-45
  
  K3().compare_weighted_squared_radius_3_object()(p,q,r,s,NT_pair(alpha_pqrs,alpha_pqrs));
  K3().compare_weighted_squared_radius_3_object()(p,q,r,  NT_pair(alpha_pqr,alpha_pqr));
  K3().compare_weighted_squared_radius_3_object()(p,q,    NT_pair(alpha_pq,alpha_pq));
  K3().compare_weighted_squared_radius_3_object()(p,      NT_pair(alpha_p,alpha_p));
  
}

void test_power_test_3(){
  
  CGAL::Weighted_converter_3<CGAL::Cartesian_converter<Exact_kernel,Filtered_kernel> > convert_to_double;
  CGAL::Weighted_converter_3<CGAL::Cartesian_converter<Filtered_kernel,Exact_kernel> > convert_to_exact;
  
  Weighted_point_3 p=my_rand_wp3();
  Weighted_point_3 q=my_rand_wp3();
  Weighted_point_3 r=my_rand_wp3();
  Weighted_point_3 s=my_rand_wp3();  
  Weighted_point_3 query_pt=my_rand_wp3();  

  //test with random points
  K3().power_test_3_object()(p,q,r,s,query_pt);  
  K3().power_test_3_object()(p,q,r,query_pt);  
  K3().power_test_3_object()(p,q,query_pt);  
  K3().power_test_3_object()(p,query_pt);
  
  //test in degenerate case
  Exact_traits::Weighted_point_3::Point origin(0,0,0);
  Exact_traits::Weighted_point_3 tmp=Exact_traits::Weighted_point_3(
    origin,
      CGAL::squared_distance(origin,Exact_traits().construct_weighted_circumcenter_3_object()(p.second,q.second,r.second,s.second))-
      Exact_traits().compute_squared_radius_smallest_orthogonal_sphere_3_object()(p.second,q.second,r.second,s.second)
  );
  assert(Exact_traits().power_test_3_object()(p.second,q.second,r.second,s.second,tmp)==CGAL::ON_ORIENTED_BOUNDARY);
  
  Weighted_point_3 ortho_pqrs( convert_to_double(tmp),convert_to_exact( convert_to_double(tmp) ) );
  tmp=Exact_traits::Weighted_point_3(
      Exact_traits().construct_weighted_circumcenter_3_object()(p.second,q.second,r.second),
      -Exact_traits().compute_squared_radius_smallest_orthogonal_sphere_3_object()(p.second,q.second,r.second)
    );
  assert(Exact_traits().power_test_3_object()(p.second,q.second,r.second,tmp)==CGAL::ON_ORIENTED_BOUNDARY);    
  Weighted_point_3 ortho_pqr( convert_to_double(tmp),convert_to_exact( convert_to_double(tmp) ) );
  tmp=Exact_traits::Weighted_point_3(
      Exact_traits().construct_weighted_circumcenter_3_object()(p.second,q.second),
      -Exact_traits().compute_squared_radius_smallest_orthogonal_sphere_3_object()(p.second,q.second)
    );
  assert(Exact_traits().power_test_3_object()(p.second,q.second,tmp)==CGAL::ON_ORIENTED_BOUNDARY);        
  Weighted_point_3 ortho_pq( convert_to_double(tmp),convert_to_exact( convert_to_double(tmp) ) );
  
  
  K3().power_test_3_object()(p,q,r,s,ortho_pqrs);
  K3().power_test_3_object()(p,q,r  ,ortho_pqr);
  K3().power_test_3_object()(p,q    ,ortho_pq);
  // Then with some perturbation on coordinates and weight.
  perturb(p, 1.0/(1<<25)/(1<<20)); // 2^-45
  
  K3().power_test_3_object()(p,q,r,s,ortho_pqrs);
  K3().power_test_3_object()(p,q,r  ,ortho_pqr);
  K3().power_test_3_object()(p,q    ,ortho_pq);
}


int main(int argc, char **argv)
{
  int loops = (argc < 2) ? 2000 : std::atoi(argv[1]);
  int seed  = (argc < 3) ? CGAL::default_random.get_int(0, 1<<30)
                         : std::atoi(argv[2]);

  std::cout << "Initializing random generator with seed = " << seed
            << std::endl
            << "#loops = " << loops << " (can be changed on the command line)"
            << std::endl;

  CGAL::Random rnd(seed);
  r = &rnd;

  std::cout.precision(20);
  std::cerr.precision(20);

  std::cout << "ulp(1) = " << CGAL::internal::Static_filter_error::ulp() << std::endl;

  std::cout << "Testing Compare_weighted_squared_radius_3" << std::endl;
  for(int i=0; i<loops; ++i)
    test_compare_weighted_squared_radius_3();  

  std::cout << "Testing Power_test_3" << std::endl;
  for(int i=0; i<loops; ++i)
    test_power_test_3();  

  return 0;
}











