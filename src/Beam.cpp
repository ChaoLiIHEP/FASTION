#include "Beam.h"
#include "LongImpSingalBunch.h"
#include <fstream>
#include <stdlib.h> 
#include <iostream>
#include "Faddeeva.h"
#include <time.h>
#include <string>
#include <cmath>
#include <gsl/gsl_fft_complex.h>
#include <complex>
#include "FIRFeedBack.h"




using namespace std;
using std::vector;
using std::complex;


Beam::Beam()
{

}

Beam::~Beam()
{
    
}

void Beam::Initial(Train &train, LatticeInterActionPoint &latticeInterActionPoint , ReadInputSettings &inputParameter)
{
    int totBunchNum = inputParameter.totBunchNumber;
    beamVec.resize(totBunchNum);
    harmonics = inputParameter.harmonics;
    printInterval=inputParameter.printInterval;
    ionLossBoundary = inputParameter.ionLossBoundary;

    bunchInfoOneTurn.resize(totBunchNum);
    for(int i=0;i<totBunchNum;i++)
    {
        bunchInfoOneTurn[i].resize(15);
    }
    
    vector<int> bunchGapIndexTemp;
    bunchGapIndexTemp.resize(totBunchNum);
    
    
    actionJxMax   = 0.E0;
    actionJyMax   = 0.E0;
    bunchSizeXMax = 0.E0;
    bunchSizeYMax = 0.E0;
 
    synchRadDampTime.resize(3);
  // in the unit of number of truns for synchRadDampTime setting.
  
    for(int i=0;i<synchRadDampTime.size();i++)
    {
        synchRadDampTime[i] = inputParameter.synchRadDampTime[i];
    }



    for(int i=0;i<totBunchNum;i++)
    {
        beamVec[i].Initial(latticeInterActionPoint, inputParameter);
        beamVec[i].DistriGenerator(latticeInterActionPoint,i);

    }



    int counter=0;
    for(int i=0;i<train.trainNumber;i++)
    {
        for(int j=0;j<train.bunchNumberPerTrain[i];j++)
        {
            bunchGapIndexTemp[counter] = train.trainStart[i] + j;
//            cout<<counter<<"    "<< bunchGapIndexTemp[counter]<<endl;
            counter++; 
        }
    }    
    


    for(int i=0;i<totBunchNum-1;i++)
    {
        beamVec[i].bunchGap = bunchGapIndexTemp[i+1]-bunchGapIndexTemp[i];
    }
    beamVec[totBunchNum-1].bunchGap = harmonics - bunchGapIndexTemp[totBunchNum-1];
    
//    for(int i=0;i<totBunchNum;i++)
//    {
//        cout<<beamVec[i].bunchGap<<"    "<<i<<" "<<__LINE__<<endl;
//    }
//    getchar();

}   





void Beam::Run(Train &train, LatticeInterActionPoint &latticeInterActionPoint,ReadInputSettings &inputParameter)
{
    int nTurns = inputParameter.nTurns;
    int totBunchNum = inputParameter.totBunchNumber;
    int calSettings=inputParameter.calSettings;
    int synRadDampingFlag = inputParameter.synRadDampingFlag;
    int intevalofTurnsIonDataPrint = inputParameter.intevalofTurnsIonDataPrint;
    int printInterval = inputParameter.printInterval;
    int fIRBunchByBunchFeedbackFlag = inputParameter.fIRBunchByBunchFeedbackFlag;
    
    double beamEnergy = inputParameter.electronBeamEnergy;
    
    ofstream fout ("result.dat",ios::out);


//   calSettings =1 :  beam_ion_interaction      --- multi-bunch-multi-turn
//   calSettings =2 :  single_bunch_Longi_imped  --- signal-bunch-signal-turn  ---board band
//   calSettings =3 :  single_bunch_Trans_imped  --- signal-bunch-signal-turn  ---board band



// preapre the data for bunch-by-bunch system --------------- 

    FIRFeedBack firFeedBack;
    if(fIRBunchByBunchFeedbackFlag)
    {        
        firFeedBack.Initial(totBunchNum,beamEnergy);
    }
// ***************end ********************************



    switch(calSettings)
    {

        case 1 :    //   calSettings =1 :  beam_ion_interaction      --- multi-bunch-multi-turn
        {

            if(beamVec[0].macroEleNumPerBunch==1)
            {
                cout<<"Press Inter to go on the weak-strong ion-beam simulation"<<endl;
//                getchar();
                
                for(int i=0;i<nTurns;i++)   
                {

                    for(int k=0;k<inputParameter.numberofIonBeamInterPoint;k++)
                    {
                        WSBeamRMSCal(latticeInterActionPoint,k);
                    }
                    WSGetMaxBunchInfo();

                    for (int k=0;k<inputParameter.numberofIonBeamInterPoint;k++)
                    {
                        WSBeamIonEffectOneInteractionPoint(inputParameter,latticeInterActionPoint, i,  k,intevalofTurnsIonDataPrint);  
                        BeamTransferPerInteractionPointDueToLattice(latticeInterActionPoint,k); 
                    }

                    WSGetMaxBunchInfo();

                    if(synRadDampingFlag)
                    {
                         BeamSynRadDamping(synchRadDampTime,latticeInterActionPoint);
                    }
                    if(fIRBunchByBunchFeedbackFlag)
                    {
                        FIRBunchByBunchFeedback(firFeedBack,i);  
                    }
  
                    IonBeamDataPrintPerTurn(i,latticeInterActionPoint,fout);

                    cout<<i                                                         //1
                        <<" turns  "<<"  "                                          //2
                        <<latticeInterActionPoint.ionAccumuNumber[0]<<"     "       //3
                        <<sqrt(actionJxMax)/sqrt(beamVec[0].emittanceX)<<"    "     //4
                        <<sqrt(actionJyMax)/sqrt(beamVec[0].emittanceY)<<"  "       //5
                        <<bunchAverXMax/beamVec[0].rmsRx<<"   "                      //8
                        <<bunchAverYMax/beamVec[0].rmsRy<<"   "                      //9
                        <<beamVec[0].pxAver<<"  "                                       //10
                        <<beamVec[0].pyAver<<"  "                                       //11
                        <<beamVec[0].xAver<<"   "                                   //12
                        <<beamVec[0].yAver<<"   "<<endl;                            //13
//                        <<log10(sqrt(actionJxMax))<<"    "
//                        <<log10(sqrt(actionJyMax))
//                        <<endl;
                }
            }
            else   //strong-strong beam model;
            {
                cout<<"Press Inter to go on the quasi-strong-strong ion-beam simulation"<<endl;
                getchar();

                for(int i=0;i<nTurns;i++)   
                { 
                    
                    for(int j=0;j<totBunchNum;j++)
                    {
                        beamVec[j].RMSCal(latticeInterActionPoint,0);
                    }

                    SSGetMaxBunchInfo();
                    
                    SSBeamIonEffectOneTurn(latticeInterActionPoint, inputParameter,i,  intevalofTurnsIonDataPrint,  printInterval);
                    
                    SSGetMaxBunchInfo();
                    
                    if(synRadDampingFlag)
                    {
                         BeamSynRadDamping(synchRadDampTime,latticeInterActionPoint);
                    }
                    
                    if(fIRBunchByBunchFeedbackFlag)
                    {
                        FIRBunchByBunchFeedback(firFeedBack,i);  
                    }
                  
                    if(intevalofTurnsIonDataPrint && (i%intevalofTurnsIonDataPrint==0))
                    {
                        SSIonDataPrint(latticeInterActionPoint, i);
                        SSBunchDataPrint(beamVec[beamVec.size()-1],i);
                    }

                    IonBeamDataPrintPerTurn(i,latticeInterActionPoint,fout);


                    cout<<i<<" turns  "<<"  "
                        <<latticeInterActionPoint.ionAccumuNumber[0]<<"     "
                        <<emitXMax<<"  "
                        <<emitYMax<<"  "
                        <<effectivEemitXMax<<"  "
                        <<effectivEemitYMax<<"  "
                        <<bunchSizeXMax<<"    "
                        <<bunchSizeYMax<<"    "
                        <<bunchEffectiveSizeXMax<<"   "
                        <<bunchEffectiveSizeYMax<<"   "
                        <<beamVec[beamVec.size()-1].macroEleNumPerBunch<<"  "
                        <<endl;
                }
            }

            break;
        }
        case 2 :        //   calSettings =2 :  single_bunch_Longi_imped  --- signal-bunch-signal-turn
        {
            if(train.totBunchNum!=1)
            {

                cerr<<"The total buncher number is largaer than 1 in the single_bunch_Longi_imped calcualtion"<<endl;
                exit(0);
            }
            
            LongImpSingalBunch longImpSingalBunch;
            longImpSingalBunch.Initial();
            
            for(int i=0;i<nTurns;i++)   
            { 
		        BeamTransferPerTurnDueToLattice(latticeInterActionPoint);
                SingleBunchLongiImpedInterAction(longImpSingalBunch,i,fout);
		        BeamTransferPerTurnDueToLattice(latticeInterActionPoint);
            }
            break;
        }

        case 3 :
        {
            if(train.totBunchNum!=1)
            {
                cerr<<"The total bucher nummber is largaer than 1"<<endl;
                cerr<<" in the single_bunch_trans_imped calcualtion "<<endl;
                exit(0);
            }
            
            for(int i=0;i<nTurns;i++)
            { 
                cout<<"still ongoing..."<<endl;
            }
            break;
        }
        default:
            cerr<<"Do Nothing...  "<<endl;
    }



    fout.close();
}


