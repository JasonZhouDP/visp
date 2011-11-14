/****************************************************************************
 *
 * $Id$
 *
 * This file is part of the ViSP software.
 * Copyright (C) 2005 - 2011 by INRIA. All rights reserved.
 * 
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * ("GPL") version 2 as published by the Free Software Foundation.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact INRIA about acquiring a ViSP Professional 
 * Edition License.
 *
 * See http://www.irisa.fr/lagadic/visp/visp.html for more information.
 * 
 * This software was developed at:
 * INRIA Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * http://www.irisa.fr/lagadic
 *
 * If you have questions regarding the use of this file, please contact
 * INRIA at visp@inria.fr
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 * Description:
 * Moving edges.
 *
 * Authors:
 * Andrew Comport
 *
 *****************************************************************************/

/*!
  \file vpMeTracker.cpp
  \brief Contains abstract elements for a Distance to Feature type feature.
*/



#include <visp/vpMeTracker.h>
#include <visp/vpDisplay.h>
#include <visp/vpColor.h>

#include <visp/vpTrackingException.h>
#include <visp/vpDebug.h>
#include <algorithm>
#define DEBUG_LEVEL1 0
#define DEBUG_LEVEL2 0

void
vpMeTracker::init()
{
  vpTracker::init()  ;
  p.resize(2) ;
  selectDisplay = vpMeSite::NONE ;
}

vpMeTracker::vpMeTracker()
{
  init();
  me = NULL ;
  display_point = false ;
  nGoodElement = 0;
  query_range = 0;
  init_range = 1;
}

vpMeTracker::vpMeTracker(const vpMeTracker& meTracker):vpTracker(meTracker)
{
  init();
  
  me = meTracker.me;
  list = meTracker.list;
  nGoodElement = meTracker.nGoodElement;
  query_range = meTracker.query_range;
  init_range = meTracker.init_range;
  display_point = meTracker.display_point;
  selectDisplay = meTracker.selectDisplay;
}

vpMeTracker::~vpMeTracker()
{
  list.clear();
}

vpMeTracker&
vpMeTracker::operator = (vpMeTracker& p)
{
  list = p.list;
  me = p.me;
  selectDisplay = p.selectDisplay ;

  return *this;
}

static bool isSuppressZero(const vpMeSite& P){
  return (P.suppress == 0);
}

unsigned int
vpMeTracker::numberOfSignal()
{
  unsigned int number_signal=0;

  // Loop through all the points tracked from the contour
  number_signal = static_cast<unsigned int>(std::count_if(list.begin(), list.end(), isSuppressZero));
  return number_signal;
}

unsigned int
vpMeTracker::totalNumberOfSignal()
{
  return list.size();

}

int
vpMeTracker::outOfImage(int i, int j, int half, int rows, int cols)
{
  return (! ((i> half+2) &&
	     (i< rows -(half+2)) &&
	     (j>half+2) &&
	     (j<cols-(half+2)))
	  ) ;
}

int
vpMeTracker::outOfImage(vpImagePoint iP, int half, int rows, int cols)
{
  int i = vpMath::round(iP.get_i());
  int j = vpMath::round(iP.get_j());
  return (! ((i> half+2) &&
	     (i< rows -(half+2)) &&
	     (j>half+2) &&
	     (j<cols-(half+2)))
	  ) ;
}


/*!
  Virtual function that is called by lower classes vpMeEllipse, vpMeLine 
  and vpMeNurbs.
*/
void
vpMeTracker::initTracking(const vpImage<unsigned char>& I)
{
  // Must set range to 0
  unsigned int range_tmp = me->range;
  me->range=init_range;

  nGoodElement=0;

  int d = 0;
  vpImagePoint ip1, ip2;
  
  // Loop through list of sites to track
  for(std::list<vpMeSite>::iterator it=list.begin(); it!=list.end(); ++it){
    vpMeSite refp = *it;//current reference pixel

    d++ ;
    // If element hasn't been suppressed
    if(refp.suppress==0)
    {
      try {
        refp.track(I,me,false);
      }
      catch(...)
      {
        // EM verifier quel signal est de sortie !!!
        vpERROR_TRACE("Error caught") ;
        throw ;
      }
      if(refp.suppress==0) nGoodElement++;
    }


    if(DEBUG_LEVEL2)
    {
      double a,b ;
      a = refp.i_1 - refp.i ;
      b = refp.j_1 - refp.j ;
      if(refp.suppress==0) {
        ip1.set_i( refp.i );
        ip1.set_j( refp.j );
        ip2.set_i( refp.i+a );
        ip2.set_j( refp.j+b );
        vpDisplay::displayArrow(I, ip1, ip2, vpColor::green) ;
      }
    }

    *it = refp;
  }

  /*
  if (res != OK)
  {
    std::cout<< "In vpMeTracker::initTracking(): " ;
    switch (res)
    {
    case  ERR_TRACKING:
      std::cout << "vpMeTracker::initTracking:Track return ERR_TRACKING " << std::endl ;
      break ;
    case fatalError:
      std::cout << "vpMeTracker::initTracking:Track return fatalError" << std::endl ;
      break ;
    default:
      std::cout << "vpMeTracker::initTracking:Track return error " << res << std::endl ;
    }
    return res ;
  }
  */

  me->range=range_tmp;
}


