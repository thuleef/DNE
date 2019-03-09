#include "DEM.h"

const Direction TDEM::Add[9][2]={{0, 0}, {-1, 0}, {0, 1}, {1, 0}, {0, -1}, {-1, -1}, {-1, 1}, {1, 1}, {1, -1}};
const Direction TDEM::Inv[9]={-1, 3, 4, 1, 2, 7, 8, 5, 6};

bool SortSCell(const sSmlCell &sC1, const sSmlCell &sC2)
{
	return sC1.E < sC2.E;
}

TDEM::TDEM()
{
	AllArea=0;   Row=0;   Col=0;   Root=0;   Head=0;   Tail=0;
	Cell.push_back(sCell());
};

TDEM::~TDEM()
{

};

//--> 1.DEM_Sort.Tree Right Rotate
void TDEM::RightRotate(CellId & Tmp)
{
	CellId k=Cell[Tmp].L;   Cell[Tmp].L=Cell[k].R;
	Cell[k].R=Tmp;   Cell[k].Size=Cell[Tmp].Size;
	Cell[Tmp].Size=Cell[Cell[Tmp].L].Size+Cell[Cell[Tmp].R].Size+1;
	Tmp=k;
};

//--> 1.DEM_Sort.Tree Left Rotate
void TDEM::LeftRotate(CellId & Tmp)
{
	CellId k=Cell[Tmp].R;   Cell[Tmp].R=Cell[k].L;
	Cell[k].L=Tmp;   Cell[k].Size=Cell[Tmp].Size;
	Cell[Tmp].Size=Cell[Cell[Tmp].L].Size+Cell[Cell[Tmp].R].Size+1;
	Tmp=k;
};

//--> 1.DEM_Sort.Tree Maintain
void TDEM::Maintain(CellId & Tmp, bool Flag)
{
	if (Flag==false)
	{
		if ( Cell[Cell[Cell[Tmp].L].L].Size > Cell[Cell[Tmp].R].Size )
			RightRotate(Tmp);
		else if ( Cell[Cell[Cell[Tmp].L].R].Size > Cell[Cell[Tmp].R].Size )
		{
			LeftRotate( Cell[Tmp].L );   RightRotate(Tmp);
		}
		else return;
	}
	else
	{
		if ( Cell[Cell[Cell[Tmp].R].R].Size > Cell[Cell[Tmp].L].Size ) 
			LeftRotate(Tmp);
		else if ( Cell[Cell[Cell[Tmp].R].L].Size > Cell[Cell[Tmp].L].Size )
		{
			RightRotate( Cell[Tmp].R );   LeftRotate(Tmp);
		}
		else return;
	}
	if (Cell[Cell[Tmp].L].L!=0)		Maintain( Cell[Tmp].L, false);
	if (Cell[Cell[Tmp].R].R!=0)		Maintain( Cell[Tmp].R, true);
	Maintain(Tmp, false);	Maintain(Tmp, true);
};

//--> 1.DEM_Sort.Insert Node into Tree
void TDEM::InsertTree(CellId Now, CellId & Tmp)
{
	bool Flag;
	if (Tmp==0)
	{
		Cell[Now].Size=1; Cell[Now].L=0; Cell[Now].R=0; Tmp=Now;
	}
	else
	{
		Cell[Tmp].Size++;   Flag=(Cell[Now].E<Cell[Tmp].E);
		if (Flag==true)
			InsertTree(Now, Cell[Tmp].L);
		else
			InsertTree(Now, Cell[Tmp].R);
		Maintain(Tmp, !Flag);
	}
};

//--> 1.DEM_Sort.Delete Min Node from Tree
void TDEM::DeleteMin(CellId & Now, CellId & Tmp)
{
	Cell[Tmp].Size--;
	if (Cell[Tmp].L==0) 
	{
		Now=Tmp;   Tmp=Cell[Tmp].R;
	}
	else
		DeleteMin( Now, Cell[Tmp].L );
};

//--> 1.DEM_Sort.Insert Link node at Tail
void TDEM::InsertTail(CellId Now)
{
	if (Tail==0)
	{
		Tail=Now; Head=Now;   Cell[Head].R=0;
	}
	else
	{
		Cell[Tail].L=Now;   Cell[Now].R=Tail;
		Tail=Now;   Cell[Tail].L=0;
	}
};