void Beam::BeamTransferPerInteractionPointDueToLattice(LatticeInterActionPoint &latticeInterActionPoint, int k)
{
    for(int j=0;j<beamVec.size();j++)
    {
        beamVec[j].BunchTransferDueToLattice(latticeInterActionPoint,k);
    }

}

void Beam::BeamSynRadDamping(vector<double> &synchRadDampTime,LatticeInterActionPoint &latticeInterActionPoint)
{
	for(int j=0;j<beamVec.size();j++)
	{	
		beamVec[j].BunchSynRadDamping(synchRadDampTime,latticeInterActionPoint);		
	}
}

void Beam::WSBeamRMSCal(LatticeInterActionPoint &latticeInterActionPoint, int k)
{
    for(int j=0;j<beamVec.size();j++)
    {
        beamVec[j].WSRMSCal(latticeInterActionPoint,k);
    }

}


//void Beam::SSBeamRMSCal()
//{
//   for(int j=0;j<beamVec.size();j++)
//    {
//        beamVec[j].SSRMSCal(latticeInterActionPoint,k);
//    }
//}




void Beam::BeamTransferPerTurnDueToLattice(LatticeInterActionPoint &latticeInterActionPoint)
{
    for(int k=0;k<latticeInterActionPoint.numberOfInteraction;k++)
    {
        for(int j=0;j<beamVec.size();j++)
        {
            beamVec[j].BunchTransferDueToLattice(latticeInterActionPoint,k); 
        }
    }
}




