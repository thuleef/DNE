//Attention!!! Need Alignment in Project Properties: Project - Properties - Configuration Properties - C/C++ - Code Generation - Structure Member Alignment <1 byte>
//注意!!! 需要在项目属性中配置字节对齐： 项目-工程属性-配置属性-C/C++-代码生成-结构成员对齐<1字节对齐>

#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <atlstr.h>
#include <atltime.h>
#include <cmath>
#include <vector>
#include <deque>
#include <shlwapi.h>
#include <algorithm>

using namespace std;

typedef		unsigned char				UInt8;
typedef		signed char					Int8;
typedef		unsigned short int			UInt16;
typedef		signed short int			Int16;
typedef		unsigned long int			UInt32;
typedef		signed long int				Int32;
typedef		unsigned long long int		UInt64;
typedef		signed long long int		Int64;

#if defined (_WIN64)
typedef		Int64			CellId;
#else
typedef		Int32			CellId;
#endif
typedef		UInt32			RowCol;
typedef		Int8			Direction;
typedef		UInt8			Iterator;
typedef		Int32			ChannelId;
typedef		UInt16			RegionId;

//Drainage network extraction control options
struct sParam	
{
	char			*DEMPath, *FileName, *OutPath;				//DEM Folder, DEM Filename, Output Folder
	double			CSSA, CASL, Window, Confidence;				//Change-Point Detection Parameter: Critical_S_Sqrt_A, Critical_A_Sqr_L, Source Window Movement, Confidence Level
	double			GeoPixelScale[3], GeoTiePoint[6];			//Geographical Information of DEM
	UInt32			GeoModel, GeoCoordinate;					//Geographical Information of DEM
	CTime			RunStart, RunNow;							//Start Time and Running Time
	CTimeSpan		ReadSpan, SortSpan, DirAreaSpan;			//the Consumed Time of Read, Sort and Calculate the Source Area of DEM
	CTimeSpan		DetectSpan, PrintSpan, TotalSpan;			//the Consumed Time of Detect, Print and Altogether of the Program
	sParam()
	{
		DEMPath=NULL;   FileName=NULL;   OutPath=NULL;
		CSSA = 0;   CASL = 0;   Window = 0;   Confidence = 0;
		memset(&GeoPixelScale, 0, 24);   memset(&GeoTiePoint, 0, 48);
		GeoModel=0;   GeoCoordinate=0;
	}
};

struct sRowCol
{
	RowCol R, C;
	sRowCol(){   R=0;   C=0;   }
	sRowCol(RowCol R0, RowCol C0){   R=R0;   C=C0;   }
};

struct sPoint
{
	double X, Y;
	sPoint(){   X=0;   Y=0;   }
	sPoint(double X0, double Y0){   X=X0;   Y=Y0;   }
};

//Data Structure of Flow Path
struct sPath
{
	CellId ID;												//Cell Id
	CellId L;												//Accumulative Length of Cell p
	double S;												//Local Slope of Cell p
	double A;												//Source Area of Cell p
	double FA;												//Geomorphologic Characteristic Function Value of Cell p
	sPath(){ ID = L = 0; S = A = FA = 0; }
	sPath(CellId id0, CellId l0) { ID = id0; L = l0; S = A = FA = 0; }
};

//Region: Record the Region Outlet Cell ID
struct sRegion
{
	CellId Outlet;
	sRegion(){ Outlet = 0; }
	sRegion(CellId Outlet0) { Outlet = Outlet0; }
};

struct sSmlCell
{
	Int8	k;
	CellId  ID;
	float	E;
	sSmlCell() {}
	sSmlCell(CellId id0, Int8 k0, float E0){ ID = id0; k = k0;  E = E0; }
};

struct sCell
{
	CellId				L, R;							//the Left and Right Nodes of the Size Balance Binary Tree
	CellId				Size;							//the Size of the Size Balance Binary Tree
	RowCol				X, Y;							//X is Row and Y is Col
	float				E, S;							//Elevation and Slope
	CellId				G, A;							//Cell Attribute Mark and Source Area
	Direction			D;								//D8 Direction
	sCell(){   L=0;   R=0;   Size=0;   X=0;   Y=0;   E=0;   S=0;   G=0;  A=0;   D=0;  }
	sCell(RowCol R0, RowCol C0, float E0){ L=0;   R=0;   Size=0;   X=R0;   Y=C0;   E=E0;   S=0;   G=0;  A=1;   D=0;}
};

//Stack of Cell for Channel Head Detection
struct sCellStack
{
	CellId				CId;							//Index: Cell Id
	Direction			Branch;							//Branch Num of this Cell
	sCellStack(){   CId=0;  Branch=0;   }
	sCellStack(CellId CId0, Direction Branch0){ CId=CId0;  Branch=Branch0; }
};

class TDEM
{
public:
	static const Direction		Add[9][2];				//Const: Cell Step Add, Inverse Add
	static const Direction		Inv[9];					//Const: Coresponding Add_Clock

	sParam						Param;					//Parameters of the Program
	RowCol						Row, Col;				//Total Row Num and Col Num of this DEM
	CellId						AllArea;				//Total Cell Num of this DEM
	vector<sCell>				Cell;					//Container(Vector) of All Cell Data
	vector<vector<CellId>>		Map;					//Container(Vector<Vector>) of the Cell map
	vector<sRegion>				Region;					//Container(Vector) of the Region to be Extracted

	TDEM();
	~TDEM();
	bool DEM_Sort();									//1.DEM Sort by the Size Balanced Binary Search Tree Method
	bool DEM_Area();									//2.Calculate the Source Area of each Cell
	bool DEM_ChannelHead();								//3.Find Channel Heads using Change-Point Detection Method

	void GetXY(RowCol, RowCol, sPoint &);				//Get Geographical Coordinates by Row and Col Id
	void GetDetXY(RowCol, double &, double &);			//Get Geographical Unit Distance(1 Degree) by Row Id

private:
	CellId						Root, Head, Tail;		//Root, Head and Tail of the Size Balanced Binary Search Tree
	deque<sRowCol>				Inword;					//the Breadth-First Search Queue
	deque<sCellStack>			CStack;					//the Container(Deque) of sCellStack using to Detect the Channel Heads

	void RightRotate(CellId &);							//1.DEM_Sort.Tree Right Rotate
	void LeftRotate(CellId &);							//1.DEM_Sort.Tree Left Rotate
	void Maintain(CellId &, bool);						//1.DEM_Sort.Tree Maintain
	void InsertTree(CellId, CellId &);					//1.DEM_Sort.Insert Node into Tree
	void DeleteMin(CellId &, CellId &);					//1.DEM_Sort.Delete Min Node from Tree
	void InsertTail(CellId);							//1.DEM_Sort.Insert Link node at Tail
};