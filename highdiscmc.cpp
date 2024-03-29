/*
   Copyright 2011 Julian Schutsch

   This file is part of TRTGarf

   TRTGarf is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   TRTGarf is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with TRTGarf.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <stdio.h>
#include <float.h>
#include "Signals/Signal.hpp"
#include "Signals/Discriminate.hpp"
#include "Signals/SignalPairSet.hpp"
#include "TRTElectronics/ASDBLRDiscriminator.hpp"
#include "tools.hpp"
#include <TApplication.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TMultiGraph.h>
#include <TRandom.h>
#include <TRandom3.h>

// #include "boost/filesystem.hpp"

Signals::SignalPairSet signalpairs;

const std::string TopDir="/home/cluster/kn2a1/lu32cud/";

std::vector<std::string> DestVar;
std::vector<std::string> SourceVar;

const double SignalBins   = 64;
const double SignalBinDiv = 256;

void LoadSignals(std::string ASignalIncPath,std::string ASignalLis,std::vector<unsigned int> ASections)
{
  for(unsigned int i=0;i<ASections.size();++i)
  {
    Signals::Signal signal(3.125/SignalBinDiv);
    Signals::Signal dest(3.125/SignalBinDiv);
    signal.ReadSignalFromHSpicelis(ASignalLis,SourceVar,ASections[i]);
    signal.Shift(-signal.GetSignalBin(0));
    dest.ReadSignalFromHSpicelis(ASignalLis,DestVar,ASections[i]);
    dest.Flatten(SignalBinDiv);
    dest.Shift(-dest.GetSignalBin(0));
    Signals::Discriminate dest2(3.125/SignalBinDiv,0.4);
    signalpairs.AddSignalPair(signal,dest>>dest2);
    signalpairs.SetCurrentExtra1(dest);
  }
}

int main(int argc,char * argv[])
{
  try
  {
    TApplication application("app",&argc,argv);
    InitRoot();
    TRandom3 * random=new TRandom3();

    SourceVar.push_back("trdisc");
    DestVar.push_back("ternary");
    DestVar.push_back("");
    DestVar.push_back("");

    std::vector<unsigned int> sections;
    for(unsigned int i=0;i<6;++i) sections.push_back(i);
    
/*    LoadSignals(TopDir+"sigs/ScaledSig/",
                TopDir+"sigs/ASDSigScaled.lis",
                sections);*/
    LoadSignals(TopDir+"sigs/TrackSignals5/",
                TopDir+"sigs/ASDSclWtrk_tr.lis",
                sections);
                
    double Bestd=1e30; TCanvas * canvas=new TCanvas();
    std::cout<<"RUN"<<std::endl;
    
    TRT::ASDBLRDiscriminator discriminator(signalpairs.GetSignalBinTime());

    for(unsigned int mc=0;mc<10000000;++mc)
    {
      discriminator.FThreshold         = 1E-2*random->Rndm();
      discriminator.FDCHysteresis      = 1E-2*random->Rndm();
      discriminator.FACHysteresis      = 0*0.03*random->Rndm();
      discriminator.FACHysteresisDecay = 5/3.125*SignalBinDiv;
      discriminator.FDelay             = 5+2.5*random->Rndm();

      signalpairs.SetPlotScales(1,0.028,0.03,0.03);
      signalpairs.SetPlotColors(kBlack,kCyan,kBlue,kGreen);

      if(signalpairs.IterReset())
      {
        do
        {
          signalpairs.SetCurrentResult(signalpairs.GetCurrentInput()>>discriminator);
        } while(signalpairs.IterNext());
        double d=signalpairs.CompareAll();
        if(d<Bestd)
        {
          Bestd=d;
          std::cout<<"*** "<<Bestd<<" ***"<<std::endl;
          discriminator.SaveCoefficientsToFile("HighDisc"+IntToStr(mc)+".dat");
          discriminator.Debug();
          signalpairs.PlotAll(canvas);
          std::cout<<"Update"<<std::endl;
        }
      }
    }
    std::cout<<"Done"<<std::endl;
    application.Run(kTRUE);
  }
  catch(const char * e)
  {
    std::cout<<e<<std::endl;
    throw;
  }

  return 0;
}