void Beam::WSBeamIonEffectOneInteractionPoint(ReadInputSettings &inputParameter,LatticeInterActionPoint &latticeInterActionPoint, int nTurns, int k, int intevalofTurnsIonDataPrint)
{
    int totBunchNum = beamVec.size();
    
    double corssSectionEI = inputParameter.corssSectionEI;
    


    for(int j=0;j<totBunchNum;j++)
    {

        latticeInterActionPoint.ionLineDensity[k] = corssSectionEI * latticeInterActionPoint.vacuumPressure[k]
                                /latticeInterActionPoint.temperature[k]/Boltzmann * beamVec[j].electronNumPerBunch;


        latticeInterActionPoint.ionNumber[k] = latticeInterActionPoint.ionLineDensity[k] * 
                                               latticeInterActionPoint.interactionLength[k];

        beamVec[j].WSRMSCal(latticeInterActionPoint,k);

        latticeInterActionPoint.IonGenerator(beamVec[j].rmsRx,beamVec[j].rmsRy,beamVec[j].xAver,beamVec[j].yAver,k);


        latticeInterActionPoint.WSIonsUpdate(bunchEffectiveSizeXMax,bunchEffectiveSizeYMax,beamVec[j].xAver,beamVec[j].yAver,k);

        latticeInterActionPoint.IonRMSCal(k);

        beamVec[j].WSIonBunchInteraction(latticeInterActionPoint,k);

        beamVec[j].BunchTransferDueToIon(latticeInterActionPoint,k);
        
        if(intevalofTurnsIonDataPrint && (nTurns%intevalofTurnsIonDataPrint==0) && k==0)
        {
            WSIonDataPrint(latticeInterActionPoint, nTurns, k);
        }


        latticeInterActionPoint.IonTransferDueToBunch(beamVec[j].bunchGap,k,bunchEffectiveSizeXMax,bunchEffectiveSizeYMax,beamVec[j].macroEleNumPerBunch);





        if(nTurns%printInterval==0 && k==0)
        {
            bunchInfoOneTurn[j][0] = nTurns;									    //1
            bunchInfoOneTurn[j][1] = j;												//2
            bunchInfoOneTurn[j][2] = latticeInterActionPoint.ionAccumuNumber[k];   //3
            bunchInfoOneTurn[j][3] = beamVec[j].rmsRx;								//4
            bunchInfoOneTurn[j][4] = beamVec[j].rmsRy;								//5
//            bunchInfoOneTurn[j][5] = sqrt(beamVec[j].actionJx)/sqrt(beamVec[0].emittanceX);	//6
//            bunchInfoOneTurn[j][6] = sqrt(beamVec[j].actionJy)/sqrt(beamVec[0].emittanceY);	//7
//            bunchInfoOneTurn[j][7] = beamVec[j].xAver/beamVec[j].rmsRx;				//8
//            bunchInfoOneTurn[j][8] = beamVec[j].yAver/beamVec[j].rmsRy;				//9 
            bunchInfoOneTurn[j][5] = sqrt(beamVec[j].actionJx);	                    //6
            bunchInfoOneTurn[j][6] = sqrt(beamVec[j].actionJy);	                    //7
            bunchInfoOneTurn[j][7] = beamVec[j].xAver;              				//8
            bunchInfoOneTurn[j][8] = beamVec[j].yAver;                              //9    
            bunchInfoOneTurn[j][9] = beamVec[j].pxAver;								//10
            bunchInfoOneTurn[j][10] = beamVec[j].pyAver;							//11
            bunchInfoOneTurn[j][11] = latticeInterActionPoint.ionAccumuAverX[k];	//12
            bunchInfoOneTurn[j][12] = latticeInterActionPoint.ionAccumuAverY[k];	//13
            bunchInfoOneTurn[j][13] = latticeInterActionPoint.ionAccumuRMSX[k];     //14
            bunchInfoOneTurn[j][14] = latticeInterActionPoint.ionAccumuRMSY[k];		//15
        }           

    }
//    getchar();
}



void Beam::SSBeamIonEffectOneTurn( LatticeInterActionPoint &latticeInterActionPoint, ReadInputSettings &inputParameter, int nTurns, int intevalofTurnsIonDataPrint, int printInterval)
{

    double corssSectionEI = inputParameter.corssSectionEI;
    int totBunchNum = inputParameter.totBunchNumber;

    int counter=nTurns*latticeInterActionPoint.numberOfInteraction*totBunchNum;
    
    for(int k=0;k<latticeInterActionPoint.numberOfInteraction;k++)  
    {
        for(int j=0;j<totBunchNum;j++)
        {

            latticeInterActionPoint.ionLineDensity[k] = corssSectionEI * latticeInterActionPoint.vacuumPressure[k]
                                    /latticeInterActionPoint.temperature[k]/Boltzmann * beamVec[j].electronNumPerBunch;
            //(1) get the ion beam line density at the kth interaction points due to the jth beam bunch

            latticeInterActionPoint.ionNumber[k] = latticeInterActionPoint.ionLineDensity[k] * 
                                                   latticeInterActionPoint.interactionLength[k];
            //(2) get the ion beam number to be generated at the kth interaction point due to the jth beam bunch

            beamVec[j].RMSCal(latticeInterActionPoint,k);

            //(3) get the rms size of jth beam at kth interaction point, rmsRx rmsRy
            latticeInterActionPoint.IonGenerator(beamVec[j].rmsRx,beamVec[j].rmsRy,beamVec[j].xAver,beamVec[j].yAver,k);
            //(4) generate the ions randomly (Gausion distribution truncated n*rms) at the kth interaction point 


            latticeInterActionPoint.SSIonsUpdate(bunchEffectiveSizeXMax, bunchEffectiveSizeYMax,beamVec[j].xAver,beamVec[j].yAver,k);
            
            latticeInterActionPoint.IonRMSCal(k);
            //(5) get the accumulated ions information at the kth point (new ions plus old ions)
            //    get the rms vaues of the accumulated ion distribution.

            beamVec[j].SSIonBunchInteraction(latticeInterActionPoint,k);
            //(6) get the interactionforces between accumulated ions and the jth beam bunch at the kth interaction point 

            if(intevalofTurnsIonDataPrint && (nTurns%intevalofTurnsIonDataPrint==0) && j==beamVec.size()-1)
            {
                SSBunchDataPrint(beamVec[j],nTurns);
            }

            if(printInterval && (nTurns%printInterval==0) && j==beamVec.size()-1)
            {
                SSIonDataPrint1(latticeInterActionPoint, nTurns, k);
            }


            beamVec[j].BunchTransferDueToIon(latticeInterActionPoint,k); 
            //(7) kick the jth electron beam bunch due to accumulated ions at the kth interactoin point 

            latticeInterActionPoint.IonTransferDueToBunch(beamVec[j].bunchGap,k,bunchEffectiveSizeXMax, bunchEffectiveSizeYMax, beamVec[j].macroEleNumPerBunch);         
            //(8) kick the accumulated ions due to jth electron bunch 
            
            
            
            
            
            
            beamVec[j].BunchTransferDueToLattice(latticeInterActionPoint,k); 
            //(9) electron beam transfer to next interaction point 


            if(nTurns%printInterval==0 && k==0)
            {
                bunchInfoOneTurn[j][0] = nTurns;									    //1
                bunchInfoOneTurn[j][1] = j;												//2
                bunchInfoOneTurn[j][2] = latticeInterActionPoint.ionAccumuNumber[k];   //3
                bunchInfoOneTurn[j][3] = beamVec[j].rmsEffectiveRx;						//4
                bunchInfoOneTurn[j][4] = beamVec[j].rmsEffectiveRy;						//5
                bunchInfoOneTurn[j][5] = beamVec[j].rmsEffectiveRingEmitX;              //6
                bunchInfoOneTurn[j][6] = beamVec[j].rmsEffectiveRingEmitX;              //7
                bunchInfoOneTurn[j][7] = beamVec[j].xAver;								//8
                bunchInfoOneTurn[j][8] = beamVec[j].yAver;								//9    
                bunchInfoOneTurn[j][9] = beamVec[j].pxAver;								//10
                bunchInfoOneTurn[j][10] = beamVec[j].pyAver;							//11
                bunchInfoOneTurn[j][11] = latticeInterActionPoint.ionAccumuAverX[k];	//12
                bunchInfoOneTurn[j][12] = latticeInterActionPoint.ionAccumuAverY[k];	//13
                bunchInfoOneTurn[j][13] = latticeInterActionPoint.ionAccumuRMSX[k];     //14
                bunchInfoOneTurn[j][14] = latticeInterActionPoint.ionAccumuRMSY[k];		//15
            }
            counter++;
        }
    }
}







