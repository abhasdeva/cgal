// ======================================================================
//
// Copyright (c) The CGAL Consortium
//
// This software and related documentation is part of an INTERNAL release
// of the Computational Geometry Algorithms Library (CGAL). It is not
// intended for general use.
//
// ----------------------------------------------------------------------
//
// release       : $CGAL_Revision: CGAL-2.3-I-24 $
// release_date  : $CGAL_Date: 2000/12/29 $
//
// file          : include/CGAL/Isr_kd_2.h
// package       : arr (1.73)
// maintainer    : Eli Packer <elip@post.tau.ac.il>
// author(s)     : Eli Packer
// coordinator   : Tel-Aviv University (Dan Halperin <halperin@math.tau.ac.il>)
//
// ======================================================================
#ifndef CGAL_SR_KD_2_H
#define CGAL_SR_KD_2_H

#ifndef CGAL_CONFIG_H
#include <CGAL/config.h>
#endif

#include <list>

#ifndef  CGAL_KDTREE_D_H
#include <CGAL/kdtree_d.h>
#endif

#ifndef CGAL_VECTOR_2_H
#include <CGAL/Vector_2.h>
#endif

template<class NT,class SAVED_OBJECT>
class my_point : public CGAL::Point_2<CGAL::Cartesian<NT> > {

typedef CGAL::Point_2<CGAL::Cartesian<NT> >  Point;

public:
  Point orig;
  SAVED_OBJECT object;
  my_point(Point p,Point inp_orig,SAVED_OBJECT obj) : Point(p),orig(inp_orig),
           object(obj) {}
  my_point(Point p) : Point(p),orig(Point(0,0)) {}
  my_point() : Point(),orig() {}
  my_point(NT x,NT y) : Point(x,y),orig(Point(0,0)) {}
};

template<class NT,class SAVED_OBJECT>
class Multiple_kd_tree {

typedef CGAL::Point_2<CGAL::Cartesian<NT> >  Point;
typedef CGAL::Segment_2<CGAL::Cartesian<NT> >      Segment;
typedef CGAL::Kdtree_interface_2d<my_point<NT,SAVED_OBJECT> >  kd_interface;
typedef CGAL::Kdtree_d<kd_interface>  kd_tree;
typedef typename kd_tree::Box Box;
typedef std::list<my_point<NT,SAVED_OBJECT> > Points_List; 

private:
  static map<const int,NT> angle_to_sines_appr;
  static bool map_done;
  const double pi,half_pi,epsilon,rad_to_deg;
  int number_of_trees;
  std::list<std::pair<kd_tree *,NT> > kd_trees_list;
  std::list<std::pair<std::pair<NT,NT>,SAVED_OBJECT > > input_points_list;

  pair<kd_tree *,NT> create_kd_tree(NT angle)
  {
    Points_List l;
    kd_tree *tree = new kd_tree(2);

    for(typename std::list<std::pair<std::pair<NT,NT>,SAVED_OBJECT> >::iterator
        iter = input_points_list.begin();
        iter != input_points_list.end();
        ++iter) {

      Point p(iter->first.first,iter->first.second);

      rotate(p,angle);

      my_point<NT,SAVED_OBJECT> rotated_point(p,Point(iter->first.first,
                                iter->first.second),iter->second);

      l.push_back(rotated_point);
    }

    tree->build(l);

    //checking validity
    if(!tree->is_valid() )
      tree->dump();
    assert(tree->is_valid());

    return(pair<kd_tree *,NT>(tree,angle));
  }

  inline NT squere(NT x) {return(x * x);}
  inline NT min(NT x,NT y) {return((x < y) ? x : y);}
  inline NT max(NT x,NT y) {return((x < y) ? y : x);}
  inline NT min(NT x1,NT x2,NT x3,NT x4,NT x5,NT x6) {return(min(min(min(x1,x2),
                min(x3,x4)),min(x5,x6)));}
  inline NT max(NT x1,NT x2,NT x3,NT x4,NT x5,NT x6) {return(max(max(max(x1,x2),
                max(x3,x4)),max(x5,x6)));}

