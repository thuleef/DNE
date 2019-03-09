/************************************************/
/*  Extract Drainage Network from DEM           */
/*  Use Size Balanced Binary Search Tree        */
/*  Prgramed by BAI Rui in Tsinghua 2014.9      */
/*  Improved by LI JiaYe in Tsinghua 2017.8     */
/*  Cooperators LI TieJian, HUANG YueFei        */
/*  Email: lijiaye16@mails.tsinghua.edu.cn      */
/************************************************/

#include "DEM.h"
#include "Tif.h"

bool Run(TDEM &, TTif &);

//Main Program
void main(int argc, char *argv[])
{
	TDEM DEM;
	TTif Tif;

	//Check CommandLine Param
	if (argc<2)
	{
		cout<<"Wrong Number of Command Line Paramters: \n\n";
		cout<<"Please input FilePath, [ Optional Parameters ] !\n";
		cin.get();  return;
	}
	
	//Check Input DEM FilePath
	DEM.Param.DEMPath = argv[1];
	if (! PathFileExists(DEM.Param.DEMPath))
	{
		cout<<"Error: Input DEM File is not Exist!\n";   cin.get();   return;
	}
	else if (! PathMatchSpec(DEM.Param.DEMPath, "*.Tif" ))
	{
		cout<<"Error: Input File is not Txt or Tif File!\n";   cin.get();   return;
	}

	//Get Filename Without Extension
	DEM.Param.FileName=new(char[260]);   strcpy(DEM.Param.FileName, DEM.Param.DEMPath);
	DEM.Param.FileName=PathFindFileName(DEM.Param.FileName);   PathRemoveExtension(DEM.Param.FileName);

	//Get Out Path
	DEM.Param.OutPath=new(char[260]);   strcpy(DEM.Param.OutPath, DEM.Param.DEMPath);
	PathRemoveExtension(DEM.Param.OutPath);   PathAddBackslash(DEM.Param.OutPath);
	if (!PathFileExists(DEM.Param.OutPath))   CreateDirectory(DEM.Param.OutPath, NULL);

	//Set Custom Parameters or Default
	if (argc==6)
	{
		DEM.Param.CSSA=atof(argv[2]);			DEM.Param.CASL=atof(argv[3]);
		DEM.Param.Window=atof(argv[4]);			DEM.Param.Confidence=atof(argv[5]);
	}
	else  // Default Parameters
	{
		DEM.Param.CSSA=600;						DEM.Param.CASL=0.2;
		DEM.Param.Window=0.25;					DEM.Param.Confidence=0.05;
	}

	// Run Program
	if (!Run(DEM, Tif))
	{
		cout<<"Drainage Network Extraction Error!\n";   cin.get();   return;
	}
	cout<<"  Press any Key to Exit!\n";  cin.get();
};

bool Run(TDEM & DEM, TTif & Tif)
{
	//Welcome Information
	cout<<"\n\n\t\t<<  Large Scale DEM Drainage Netwrok Extraction Process  >>";
	cout<<"\n\n\t\t  Programed by Bai Rui and Li JiaYe in TsingHua University\n\n\n";
	
	//{0}Print Param Option
	cout<<"River Network Extraction with DEM:\n\n";
	cout<<"  InFile="<<DEM.Param.DEMPath<<"\n\n";
	cout<<"  Parameter: CSSA="<<DEM.Param.CSSA<<", CASL="<<DEM.Param.CASL<<", Window="<<DEM.Param.Window<<", Confidence="<<DEM.Param.Confidence<<"\n\n";
	DEM.Param.RunStart=CTime::GetCurrentTime();
	cout<<DEM.Param.RunStart.Format("  StartTime=%#Y/%#m/%#d %#H:%#M.\n\n");

	//{1}Read DEM
	cout<<"Read Input DEM and Create Dynamic Memory:\n\n";
	if (!Tif.DEM_ReadTif(DEM))   return false;

	//{2}Calculate D8 and UpArea
	cout<<"Sort DEM using BST and Get D8 Direction & Upstream Area:\n\n";
	if (!DEM.DEM_Sort())   return false;
	if (!DEM.DEM_Area())   return false;

	//{3}Detect the Channel Heads
	cout << "Find Channel Heads by Changed-Point Detection Method:\n\n";
	if (!DEM.DEM_ChannelHead())   return false;

	//{4}Output Channel Head Mark
	cout << "Output the Channel Head Mark Tif File:\n\n";
	DEM.Param.RunNow=CTime::GetCurrentTime();
	if (!Tif.DEM_PrintTif(DEM, "Mark"))		return false;
	DEM.Param.PrintSpan=CTime::GetCurrentTime()-DEM.Param.RunNow;
	cout<<"  Print Time="<<DEM.Param.PrintSpan.Format("%#H:%#M:%#S.\n\n");

	//{5}Output Used Time Totally
	DEM.Param.TotalSpan=CTime::GetCurrentTime()-DEM.Param.RunStart;
	cout<<"Total Used time is "<<DEM.Param.TotalSpan.Format("%#H:%#M:%#S.\n\n");
	return true;
};