void Beam::SingleBunchLongiImpedInterAction(LongImpSingalBunch &longImpSingalBunch, int nTurns, ofstream &fout)
{

    cout<<"do the beam_impedance in fre domain"<<endl;
    cout<<"only the beamVec[0] is used in calculation" <<endl;

    beamVec[0].InitialBeamCurDenZProf(); 

    for(int i=0;i<longImpSingalBunch.longImpedR.size();i++)
    {
//        beamVec[0];
    }


}

void Beam::SSIonDataPrint(LatticeInterActionPoint &latticeInterActionPoint, int nTurns)
{
    for(int k=0;k<latticeInterActionPoint.numberOfInteraction;k++)
    {
//        string fname;
//        fname = "SSionAccumudis_"+std::to_string(nTurns)+"_"+to_string(k)+".dat";
        
        char fname[256];
        sprintf(fname, "SSionAccumudis_%d_%d.dat", nTurns, k);
        
        ofstream fout(fname,ios::out);
        for(int i=0;i<latticeInterActionPoint.ionAccumuNumber[k];i++)
        {
            fout<<i                     <<"      "
                <<latticeInterActionPoint.ionAccumuPositionX[k][i]   <<"      "       
                <<latticeInterActionPoint.ionAccumuPositionY[k][i]   <<"      "       
                <<latticeInterActionPoint.ionAccumuVelocityX[k][i]   <<"      "       
                <<latticeInterActionPoint.ionAccumuVelocityY[k][i]   <<"      "       
                <<latticeInterActionPoint.ionAccumuFx[k][i]   <<"      "       
                <<latticeInterActionPoint.ionAccumuFy[k][i]   <<"      "       
                <<endl;
        }
        
        fout.close();
    }


    vector<double> histogrameX;
    vector<double> histogrameY;
    
    double x_max = 0;
    double x_min = 0;
    
    double y_max = 0;
    double y_min = 0;
    int bins = 100;
    histogrameX.resize(bins);
    histogrameY.resize(bins);
    
    
    double binSizeX =0;
    double binSizeY =0;
    

    double temp;
    int indexX;
    int indexY;
    
    for(int k=0;k<latticeInterActionPoint.numberOfInteraction;k++)
    {

        x_max =  ionLossBoundary*bunchEffectiveSizeXMax;
        x_min = -ionLossBoundary*bunchEffectiveSizeXMax;
        y_max =  ionLossBoundary*bunchEffectiveSizeYMax;
        y_min = -ionLossBoundary*bunchEffectiveSizeYMax;

//        x_max = latticeInterActionPoint.ionAccumuAverX[k]  + latticeInterActionPoint.pipeAperatureX;
//        x_min = latticeInterActionPoint.ionAccumuAverX[k]  - latticeInterActionPoint.pipeAperatureX;
//        y_max = latticeInterActionPoint.ionAccumuAverY[k]  + latticeInterActionPoint.pipeAperatureY;
//        y_min = latticeInterActionPoint.ionAccumuAverY[k]  - latticeInterActionPoint.pipeAperatureY;



        binSizeX  = (x_max - x_min)/bins;
        binSizeY  = (y_max - y_min)/bins;

        for(int i=0;i<latticeInterActionPoint.ionAccumuNumber[k];i++)
        {
            temp   = (latticeInterActionPoint.ionAccumuPositionX[k][i] - x_min)/binSizeX;
            indexX = floor(temp);
            
            temp   = (latticeInterActionPoint.ionAccumuPositionY[k][i] - y_min)/binSizeY;
            indexY = floor(temp);
            if(indexX<0 || indexY<0 || indexX>bins || indexY>bins) continue;
            
            histogrameX[indexX] = histogrameX[indexX] +1;
            histogrameY[indexY] = histogrameY[indexY] +1;
        }
        
        
        
//        string fname;
//        fname = "SSionAccumuHis_"+to_string(nTurns)+"_"+to_string(k)+".dat";
        
        char fname[256];
        sprintf(fname, "SSionAccumuHis_%d_%d.dat", nTurns, k);
        
        
        ofstream foutHis(fname,ios::out);

        for(int i=0;i<bins;i++)
        {
            foutHis<<i<<"    "
                   <<(x_min+i*binSizeX)/bunchEffectiveSizeXMax<<"  "
                   <<(y_min+i*binSizeY)/bunchEffectiveSizeYMax<<"  "
                   <<(x_min+i*binSizeX)<<"  "
                   <<(y_min+i*binSizeY)<<"  "
                   <<histogrameX[i]<<"  "
                   <<histogrameY[i]<<"  "
                   <<endl;
        }
     
        foutHis.close();
    }

}