//--> Get Geographical Coordinates by Row and Col Id
void TDEM::GetXY(RowCol R, RowCol C, sPoint & XY)
{
	XY.X=Param.GeoTiePoint[3]+(C-Param.GeoTiePoint[0]-0.5)*Param.GeoPixelScale[0];
	XY.Y=Param.GeoTiePoint[4]+(Param.GeoTiePoint[1]-R+0.5)*Param.GeoPixelScale[1];
};

//--> Get Geographical Unit Distance(1 Degree) by Row Id
void TDEM::GetDetXY(RowCol R, double &DetX, double &DetY)
{
	double	a=6378137.000, e=0.08181919131, pi=3.141592653, n, Latitude;
	if (Param.GeoModel==2)
	{
		if (R==0)   Latitude=0;
		else   Latitude=Param.GeoTiePoint[4]+(Param.GeoTiePoint[1]-R+0.5)*Param.GeoPixelScale[1];
		n=a/sqrt(1-e*e*sin(Latitude/180*pi)*sin(Latitude/180*pi));
		DetX=Param.GeoPixelScale[0]/180*pi*n;
		DetY=Param.GeoPixelScale[1]/180*pi*n*cos(Latitude/180*pi);
	}
	else
	{
		DetX=Param.GeoPixelScale[0];   DetY=Param.GeoPixelScale[1];
	}
};

//--> 1.DEM Sort by the Size Balanced Binary Search Tree Method
bool TDEM::DEM_Sort()
{
	CellId		Now, Count, Nodata;
	RowCol		i, j;
	sRowCol		TmpRC;
	Direction	k, InnerN;
	float		InnerSum;
	float		fx, fy;
	double		DetX, DetY;
	vector<sSmlCell> TmpCell;

/*1*/cout<<"  Scan Border Nodata...      ";   Param.RunNow=CTime::GetCurrentTime();
	for (i=0; i<=Row+1; i++)   //Initial Edge Into Width Search Queue
	{
		Inword.push_back(sRowCol(i, 0));   Map[i][0]=0;   Inword.push_back(sRowCol(i, Col+1));   Map[i][Col+1]=0;
	}
	for (j=1; j<=Col; j++)
	{
		Inword.push_back(sRowCol(0, j));   Map[0][j]=0;   Inword.push_back(sRowCol(Row+1, j));   Map[Row+1][j]=0;
	}
	Root=0;   Count=0;   Nodata=(Row+Col+2)*2;
	while(!Inword.empty())
	{
		TmpRC=Inword.front();   Inword.pop_front();   Now=Map[TmpRC.R][TmpRC.C];
		if (Now>0 && Cell[Now].Size==0)
		{
			Cell[Now].D=-1;   Cell[Now].G=-1;   Count++;   InsertTree(Now, Root);   //Outlet: D=-1; Boundary: G=-1
		}
		else if (Now==0)
		{
			for (k=1; k<=8; k++)
			{
				i=TmpRC.R+Add[k][0];   j=TmpRC.C+Add[k][1];
				if (i>0 && i<=Row && j>0 && j<=Col && Map[i][j]!=0)
				{
					if ( !(Map[i][j]>0 && Cell[Map[i][j]].Size>0))   Inword.push_back(sRowCol(i, j));
					if (Map[i][j]==-1)
					{
						Map[i][j]=0;   Nodata++;
					}
				}
			}
		}
		if ((UInt64)(Nodata-1)*100/((Row+2)*(Col+2)-AllArea)<(UInt64)Nodata*100/((Row+2)*(Col+2)-AllArea))
			cout<<"\b\b\b\b"<<setw(3)<<(UInt64)Nodata*100/((Row+2)*(Col+2)-AllArea)<<"%";
	}
	cout<<"  Insert BST Node "<<Count<<".\n\n";
	Inword.swap(deque<sRowCol>(0));
/*2*/cout<<"  Scan and Assign Inner Nodata...      ";
	for (Count=0, i=1; i<=Row; i++)
	{
		for (j=1; j<=Col; j++)
			if (Map[i][j]==-1)
			{
				InnerN=0;   InnerSum=0;
				for (k=1;k<=8;k++)
					if (Map[i+Add[k][0]][j+Add[k][1]]>0)
					{
						InnerN++;   InnerSum+=Cell[Map[i+Add[k][0]][j+Add[k][1]]].E;
					}
				if (InnerN>0)
				{
					Cell.push_back(sCell(i, j, InnerSum/InnerN));   Count++;   Map[i][j]=++AllArea;
				}
				else   Map[i][j]=0;
			}
		if ((UInt64)(i-1)*100/Row<(UInt64)i*100/Row)
			cout<<"\b\b\b\b"<<setw(3)<<(UInt64)i*100/Row<<"%";
	}
	cout<<"  Inner Nodata Cell "<<Count<<".\n\n";
/*3*/cout<<"  Sort DEM and Insert/Delete from BST...      ";
	Count=0; Tail=0; Head=0;
	while (Root!=0)
	{
		DeleteMin(Now, Root);   InsertTail(Now);
		Count++;   Cell[Now].Size=Count;
		TmpCell.clear();
		for (k = 1; k <= 8; k++)
		{
			i = Cell[Now].X + Add[k][0];   j = Cell[Now].Y + Add[k][1];
			if (Map[i][j]>0 && Cell[Map[i][j]].Size == 0)
			{
				TmpCell.push_back(sSmlCell(Map[i][j], k, Cell[Map[i][j]].E));
			}
		}
		sort(TmpCell.begin(), TmpCell.end(), SortSCell);
		for (k = 0; k < TmpCell.size(); k++)
		{
			Cell[TmpCell[k].ID].D = Inv[TmpCell[k].k];
			InsertTree(TmpCell[k].ID, Root);
		}
		i = Cell[Now].X; j = Cell[Now].Y;
		GetDetXY(Cell[Now].X, DetX, DetY);
		fx = float((Cell[Map[i + 1][j - 1]].E + Cell[Map[i + 1][j]].E + Cell[Map[i + 1][j + 1]].E - Cell[Map[i - 1][j - 1]].E - Cell[Map[i - 1][j]].E - Cell[Map[i - 1][j + 1]].E) / (6 * DetX));
		fy = float((Cell[Map[i - 1][j + 1]].E + Cell[Map[i][j + 1]].E + Cell[Map[i + 1][j + 1]].E - Cell[Map[i - 1][j - 1]].E - Cell[Map[i][j - 1]].E - Cell[Map[i + 1][j - 1]].E) / (6 * DetY));
		Cell[Now].S = sqrt(fx * fx + fy * fy);   //Get Local Slope of each Cell
		if ((UInt64)(Count-1)*100/AllArea<(UInt64)Count*100/AllArea)
			cout<<"\b\b\b\b"<<setw(3)<<(UInt64)Count*100/AllArea<<"%";
	}
	Param.SortSpan=CTime::GetCurrentTime()-Param.RunNow;
	cout<<"  Time="<<Param.SortSpan.Format("%#H:%#M:%#S.\n\n");   return true;
};

