//*************************************************************************
//Copyright (c) 2020 IHEP                                                  
//Copyright (c) 2021 DESY                                                  
//This program is free software; you can redistribute it and/or modify     
//it under the terms of the GNU General Public License                     
//Author: chao li, li.chao@desy.de                                         
//*************************************************************************
#ifndef RESONATOR_H
#define RESONATOR_H

#include "Global.h"
#include <vector>
#include <complex>

// resonator is used as cavities


using namespace std;
using std::vector;
using std::complex;

class Resonator
{


public:
    Resonator();
    ~Resonator();
    

    int    resHarm;                       // normalized by the ringHarmoics * f0
    int    resType;
    double resShuntImpRs = 0.E0;
    double resQualityQ0  = 0.E0;
    double resQualityQL  = 0.E0;
    double resCouplingBeta = 0.E0;       
    double resDetuneFre    = 0.E0;                  // Hz
    double resFre          = 0.E0;                        // Hz, 
    double resDeTunePsi    = 0.E0;                  // de-tune angle psi, Bill (7.23) // got from Atan2(x,y) in the range of (-PI,PI)                    
    double tF =0.E0;
    int    resCold;
    double resVolAbsReq;
    double resPhaseReq;
    int rfResExciteIntability=1;
    int rfResCavVolFB=1;

    complex<double> resCavVolReq=(0,0);     // it is the target value -- take into the self-beam loading voltage into account.   
    complex<double> resGenVol=(0,0);        // in the cos  convention used in the code, the real part represents the momentum change
    complex<double> vbAccum=(0,0);          // used in the tracking, record transient  cavity voltage
    complex<double> vbAccum0=(0,0);          // used in the tracking,record transient cavity voltage in omegarf frame 
    complex<double> vbAccumRFFrame=(0,0);          // used in the tracking,record transient cavity voltage in omegarf frame    
    complex<double> resGenVolFB=(0.0,0);    // ref. PRAB 24, 104401 2021 Eq. (9). 

    //  resCavVol=(0,0) is the target vaule or designed value 
    //  vbAccum=(0,0)   is the beam induce votage vector.-- tracking with certain filling pater 10 damping times, and the average of last term is set as vbAccum=(0,0) 
    //  resGenVol=(0,0) is the generate votage---  it meets resGenVol  =  resCavVol - vbAccum; 
    //  later during the particle tracking, the obtaiend resGenVol and the transiant beam induced voltage are used is simulation.  

    vector<vector<complex<double> > > resCavVolOffsetPastTurns;
    vector<complex<double> > resCavVolOffsetAver;
    
            
private:

};




#endif