void Beam::SSIonDataPrint1(LatticeInterActionPoint &latticeInterActionPoint, int nTurns, int k)
{



//    string fname;
//    fname = "SSionAccumudis_"+to_string(nTurns)+"_"+to_string(k)+".dat";
    
    char fname[256];
    sprintf(fname, "SSionAccumudis_%d_%d.dat", nTurns, k);
    
    ofstream fout(fname,ios::out);
    for(int i=0;i<latticeInterActionPoint.ionAccumuNumber[k];i++)
    {
        fout<<i                     <<"      "
            <<latticeInterActionPoint.ionAccumuPositionX[k][i]   <<"      "       
            <<latticeInterActionPoint.ionAccumuPositionY[k][i]   <<"      "       
            <<latticeInterActionPoint.ionAccumuVelocityX[k][i]   <<"      "       
            <<latticeInterActionPoint.ionAccumuVelocityY[k][i]   <<"      "       
            <<latticeInterActionPoint.ionAccumuFx[k][i]   <<"      "       
            <<latticeInterActionPoint.ionAccumuFy[k][i]   <<"      "       
            <<endl;
    }
    
    fout.close();



    vector<double> histogrameX;
    vector<double> histogrameY;
    
    double x_max = 0;
    double x_min = 0;
    
    double y_max = 0;
    double y_min = 0;
    int bins = 100;
    histogrameX.resize(bins);
    histogrameY.resize(bins);
    
    
    double binSizeX =0;
    double binSizeY =0;
    

    double temp;
    int indexX;
    int indexY;
    


    x_max =  ionLossBoundary*bunchEffectiveSizeXMax;
    x_min = -ionLossBoundary*bunchEffectiveSizeXMax;
    y_max =  ionLossBoundary*bunchEffectiveSizeYMax;
    y_min = -ionLossBoundary*bunchEffectiveSizeYMax;

//  x_max = latticeInterActionPoint.ionAccumuAverX[k]  + latticeInterActionPoint.pipeAperatureX;
//  x_min = latticeInterActionPoint.ionAccumuAverX[k]  - latticeInterActionPoint.pipeAperatureX;
//  y_max = latticeInterActionPoint.ionAccumuAverY[k]  + latticeInterActionPoint.pipeAperatureY;
//  y_min = latticeInterActionPoint.ionAccumuAverY[k]  - latticeInterActionPoint.pipeAperatureY;



    binSizeX  = (x_max - x_min)/bins;
    binSizeY  = (y_max - y_min)/bins;

    for(int i=0;i<latticeInterActionPoint.ionAccumuNumber[k];i++)
    {
        temp   = (latticeInterActionPoint.ionAccumuPositionX[k][i] - x_min)/binSizeX;
        indexX = floor(temp);
        
        temp   = (latticeInterActionPoint.ionAccumuPositionY[k][i] - y_min)/binSizeY;
        indexY = floor(temp);
        if(indexX<0 || indexY<0 || indexX>bins || indexY>bins) continue;
        
        histogrameX[indexX] = histogrameX[indexX] +1;
        histogrameY[indexY] = histogrameY[indexY] +1;
    }
    
    



//    fname = "SSionAccumuHis_"+to_string(nTurns)+"_"+to_string(k)+".dat";
    sprintf(fname, "SSionAccumuHis_%d_%d.dat", nTurns, k);
    ofstream foutHis(fname,ios::out);

    for(int i=0;i<bins;i++)
    {
        foutHis<<i<<"    "
               <<(x_min+i*binSizeX)/bunchEffectiveSizeXMax<<"  "
               <<(y_min+i*binSizeY)/bunchEffectiveSizeYMax<<"  "
               <<(x_min+i*binSizeX)<<"  "
               <<(y_min+i*binSizeY)<<"  "
               <<histogrameX[i]<<"  "
               <<histogrameY[i]<<"  "
               <<endl;
    }
 
    foutHis.close();

}




void Beam::SSBunchDataPrint( Bunch &bunch, int nTurn) 
{


//    string fname;
//    fname  = "SSbeamdis_"+to_string(nTurn)+".dat";
    
    char fname[256];
    sprintf(fname, "SSbeamdis_%d.dat", nTurn);
    
    ofstream fout(fname);

    for(int i=0;i<bunch.macroEleNumPerBunch;i++)
    {

        fout<<i                     <<"      "
            <<bunch.ePositionX[i]   <<"      "       
            <<bunch.ePositionY[i]   <<"      "       
            <<bunch.eMomentumX[i]   <<"      "       
            <<bunch.eMomentumY[i]   <<"      "       
            <<bunch.eFx[i]          <<"      "       
            <<bunch.eFy[i]          <<"      "       
            <<endl;
    }
    fout.close();
}