  int get_kd_num(std::pair<std::pair<NT,NT>,std::pair<NT,NT> > &seg,int n)
  {
    int i;
    double x1 = seg.first.first.to_double();
    double y1 = seg.first.second.to_double();
    double x2 = seg.second.first.to_double();
    double y2 = seg.second.second.to_double();
    double alpha = atan((y2 -y1)/(x2 - x1));

    if(alpha < 0)
      alpha += pi / 2.0;

    if(alpha >= pi * (2 * n - 1) / (4 * n))
      i = 0;
    else {
      alpha += pi / (4 * n);
      i = int(2 * n * alpha / pi);
    }

    return(i);
  }

  void check_kd(int *kd_counter,int number_of_trees,
       std::list<std::pair<std::pair<NT,NT>,std::pair<NT,NT> > > &seg_list)
  {
    for(int i = 0;i < number_of_trees;++i)
      kd_counter[i] = 0;

    int kd_num;
    for(typename std::list<std::pair<std::pair<NT,NT>,std::pair<NT,NT> > >::
        iterator iter = seg_list.begin();iter != seg_list.end();++iter) {
      kd_num = get_kd_num(*iter,number_of_trees);
      kd_counter[kd_num]++;
    }
  }

public:
  Multiple_kd_tree(std::list<std::pair<std::pair<NT,NT>,SAVED_OBJECT> > 
                   &inp_points_list,int inp_number_of_trees,
                   std::list<std::pair<std::pair<NT,NT>,std::pair<NT,NT> > >
                   &seg_list) : 
    pi(3.1415),half_pi(1.57075),epsilon(0.001),rad_to_deg(57.297),
    number_of_trees(inp_number_of_trees),input_points_list(inp_points_list)
  {
    init_angle_appr();
#ifdef TIMER
    CGAL::Timer t;
    t.start();
#endif

    pair<kd_tree *,NT> kd;

    // check that there are at least two trees
    if(number_of_trees < 1) {
      std::cerr << "There must be at least one kd-tree\n";
      exit(1);
    }

    // find the kd trees that have enough candidates  (segments with a close
    // slope
    int *kd_counter = new int[number_of_trees];
    int number_of_segments = seg_list.size();
    check_kd(kd_counter,number_of_trees,seg_list);

#ifdef DEBUG
    int x = 0;
#endif
    int ind = 0;
    for(NT angle = 0;angle < half_pi;angle += half_pi / number_of_trees) {

#ifdef DEBUG
      std::cerr << "creating kd tree:  nu. " << ++x << endl;
#endif
      if(kd_counter[ind] >= (double)number_of_segments /
	                    (double) number_of_trees / 2.0) {
        kd = create_kd_tree(angle);
        kd_trees_list.push_back(kd);
      }

      ++ind;
    }

#ifdef TIMER
    t.stop();

    std::cerr << endl << "Kd trees creation took " <<
                 t.time() << " seconds\n\n";
#endif
  }