void
vpMeTracker::track(const vpImage<unsigned char>& I)
{
  if (list.empty())
  {
    if (DEBUG_LEVEL1)
      vpERROR_TRACE("Error Tracking: only %d "
     "pixels when entered the function ",list.size()) ;
    throw(vpTrackingException(vpTrackingException::notEnoughPointError,
			      "too few pixel to track")) ;

  }

  vpImagePoint ip1, ip2;
  nGoodElement=0;
  //  int d =0;
  // Loop through list of sites to track
  for(std::list<vpMeSite>::iterator it=list.begin(); it!=list.end(); ++it){
    vpMeSite s = *it;//current reference pixel

    //    d++ ;
    // If element hasn't been suppressed
    if(s.suppress==0)
    {

      try{
	//	vpERROR_TRACE("%d",d ) ;
	//	vpERROR_TRACE("range %d",me->range) ;
	 s.track(I,me,true);
      }
      catch(vpTrackingException)
      {
	vpERROR_TRACE("catch exception ") ;
	s.suppress=2 ;
      }

      if(s.suppress != 2)
      {
	nGoodElement++;

	if(DEBUG_LEVEL2)
	{
	  double a,b ;
	  a = s.i_1 - s.i ;
	  b = s.j_1 - s.j ;
	  if(s.suppress==0) {
	    ip1.set_i( s.i );
	    ip1.set_j( s.j );
	    ip2.set_i( s.i+a*5 );
	    ip2.set_j( s.j+b*5 );
	    vpDisplay::displayArrow(I, ip1, ip2, vpColor::green) ;
	  }
	}

      }
      *it = s;
    }
  }
}

/*!
  Displays the status of a moving edge site, a sample point:
  - in red, the sample point is considered as valid, while
  - in white, the sample point is suppressed due to a bad constrast,
  - in blue, the sample point is suppressed due to a bad likelihood ratio,
  - in green, the sample point is an outlier.
*/
void
vpMeTracker::display(const vpImage<unsigned char>& I)
{
  if (DEBUG_LEVEL1)
  {
    std::cout <<"begin vpMeTracker::displayList() " << std::endl ;
    std::cout<<" There are "<<list.size()<< " sites in the list " << std::endl ;
  }
  vpImagePoint ip;

  for(std::list<vpMeSite>::const_iterator it=list.begin(); it!=list.end(); ++it){
    vpMeSite p = *it;

    if(p.suppress == 1) {
      ip.set_i( p.i );
      ip.set_j( p.j);
      vpDisplay::displayCross(I, ip, 2, vpColor::white) ; // Contrast
    }
    else if(p.suppress == 2) {
      ip.set_i( p.i );
      ip.set_j( p.j);
      vpDisplay::displayCross(I, ip, 2,vpColor::blue) ; // Threshold
    }
    else if(p.suppress == 3) {
      ip.set_i( p.i );
      ip.set_j( p.j);
      vpDisplay::displayCross(I, ip, 3, vpColor::green) ; // M-estimator
    }
    else if(p.suppress == 0) {
      ip.set_i( p.i );
      ip.set_j( p.j);
      vpDisplay::displayCross(I, ip, 2, vpColor::red) ; // OK
    }
  }
}


void
vpMeTracker::display(const vpImage<unsigned char>& I,vpColVector &w, unsigned int &index_w)
{
  for(std::list<vpMeSite>::iterator it=list.begin(); it!=list.end(); ++it){
    vpMeSite P = *it;

    if(P.suppress == 0)
    {
      P.weight = w[index_w];
      index_w++;
    }

    *it = P;
  }
  display(I);
}

#undef DEBUG_LEVEL1
#undef DEBUG_LEVEL2