void Beam::IonBeamDataPrintPerTurn(int turns,  LatticeInterActionPoint &latticeInterActionPoint, ofstream &fout)
{
    

    
    if(beamVec[0].macroEleNumPerBunch==1)
    {
        for (int k=0;k<latticeInterActionPoint.numberOfInteraction;k++)
        {
            fout<<turns<<"  "																					//1
                <<k<<"	"																						//2
                <<latticeInterActionPoint.ionAccumuNumber[k]*latticeInterActionPoint.macroIonCharge[k]<<" " 	//3
                <<log10(sqrt(actionJxMax))<<"    "																//4
                <<log10(sqrt(actionJyMax))<<"    "																//5
                <<log10(sqrt(beamVec[0].emittanceX))<<"	"   // constant representing the beam size in x 		//6
                <<log10(sqrt(beamVec[0].emittanceY))<<"	"   // constant representing the beam size in y 		//7
                <<log10(bunchAverXMax)<<"    "																	//8
                <<log10(bunchAverYMax)<<"    "																	//9
                <<beamVec[beamVec.size()-1].xAver<<"   "														//10
                <<beamVec[beamVec.size()-1].yAver<<"   "														//11
                <<beamVec[0].xAver<<"   "														                //12
                <<beamVec[0].yAver<<"   "														                //13
                <<latticeInterActionPoint.ionAccumuAverX[k]<<"  "												//14
                <<latticeInterActionPoint.ionAccumuAverY[k]<<"  "												//15
                <<latticeInterActionPoint.ionAccumuRMSX[k]<<"  "												//16
                <<latticeInterActionPoint.ionAccumuRMSY[k]<<"  "												//17
                <<endl;
        }
    }
    else
    {
        for (int k=0;k<latticeInterActionPoint.numberOfInteraction;k++)
        {
            fout<<turns<<"  "																					//1
                <<k<<"	"																						//2
                <<latticeInterActionPoint.ionAccumuNumber[k]*latticeInterActionPoint.macroIonCharge[k]<<" "     //3
                <<emitXMax<<"    "													                            //4
                <<emitYMax<<"    "												                                //5
                <<effectivEemitXMax<<"	"                                                                       //6
                <<effectivEemitYMax<<"	"                                                                       //7
                <<bunchSizeXMax<<"  "                                                                           //8  
                <<bunchSizeYMax<<"  "                                                                           //9  
                <<bunchEffectiveSizeXMax<<"     "                                                               //10
                <<bunchEffectiveSizeYMax<<"     "                                                               //11
                <<bunchAverXMax<<"  "                                                                           //12
                <<bunchAverYMax<<"  "                                                                           //13
                <<latticeInterActionPoint.ionAccumuAverX[k]<<"  "												//14
                <<latticeInterActionPoint.ionAccumuAverY[k]<<"  "												//15
                <<latticeInterActionPoint.ionAccumuRMSX[k]<<"  "												//16
                <<latticeInterActionPoint.ionAccumuRMSY[k]<<"  "												//17
                <<endl;
        }
    }


    // print the data on certain turns  at the first interaction point  
    if(turns%printInterval==0)
    {
//        string fname;
//        fname = "Aver_"+to_string(turns)+".dat";
        
        char fname[256];
        sprintf(fname, "Aver_%d.dat", turns);
        
        ofstream fAverOut(fname,ios::out);
        


        double dataX[2*harmonics];
        double dataY[2*harmonics];
        
        for(int i=0;i<harmonics;i++)
        {
            dataX[2*i  ] = 0.E0; 
            dataX[2*i+1] = 0.E0;
            dataY[2*i  ] = 0.E0;
            dataY[2*i+1] = 0.E0;
        }
        
        int index=0;
        
        for(int i=0;i<bunchInfoOneTurn.size();i++)
        {
            int j=0;
            while(j<beamVec[i].bunchGap)
            {
                if(j==0)
                {
                    dataX[2*index    ] = bunchInfoOneTurn[i][7];
                    dataX[2*index + 1] = 0.E0;
                    dataY[2*index    ] = bunchInfoOneTurn[i][8];
                    dataY[2*index + 1] = 0.E0;
                }
                else
                {
                    dataX[2*index    ] = 0.E0;
                    dataX[2*index + 1] = 0.E0;
                    dataY[2*index    ] = 0.E0;
                    dataY[2*index + 1] = 0.E0;
                }
                
                index = index + 1;
                j=j+1;
            }
        }

//        for(int i=0;i<harmonics;i++)
//        {
//            cout<<i<<"  "<<dataX[2*i]<<"  "<<dataX[2*i+1]<<endl;
//        }        
//        
//        getchar();
//    
        
//        for(int i=0;i<bunchInfoOneTurn.size();i++)
//        {
//            dataX[2*i  ] = bunchInfoOneTurn[i][10];	 
//            dataX[2*i+1] = 0.E0;
//            dataY[2*i  ] = bunchInfoOneTurn[i][11];	 
//            dataY[2*i+1] = 0.E0;
//        }




        gsl_fft_complex_wavetable * wavetable;
        gsl_fft_complex_workspace * workspace;

        wavetable = gsl_fft_complex_wavetable_alloc (harmonics);
        workspace = gsl_fft_complex_workspace_alloc (harmonics);

        gsl_fft_complex_forward (dataX, 1, harmonics, wavetable, workspace);   // FFT for bunch[i].xAver in one turn;
                
        vector<complex<double> > beamCentoidXFFT;
        beamCentoidXFFT.resize(harmonics);
        
        for(int i=0;i<harmonics;i++)
        {
            beamCentoidXFFT[i] = complex< double > ( dataX[2*i], dataX[2*i+1]  );
        }
        
        
        vector<complex<double> > beamCentoidYFFT;
        beamCentoidYFFT.resize(harmonics);
        
        gsl_fft_complex_forward (dataY, 1, harmonics, wavetable, workspace);  // FFT for bunch[i].yAver in one turn;
        
        for(int i=0;i<harmonics;i++)
        {
            beamCentoidYFFT[i] = complex< double > ( dataY[2*i], dataY[2*i+1]  );
        }
       
        gsl_fft_complex_workspace_free (workspace);
        gsl_fft_complex_wavetable_free (wavetable);
        
        // the FFT calculated with size as harmonics number,  
        // to print only the bunch number  
        for(int i=0;i<bunchInfoOneTurn.size();i++)
        {
            for(int j=0;j<bunchInfoOneTurn[0].size();j++)
            {
                fAverOut<< bunchInfoOneTurn[i][j]<<"    ";
            }
            fAverOut<<log10(abs(beamCentoidXFFT[i]))<<"    "<<log10(abs(beamCentoidYFFT[i]));
            fAverOut<<endl;
        }
        fAverOut.close();
    }
}