//--> 2.Calculate the Source Area of each Cell
bool TDEM::DEM_Area()
{
	CellId	Now, Down, Count;
/*4*/cout<<"  Get Accumulate Upstream Area...      ";   Param.RunNow=CTime::GetCurrentTime();
	Count=0;
	do
	{
		if (Count==0)   Now=Tail;   else   Now=Cell[Now].R;   Count++;
		Cell[Now].L=1;
		if (Cell[Now].D>0)
		{
			Down = Map[Cell[Now].X+Add[Cell[Now].D][0]][Cell[Now].Y+Add[Cell[Now].D][1]];
			Cell[Down].A+=Cell[Now].A;   //Get Upstream catchment Area of each Cell
		}
		else if (Cell[Now].A*1.0/AllArea>0.5)   //Put the Largest Region into the Stack
		{
			Region.push_back(sRegion(Now));
		}
		if ((UInt64)(Count-1)*100/AllArea<(UInt64)Count*100/AllArea)
			cout<<"\b\b\b\b"<<setw(3)<<(UInt64)Count*100/AllArea<<"%";
	}while(Now!=Head);
	Param.DirAreaSpan=CTime::GetCurrentTime()-Param.RunNow;
	cout<<"  Time="<<Param.DirAreaSpan.Format("%#H:%#M:%#S.\n\n"); return true;
}