  static void init_angle_appr()
  {
    if(map_done)
      return;

    map_done = true;

    angle_to_sines_appr[0] = NT(0);
    angle_to_sines_appr[1] = NT(115) / NT(6613);
    angle_to_sines_appr[2] = NT(57) / NT(1625);
    angle_to_sines_appr[3] = NT(39) / NT(761);
    angle_to_sines_appr[4] = NT(29) / NT(421);
    angle_to_sines_appr[5] = NT(23) / NT(265);
    angle_to_sines_appr[6] = NT(19) / NT(181);
    angle_to_sines_appr[7] = NT(32) / NT(257);
    angle_to_sines_appr[8] = NT(129) / NT(929);
    angle_to_sines_appr[9] = NT(100) / NT(629);
    angle_to_sines_appr[10] = NT(92) / NT(533);
    angle_to_sines_appr[11] = NT(93) / NT(485);
    angle_to_sines_appr[12] = NT(76) / NT(365);
    angle_to_sines_appr[13] = NT(156) / NT(685);
    angle_to_sines_appr[14] = NT(205) / NT(853);
    angle_to_sines_appr[15] = NT(69) / NT(269);
    angle_to_sines_appr[16] = NT(7) / NT(25);
    angle_to_sines_appr[17] = NT(120) / NT(409);
    angle_to_sines_appr[18] = NT(57) / NT(185);
    angle_to_sines_appr[19] = NT(12) / NT(37);
    angle_to_sines_appr[20] = NT(51) / NT(149);
    angle_to_sines_appr[21] = NT(135) / NT(377);
    angle_to_sines_appr[22] = NT(372) / NT(997);
    angle_to_sines_appr[23] = NT(348) / NT(877);
    angle_to_sines_appr[24] = NT(231) / NT(569);
    angle_to_sines_appr[25] = NT(36) / NT(85);
    angle_to_sines_appr[26] = NT(39) / NT(89);
    angle_to_sines_appr[27] = NT(300) / NT(661);
    angle_to_sines_appr[28] = NT(8) / NT(17);
    angle_to_sines_appr[29] = NT(189) / NT(389);
    angle_to_sines_appr[30] = NT(451) / NT(901);
    angle_to_sines_appr[31] = NT(180) / NT(349);
    angle_to_sines_appr[32] = NT(28) / NT(53);
    angle_to_sines_appr[33] = NT(432) / NT(793);
    angle_to_sines_appr[34] = NT(161) / NT(289);
    angle_to_sines_appr[35] = NT(228) / NT(397);
    angle_to_sines_appr[36] = NT(504) / NT(865);
    angle_to_sines_appr[37] = NT(3) / NT(5);
    angle_to_sines_appr[38] = NT(580) / NT(941);
    angle_to_sines_appr[39] = NT(341) / NT(541);
    angle_to_sines_appr[40] = NT(88) / NT(137);
    angle_to_sines_appr[41] = NT(48) / NT(73);
    angle_to_sines_appr[42] = NT(65) / NT(97);
    angle_to_sines_appr[43] = NT(429) / NT(629);
    angle_to_sines_appr[44] = NT(555) / NT(797);
    angle_to_sines_appr[45] = NT(697) / NT(985);
    angle_to_sines_appr[46] = NT(572) / NT(797);
    angle_to_sines_appr[47] = NT(460) / NT(629);
    angle_to_sines_appr[48] = NT(72) / NT(97);
    angle_to_sines_appr[49] = NT(55) / NT(73);
    angle_to_sines_appr[50] = NT(105) / NT(137);
    angle_to_sines_appr[51] = NT(420) / NT(541);
    angle_to_sines_appr[52] = NT(741) / NT(941);
    angle_to_sines_appr[53] = NT(4) / NT(5);
    angle_to_sines_appr[54] = NT(703) / NT(865);
    angle_to_sines_appr[55] = NT(325) / NT(397);
    angle_to_sines_appr[56] = NT(240) / NT(289);
    angle_to_sines_appr[57] = NT(665) / NT(793);
    angle_to_sines_appr[58] = NT(45) / NT(53);
    angle_to_sines_appr[59] = NT(299) / NT(349);
    angle_to_sines_appr[60] = NT(780) / NT(901);
    angle_to_sines_appr[61] = NT(340) / NT(389);
    angle_to_sines_appr[62] = NT(15) / NT(17);
    angle_to_sines_appr[63] = NT(589) / NT(661);
    angle_to_sines_appr[64] = NT(80) / NT(89);
    angle_to_sines_appr[65] = NT(77) / NT(85);
    angle_to_sines_appr[66] = NT(520) / NT(569);
    angle_to_sines_appr[67] = NT(805) / NT(877);
    angle_to_sines_appr[68] = NT(925) / NT(997);
    angle_to_sines_appr[69] = NT(352) / NT(377);
    angle_to_sines_appr[70] = NT(140) / NT(149);
    angle_to_sines_appr[71] = NT(35) / NT(37);
    angle_to_sines_appr[72] = NT(176) / NT(185);
    angle_to_sines_appr[73] = NT(391) / NT(409);
    angle_to_sines_appr[74] = NT(24) / NT(25);
    angle_to_sines_appr[75] = NT(260) / NT(269);
    angle_to_sines_appr[76] = NT(828) / NT(853);
    angle_to_sines_appr[77] = NT(667) / NT(685);
    angle_to_sines_appr[78] = NT(357) / NT(365);
    angle_to_sines_appr[79] = NT(476) / NT(485);
    angle_to_sines_appr[80] = NT(525) / NT(533);
    angle_to_sines_appr[81] = NT(621) / NT(629);
    angle_to_sines_appr[82] = NT(920) / NT(929);
    angle_to_sines_appr[83] = NT(255) / NT(257);
    angle_to_sines_appr[84] = NT(180) / NT(181);
    angle_to_sines_appr[85] = NT(264) / NT(265);
    angle_to_sines_appr[86] = NT(420) / NT(421);
    angle_to_sines_appr[87] = NT(760) / NT(761);
    angle_to_sines_appr[88] = NT(1624) / NT(1625);
    angle_to_sines_appr[89] = NT(6612) / NT(6613);
    angle_to_sines_appr[90] = NT(1);
  }