void Beam::WSIonDataPrint(LatticeInterActionPoint &latticeInterActionPoint, int nTurns, int k)
{
//    string fname;
//    fname = "WSionAccumudis_"+to_string(nTurns)+"_"+to_string(k)+".dat";
    
    char fname[256];
    sprintf(fname, "WSionAccumudis_%d_%d.dat", nTurns, k);
    
    ofstream fout(fname);

    for(int i=0;i<latticeInterActionPoint.ionAccumuNumber[k];i++)
    {
        fout<<i                     <<"      "
            <<latticeInterActionPoint.ionAccumuPositionX[k][i]   <<"      "       
            <<latticeInterActionPoint.ionAccumuPositionY[k][i]   <<"      "       
            <<latticeInterActionPoint.ionAccumuVelocityX[k][i]   <<"      "       
            <<latticeInterActionPoint.ionAccumuVelocityY[k][i]   <<"      "       
            <<latticeInterActionPoint.ionAccumuFx[k][i]   <<"      "       
            <<latticeInterActionPoint.ionAccumuFy[k][i]   <<"      "       
            <<endl;
    }
    
    fout.close();


 
//    fname = "WSionCenter_"+to_string(nTurns)+"_"+to_string(k)+".dat";
    sprintf(fname, "WSionCenter_%d_%d.dat", nTurns, k);
    ofstream fionCenter(fname);


    fionCenter<<latticeInterActionPoint.ionAccumuAverX[k]<<"      "       
              <<latticeInterActionPoint.ionAccumuAverY[k]<<"      "
              <<latticeInterActionPoint.ionAccumuRMSX[k]<<"      "
              <<latticeInterActionPoint.ionAccumuRMSY[k]<<"      "
              <<endl;

    fout.close();
    
    


    vector<double> histogrameX;
    vector<double> histogrameY;
    
    double x_max = 0;
    double x_min = 0;
    
    double y_max = 0;
    double y_min = 0;
    int bins = 100;
    histogrameX.resize(bins);
    histogrameY.resize(bins);
    
    
    double binSizeX =0;
    double binSizeY =0;
    

    double temp;
    int indexX;
    int indexY;
    
    for(int k=0;k<latticeInterActionPoint.numberOfInteraction;k++)
    {

        if(ionLossBoundary*bunchEffectiveSizeXMax>latticeInterActionPoint.pipeAperatureX)
        {
            x_max =  latticeInterActionPoint.pipeAperatureX;
            x_min = -latticeInterActionPoint.pipeAperatureX;
        }
        else
        {
            x_max =  ionLossBoundary*bunchEffectiveSizeXMax;
            x_min = -ionLossBoundary*bunchEffectiveSizeXMax;
        }
        
        if(ionLossBoundary*bunchEffectiveSizeYMax>latticeInterActionPoint.pipeAperatureY)
        {
            y_max =  latticeInterActionPoint.pipeAperatureY;
            y_min = -latticeInterActionPoint.pipeAperatureY;
        }
        else
        {
            y_max =  ionLossBoundary*bunchEffectiveSizeYMax;
            y_min = -ionLossBoundary*bunchEffectiveSizeYMax;
        }




//        x_max = latticeInterActionPoint.ionAccumuAverX[k]  + latticeInterActionPoint.pipeAperatureX;
//        x_min = latticeInterActionPoint.ionAccumuAverX[k]  - latticeInterActionPoint.pipeAperatureX;
//        y_max = latticeInterActionPoint.ionAccumuAverY[k]  + latticeInterActionPoint.pipeAperatureY;
//        y_min = latticeInterActionPoint.ionAccumuAverY[k]  - latticeInterActionPoint.pipeAperatureY;



        binSizeX  = (x_max - x_min)/bins;
        binSizeY  = (y_max - y_min)/bins;

        for(int i=0;i<latticeInterActionPoint.ionAccumuNumber[k];i++)
        {
            temp   = (latticeInterActionPoint.ionAccumuPositionX[k][i] - x_min)/binSizeX;
            indexX = floor(temp);
            
            temp   = (latticeInterActionPoint.ionAccumuPositionY[k][i] - y_min)/binSizeY;
            indexY = floor(temp);
            if(indexX<0 || indexY<0 || indexX>bins || indexY>bins) continue;
            
            histogrameX[indexX] = histogrameX[indexX] +1;
            histogrameY[indexY] = histogrameY[indexY] +1;
        }
        

//        fname = "WSionAccumuHis_"+to_string(nTurns)+"_"+to_string(k)+".dat";

        sprintf(fname, "WSionAccumuHis_%d_%d.dat", nTurns, k);
        ofstream foutHis(fname,ios::out);

        for(int i=0;i<bins;i++)
        {
            foutHis<<i<<"    "
                   <<(x_min+i*binSizeX)/bunchEffectiveSizeXMax<<"  "
                   <<(y_min+i*binSizeY)/bunchEffectiveSizeYMax<<"  "
                   <<(x_min+i*binSizeX)<<"  "
                   <<(y_min+i*binSizeY)<<"  "
                   <<histogrameX[i]<<"  "
                   <<histogrameY[i]<<"  "
                   <<endl;
        }

        foutHis.close();
    }
    
    
    
    


}

void Beam::SSGetMaxBunchInfo()
{
    double totBunchNum = beamVec.size();
    double tempEmitx;
    double tempEmity;
    double tempEffectiveEmitx;
    double tempEffectiveEmity;
    double tempRMSSizeX;
    double tempRMSSizeY;
    double tempEffectiveRMSSizeX;
    double tempEffectiveRMSSizeY;
    double tempSizeX;
    double tempSizeY;
    
    double tempAverX;
    double tempAverY;
    
    tempEmitx   = beamVec[0].rmsEmitX;
    tempEmity   = beamVec[0].rmsEmitY;

    tempEffectiveEmitx   = beamVec[0].rmsEffectiveRingEmitX;
    tempEffectiveEmity   = beamVec[0].rmsEffectiveRingEmitY;

    tempRMSSizeX   = beamVec[0].rmsRx;
    tempRMSSizeY   = beamVec[0].rmsRy;

    tempEffectiveRMSSizeX = beamVec[0].rmsEffectiveRx;
    tempEffectiveRMSSizeY = beamVec[0].rmsEffectiveRy;


    tempAverX   = beamVec[0].xAver;
    tempAverY   = beamVec[0].yAver;
    
    
    for(int i=1;i<totBunchNum;i++)
    {
        if(beamVec[i].rmsEmitX>tempEmitx)
        {
            tempEmitx = beamVec[i].rmsEmitX;
        }
        
        if(beamVec[i].rmsEmitY>tempEmity)
        {
            tempEmity = beamVec[i].rmsEmitY;
        }


        if(beamVec[i].rmsEffectiveRingEmitX>tempEffectiveEmitx)
        {
            tempEffectiveEmitx = beamVec[i].rmsEffectiveRingEmitX;
        }
        
        if(beamVec[i].rmsEffectiveRingEmitY>tempEffectiveEmity)
        {
            tempEffectiveEmity = beamVec[i].rmsEffectiveRingEmitY;
        }
        

        
        if(beamVec[i].rmsRx>tempRMSSizeX)
        {
            tempRMSSizeX = beamVec[i].rmsRx;
        }
        
        if(beamVec[i].rmsRy>tempRMSSizeY)
        {
            tempRMSSizeY = beamVec[i].rmsRy;
        }
        
        
         
        if(beamVec[i].rmsEffectiveRx>tempEffectiveRMSSizeX)
        {
            tempEffectiveRMSSizeX = beamVec[i].rmsEffectiveRx;
        }
        
        if(beamVec[i].rmsEffectiveRy>tempEffectiveRMSSizeY)
        {
            tempEffectiveRMSSizeY = beamVec[i].rmsEffectiveRy;
        }


        if(beamVec[i].xAver>tempAverX)
        {
            tempAverX = beamVec[i].xAver;
        }
        
        if(beamVec[i].yAver>tempAverY)
        {
            tempAverY = beamVec[i].yAver;
        }

    }
    
    emitXMax = tempEmitx;
    emitYMax = tempEmity;
    effectivEemitXMax = tempEffectiveEmitx;
    effectivEemitYMax = tempEffectiveEmity;
    
    bunchEffectiveSizeXMax = tempEffectiveRMSSizeX;
    bunchEffectiveSizeYMax = tempEffectiveRMSSizeY;
    
    
    bunchSizeXMax = tempRMSSizeX;
    bunchSizeYMax = tempRMSSizeY;
    
    bunchAverXMax  = tempAverX;
    bunchAverYMax  = tempAverY;
    
}