//--> 3.Find Channel Heads using Change-Point Detection Method
bool TDEM::DEM_ChannelHead()
{
	Direction		k, CurMark, BranchNum;
	deque<double>	Flag;
	vector<CellId>	FlagSort;
	CellId			UpStart, UpMaxA, UpTmp, Len, Window, Now, ExpandLen, Loopi, Loopj, MaxSort, AccumulateSort, Source;
	double			DetX, DetY, DetA, TmpConfidence;
	CStack.assign(1, sCellStack(Region[0].Outlet,0));   CurMark=0;   //Put the Region Outlet into the Cell Stack

	ofstream fout;
	CellId count_A = 0, count_L = 0;
	vector<sPath> path;
	bool Channel_Head;
/*5*/cout << "  Detect Channel Heads...      ";   Param.RunNow = CTime::GetCurrentTime();
	while (!CStack.empty())
	{
		if (CurMark==0)   //Search the Upstream and get the Flow Path Data
		{
			count_A++;
			UpStart=CStack.back().CId;   CStack.pop_back();  Len=0;
			while (Cell[UpStart].A>1)  //Identify the Total Main Stem, the Source Area of each Cell should bigger than 1
			{
				for (UpMaxA=0, BranchNum=0, k=1; k<=8; k++)
				{
					UpTmp=Map[Cell[UpStart].X-Add[k][0]][Cell[UpStart].Y-Add[k][1]];
					if (Cell[UpTmp].D==k && Cell[UpTmp].G < 1)
					{
						if(UpMaxA==0 || Cell[UpTmp].A>Cell[UpMaxA].A)   UpMaxA=UpTmp;
						BranchNum++;    //Record the Branch Num
					}
				}
				if (BranchNum==0)   break;   //No Branch: mean the Source of the Main Stem, Stop the Searching
				else
				{
					CStack.push_back(sCellStack(UpStart, BranchNum));
					UpStart = UpMaxA;   Len++;
				}
			}
			CStack.push_back(sCellStack(UpStart, 0));  Len++;
			GetDetXY(Cell[UpStart].X, DetX, DetY);   DetA=DetX*DetY;
			Flag.clear();   Window=0;   Now=UpStart;   //Find the Detection Window from the Source Cell
			count_L = 0;
			while (Cell[Now].S*sqrt(0.0001+Cell[Now].A*DetA)<sqrt(Param.CSSA) || Cell[Now].A<Param.CASL*Window*Window || Window<10)   //Detection Window Judgement Conditions
			{
				Flag.push_back(0.43429448*Cell[Now].S/log(1.0001+Cell[Now].A*DetA));
				Now=Map[ Cell[Now].X+Add[Cell[Now].D][0] ][ Cell[Now].Y+Add[Cell[Now].D][1] ];
				Window++;
				count_L++;
				path.push_back(sPath(count_A, count_L));	//Put the Cell Id and Accumulate Length into the Flow Path Series
				path.back().S = Cell[Now].S;				//Put the Local Slope into the Flow Path Series
				path.back().A = Cell[Now].A * DetA;			//Put the Accumulate Source Area into the Flow Path Series
				path.back().FA = Flag.back();				//Put the Geomorphologic Characteristic Function Value into the Flow Path Series
				if (Window==Len)   break;
			}
			if (Window==Len)   //the Length of Flow Path Series shorter than the Required Window Size, Stop Detection
			{ 
				for (Loopi=0; Loopi<Len; Loopi++)   CStack.pop_back();
				CurMark=2;   //Mark Detection Stop Status 
				Channel_Head = false;  //Mark no Channel Heads Found
			}
			else   //Satisfy the Detection Conditions and Continue
			{
				ExpandLen=CellId(Param.Window*Window);
				for (Loopi=0; Loopi<ExpandLen; Loopi++)   //Exclude the Begining Part(Controled by the Window Parameter) of the Flow Path Series
				{
					CStack.pop_back();   Flag.pop_front();
				}
				Len-=ExpandLen;   Window-=ExpandLen;
				for (Loopi=0; Loopi<(1-Param.Window)/Param.Window*ExpandLen; Loopi++)   //Extend the Data into Flow Path Series(Controled by the Window Parameter)
				{
					Flag.push_back(0.43429448*Cell[Now].S/log(1.0001+Cell[Now].A*DetA));
					Now=Map[ Cell[Now].X+Add[Cell[Now].D][0] ][ Cell[Now].Y+Add[Cell[Now].D][1] ];
					Window++;
					count_L++;
					path.push_back(sPath(count_A, count_L));		//Put the Cell Id and Accumulate Length into the Flow Path Series
					path.back().S = Cell[Now].S;					//Put the Local Slope into the Flow Path Series
					path.back().A = Cell[Now].A * DetA;				//Put the Accumulate Source Area into the Flow Path Series
					path.back().FA = Flag.back();					//Put the Geomorphologic Characteristic Function Value into the Flow Path Series
					if (Window==Len)   break;
				}
				CurMark=1;   //Mark Detection is Ready
			}
		}
		else if(CurMark==1)   //Use the Pettitt's Approach (1979) for Change-point Detection
		{
			FlagSort.assign(Window, 0);
			for (Loopi=0; Loopi<Window; Loopi++)
				for (Loopj=0; Loopj<Window; Loopj++)
					if (Flag[Loopi]<Flag[Loopj])   FlagSort[Loopi]--;
					else if (Flag[Loopi]>Flag[Loopj])   FlagSort[Loopi]++;
			AccumulateSort=FlagSort[0];   MaxSort=AccumulateSort;   Source=0;
			for (Loopi=1; Loopi<Window; Loopi++)
			{
				AccumulateSort+=FlagSort[Loopi];   FlagSort[Loopi]=0;
				if (MaxSort<abs(AccumulateSort))
				{ 
					MaxSort=abs(AccumulateSort);   Source=Loopi;
				}
			}
			TmpConfidence=2*exp( -6.0*MaxSort*MaxSort/(Window*Window*(1+Window)) );
			if (TmpConfidence<=Param.Confidence )   //the Detection Result Satisfy the Confidence Level, Find Channel Head
			{
				for (Loopi=0; Loopi<Source; Loopi++)   CStack.pop_back();
				CurMark=2;   //Mark Detection Stop Status 
				Channel_Head = true;  //Mark Channel Head Found
			}
			else if (Window==Len)   //the Detection Position Reach the Branch of Flow Path
			{
				for (Loopi=0; Loopi<Len; Loopi++)   CStack.pop_back();
				CurMark=2;   //Mark Detection Stop Status 
				Channel_Head = false;  //Mark no Channel Heads Found
			}
			else   //the Detection Result not reach the Confidence Level, Move the Detection Window
			{
				for (Loopi=0; Loopi<ExpandLen; Loopi++)   //Exclude the Begining Park
				{
					CStack.pop_back();   Flag.pop_front();
				}
				Len-=ExpandLen;   Window-=ExpandLen;
				for (Loopi=0; Loopi<ExpandLen; Loopi++)   //Expend the DownStream Flow Path
				{
					Flag.push_back(0.43429448*Cell[Now].S/log(1.0001+Cell[Now].A*DetA));
					Now=Map[ Cell[Now].X+Add[Cell[Now].D][0] ][ Cell[Now].Y+Add[Cell[Now].D][1] ];
					Window++;
					count_L++;
					path.push_back(sPath(count_A, count_L));
					path.back().S = Cell[Now].S;
					path.back().A = Cell[Now].A * DetA;
					path.back().FA = Flag.back();
					if (Window==Len)   break;   //When the Detection Position Reach the Branch of Flow Path, Stop
				}
				CurMark=1;
			}
		}
		else if (CurMark==2)   //Detection Finish and Start to Mark Channel Attribute
		{
			if (!CStack.empty())   //Set Channel Head Cell's Branch equal to 1
				CStack.back().Branch=0;
			while (!CStack.empty() && CStack.back().Branch<=1)
			{
				if (Channel_Head)  // Mark Channel Head
				{
					Cell[CStack.back().CId].G = 2; Channel_Head = false;
				}
				else     // Mark Channel
				{
					Cell[CStack.back().CId].G = 1;
				}
				CStack.pop_back();
				count_L++;
				path.push_back(sPath(count_A, count_L));
				path.back().S = Cell[Now].S;
				path.back().A = Cell[Now].A * DetA;
				path.back().FA = Flag.back();
			}
			if (!CStack.empty())   //if Stack is not empty, Continue and Repeat
			{
				UpStart=CStack.back().CId;
				for (UpMaxA=0, BranchNum=0, k=1; k<=8; k++)   //Repeat, Find the Main Stem and Record the Branch Num
				{
					UpTmp=Map[Cell[UpStart].X-Add[k][0]][Cell[UpStart].Y-Add[k][1]];
					if (Cell[UpTmp].D==k && Cell[UpTmp].G<1)
					{
						if(UpMaxA==0 || Cell[UpTmp].A>Cell[UpMaxA].A)   UpMaxA=UpTmp;
						BranchNum++;
					}
				}
				if (UpMaxA == 0)
				{
					CurMark = 2;
					Channel_Head = false;
				}
				else
				{
					CStack.push_back(sCellStack(UpMaxA, BranchNum));   CurMark = 0;
				}
			}
		}
	}
	Param.DetectSpan = CTime::GetCurrentTime() - Param.RunNow;
	cout << "  Time=" << Param.DetectSpan.Format("%#H:%#M:%#S.\n\n"); return true;
};