  void rotate(Point &p,NT angle)
  {
    int tranc_angle = int(angle.to_double() * rad_to_deg);
    NT x = p.x() * angle_to_sines_appr[90 - tranc_angle] -
           p.y() * angle_to_sines_appr[tranc_angle],
       y = p.x() * angle_to_sines_appr[tranc_angle] +
           p.y() * angle_to_sines_appr[90 - tranc_angle];

    p = Point(x,y);
  }

  void get_intersecting_points(std::list<SAVED_OBJECT> &result_list,
                               Segment inp_s,NT unit_squere)
  {
    Segment s((inp_s.source().y() < inp_s.target().y()) ? inp_s.source() :
            inp_s.target(),(inp_s.source().y() < inp_s.target().y()) ?
            inp_s.target() : inp_s.source());
    NT x1 = s.source().x(),y1 = s.source().y(),
            x2 = s.target().x(),y2 = s.target().y();
    Point ms1,ms2,ms3,ms4,ms5,ms6,
        tmp_ms1,tmp_ms2,tmp_ms3,tmp_ms4,tmp_ms5,tmp_ms6;// minkowski sum points
    Point op_ms1,op_ms2,op_ms3,op_ms4,op_ms5,op_ms6;// optimal min sums points


    // determine minkowski sum points
    if(x1 < x2) {
      // we use unit_squere instead of unit_squere / 2 in order
      //to find tangency points which are not supported by kd-tree
      ms1 = Point(x1 - 0.6 * unit_squere,y1 - 0.6 * unit_squere);
      ms2 = Point(x1 - 0.6 * unit_squere,y1 + 0.6 * unit_squere);
      ms3 = Point(x1 + 0.6 * unit_squere,y1 - 0.6 * unit_squere);
      ms4 = Point(x2 + 0.6 * unit_squere,y2 - 0.6 * unit_squere);
      ms5 = Point(x2 + 0.6 * unit_squere,y2 + 0.6 * unit_squere);
      ms6 = Point(x2 - 0.6 * unit_squere,y2 + 0.6 * unit_squere);
    } else {
      // we use unit_squere instead of unit_squere / 2 in order
      //to find tangency points which are not supported by kd-tree
      ms1 = Point(x1 + 0.6 * unit_squere,y1 - 0.6 * unit_squere);
      ms2 = Point(x1 - 0.6 * unit_squere,y1 - 0.6 * unit_squere);
      ms3 = Point(x1 + 0.6 * unit_squere,y1 + 0.6 * unit_squere);
      ms4 = Point(x2 + 0.6 * unit_squere,y2 + 0.6 * unit_squere);
      ms5 = Point(x2 - 0.6 * unit_squere,y2 + 0.6 * unit_squere);
      ms6 = Point(x2 - 0.6 * unit_squere,y2 - 0.6 * unit_squere);
    }

    Point p1,p2;
    std::list<my_point<NT,SAVED_OBJECT> > res;



    // find appropriate kd-tree
    NT size = -1,tmp_size,min_x,min_y,max_x,max_y;

    list<pair<kd_tree *,NT> >::iterator iter,right_iter;

    for(iter = kd_trees_list.begin();iter != kd_trees_list.end();++iter) {
      tmp_ms1 = ms1;
      tmp_ms2 = ms2;
      tmp_ms3 = ms3;
      tmp_ms4 = ms4;
      tmp_ms5 = ms5;
      tmp_ms6 = ms6;
      rotate(tmp_ms1,iter->second);
      rotate(tmp_ms2,iter->second);
      rotate(tmp_ms3,iter->second);
      rotate(tmp_ms4,iter->second);
      rotate(tmp_ms5,iter->second);
      rotate(tmp_ms6,iter->second);

      min_x = min(tmp_ms1.x(),tmp_ms2.x(),tmp_ms3.x(),tmp_ms4.x(),
                  tmp_ms5.x(),tmp_ms6.x());
      min_y = min(tmp_ms1.y(),tmp_ms2.y(),tmp_ms3.y(),tmp_ms4.y(),
                  tmp_ms5.y(),tmp_ms6.y());
      max_x = max(tmp_ms1.x(),tmp_ms2.x(),tmp_ms3.x(),tmp_ms4.x(),
                  tmp_ms5.x(),tmp_ms6.x());
      max_y = max(tmp_ms1.y(),tmp_ms2.y(),tmp_ms3.y(),tmp_ms4.y(),
                  tmp_ms5.y(),tmp_ms6.y());
      tmp_size = abs((max_x - min_x)*(max_y - min_y));

      if(size == -1 || tmp_size < size) {
        size = tmp_size;
        right_iter = iter;
        op_ms1 = tmp_ms1;
        op_ms2 = tmp_ms2;
        op_ms3 = tmp_ms3;
        op_ms4 = tmp_ms4;
        op_ms5 = tmp_ms5;
        op_ms6 = tmp_ms6;
      }
    }

    // query
    p1 = Point(min(op_ms1.x(),op_ms2.x(),op_ms3.x(),op_ms4.x(),op_ms5.x(),
               op_ms6.x()),min(op_ms1.y(),op_ms2.y(),op_ms3.y(),op_ms4.y(),
               op_ms5.y(),op_ms6.y()));
    p2 = Point(max(op_ms1.x(),op_ms2.x(),op_ms3.x(),op_ms4.x(),op_ms5.x(),
               op_ms6.x()),max(op_ms1.y(),op_ms2.y(),op_ms3.y(),op_ms4.y(),
               op_ms5.y(),op_ms6.y()));
    my_point<NT,SAVED_OBJECT> point1(p1); 
    my_point<NT,SAVED_OBJECT> point2(p2);

    Box b(point1,point2,2);

    right_iter->first->search(std::back_inserter(res),b);
    // create result
    result_list.empty();
    for(typename std::list<my_point<NT,SAVED_OBJECT> >::iterator my_point_iter
	  = res.begin();my_point_iter != res.end();++my_point_iter) {
      result_list.push_back(my_point_iter->object);
    }
  }
};

template<class NT,class SAVED_OBJECT>
bool Multiple_kd_tree<NT,SAVED_OBJECT>::map_done(false);

template<class NT,class SAVED_OBJECT>
  map<const int,NT> Multiple_kd_tree<NT,SAVED_OBJECT>::angle_to_sines_appr;

#endif // CGAL_SR_KD_2_H