void Beam::WSGetMaxBunchInfo()
{

    double totBunchNum = beamVec.size();
    double tempJx;
    double tempJy;

    double tempMaxAverX;
    double tempMaxAverY;
    
    tempJx      = beamVec[0].actionJx;
    tempJy      = beamVec[0].actionJy;

    tempMaxAverX = beamVec[0].xAver;
    tempMaxAverY = beamVec[0].yAver;

    
    for(int i=1;i<totBunchNum;i++)
    {
        if(beamVec[i].actionJx>tempJx)
        {
            tempJx = beamVec[i].actionJx;
        }
        
        if(beamVec[i].actionJy>tempJy)
        {
            tempJy = beamVec[i].actionJy;
        }

        
        if(beamVec[i].xAver>tempMaxAverX)
        {
            tempMaxAverX = beamVec[i].xAver;
        }
        
        if(beamVec[i].yAver>tempMaxAverY)
        {
            tempMaxAverY = beamVec[i].yAver;
        }
    }
    
    
    actionJyMax = tempJy;
    actionJxMax = tempJx;

    bunchAverXMax = tempMaxAverX;
    bunchAverYMax = tempMaxAverY;
    
    bunchEffectiveSizeXMax = beamVec[0].rmsRx + bunchAverXMax;
    bunchEffectiveSizeYMax = beamVec[0].rmsRy + bunchAverYMax;

}


void Beam::FIRBunchByBunchFeedback(FIRFeedBack &firFeedBack,int nTurns)
{

    //y[0] = \sum_0^{N} a_k x[-k] 
    //Ref. Nakamura's paper spring 8 notation here used is the same with Nakamura's paper

    double eta  = firFeedBack.kickerDisp;
    double etaP = firFeedBack.kickerDispP;

    vector<double> tranAngleKicky;
    vector<double> tranAngleKickx;
    vector<double> energyKickU;

    tranAngleKicky.resize(beamVec.size());
    tranAngleKickx.resize(beamVec.size());
    energyKickU.resize(beamVec.size());
    
    int nTaps =  firFeedBack.firCoeffx.size();

    int tapsIndex = nTurns%nTaps;

    vector< vector<double> > posxDataTemp;
    vector< vector<double> > posyDataTemp;
    vector< vector<double> > poszDataTemp;
    
    


    for(int i=0;i<beamVec.size();i++)
    {
        firFeedBack.posxData[tapsIndex][i] = beamVec[i].xAver;
        firFeedBack.posyData[tapsIndex][i] = beamVec[i].yAver;
        firFeedBack.poszData[tapsIndex][i] = beamVec[i].zAver;
    }


//    
//    double sumTempX=0;
//    double sumTempY=0;
//    double sumTempZ=0;
//    
//    for(int i=0;i<beamVec.size();i++)
//    {
//        sumTempX=0;
//        sumTempY=0;
//        sumTempZ=0;
//        for(int k=0;k<nTaps;k++)
//        {
//            sumTempX = sumTempX + firFeedBack.posxData[k][i];
//            sumTempY = sumTempY + firFeedBack.posyData[k][i]; 
//            sumTempZ = sumTempZ + firFeedBack.poszData[k][i];  
//        }
//        
//        firFeedBack.gainX[i] = 1.0/(sumTempX/nTaps);
//        firFeedBack.gainY[i] = 1.0/(sumTempY/nTaps);
//        firFeedBack.gainZ[i] = 1.0/(sumTempZ/nTaps);
//    
//        cout<<firFeedBack.gainX[i]<<"   "
//            <<firFeedBack.gainY[i]<<"   "
//            <<firFeedBack.gainZ[i]<<"   "
//            <<sumTempX<<"   "
//            <<sumTempY<<"   "
//            <<sumTempZ<<"   "
//            <<endl;   
//    }




    for(int i=0;i<beamVec.size();i++)
    {
        tranAngleKickx[i] = 0.E0;
        tranAngleKicky[i] = 0.E0;
        energyKickU[i]    = 0.E0;
    }

    for(int i=0;i<beamVec.size();i++)
    {
        for(int k=0;k<nTaps;k++)
        {
            tranAngleKickx[i] +=  firFeedBack.firCoeffx[nTaps-k-1] * firFeedBack.posxData[k][i];
            tranAngleKicky[i] +=  firFeedBack.firCoeffy[nTaps-k-1] * firFeedBack.posyData[k][i];
            energyKickU[i]    +=  firFeedBack.firCoeffz[nTaps-k-1] * firFeedBack.poszData[k][i]; 
        }
        
        if(firFeedBack.kickStrengthKx > firFeedBack.fIRBunchByBunchFeedbackKickLimit)
        {
            tranAngleKickx[i] =  tranAngleKickx[i] * firFeedBack.fIRBunchByBunchFeedbackKickLimit * firFeedBack.gain[0];
        }
        else
        {
            tranAngleKickx[i] =  tranAngleKickx[i] * firFeedBack.kickStrengthKx * firFeedBack.gain[0];
        }
    
        if(firFeedBack.kickStrengthKy > firFeedBack.fIRBunchByBunchFeedbackKickLimit)
        {
            tranAngleKicky[i] =  tranAngleKicky[i] * firFeedBack.fIRBunchByBunchFeedbackKickLimit * firFeedBack.gain[1];
        }
        else
        {
            tranAngleKicky[i] =  tranAngleKicky[i] * firFeedBack.kickStrengthKy * firFeedBack.gain[1];
        }

            //longitudinal feed_back     
//        if(firFeedBack.kickStrengthKU > firFeedBack.fIRBunchByBunchFeedbackKickLimit)
//        {
//            tranAngleKicky[i] =  tranAngleKicky[i] * firFeedBack.fIRBunchByBunchFeedbackKickLimit * firFeedBack.gain[1];
//        }
//        else
//        {
//            tranAngleKicky[i] =  tranAngleKicky[i] * firFeedBack.kickStrengthKy * firFeedBack.gain[1];
//        }
    


    }



    for(int i=0;i<beamVec.size();i++)
    {
        for(int j=0;j<beamVec[i].macroEleNumPerBunch;j++)
        {
            beamVec[i].eMomentumX[j] = beamVec[i].eMomentumX[j] + tranAngleKickx[i];
            beamVec[i].eMomentumY[j] = beamVec[i].eMomentumY[j] + tranAngleKicky[i];
//          beamVec[i].eMomentumZ[j] = beamVec[i].eMomentumZ[j] + energyKickU[i];
        }

    }




}
