//------------------------------------------------------------------------------
//  rootlogon.C: a sample ROOT logon macro allowing use of ROOT script 
//               compiler in Mu2e environment. The name of this macro file
//               is defined by the .rootrc file
//
// Jul 08 2014 P.Murat
//------------------------------------------------------------------------------
{
                                // the line below tells rootcling where to look 
				// for include files

  gInterpreter->AddIncludePath(Form("%s/otsdaq-mu2e-tracker"     ,gSystem->Getenv("SPACK_ENV" )));
  gInterpreter->AddIncludePath(Form("%s/mu2e-pcie-utils"         ,gSystem->Getenv("SPACK_ENV" )));
  gInterpreter->AddIncludePath(Form("%s/include/root"            ,gSystem->Getenv("SPACK_VIEW")));
  gInterpreter->AddIncludePath(Form("%s/include/frontends"       ,gSystem->Getenv("SPACK_VIEW")));
  gInterpreter->AddIncludePath(Form("%s/include"                 ,gSystem->Getenv("SPACK_VIEW")));

  gInterpreter->ExecuteMacro("daq.C");

  gStyle->SetOptStat(1111111);
  gStyle->SetOptFit(11111111);
  gStyle->SetFitFormat("7.6g");
  gStyle->SetStatFormat("7.6g");

}
