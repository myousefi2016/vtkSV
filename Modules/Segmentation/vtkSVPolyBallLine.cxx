/*=========================================================================

Program:   VMTK
Module:    $RCSfile: vtkSVPolyBallLine.cxx,v $
Language:  C++
Date:      $Date: 2006/04/06 16:46:43 $
Version:   $Revision: 1.5 $

  Copyright (c) Luca Antiga, David Steinman. All rights reserved.
  See LICENCE file for details.

  Portions of this code are covered under the VTK copyright.
  See VTKCopyright.txt or http://www.kitware.com/VTKCopyright.htm
  for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include "vtkSVPolyBallLine.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSVMathUtils.h"
#include "vtkSVGlobals.h"
#include "vtkSVGeneralUtils.h"

// ----------------------
// StandardNewMacro
// ----------------------
vtkStandardNewMacro(vtkSVPolyBallLine);

// ----------------------
// Constructor
// ----------------------
vtkSVPolyBallLine::vtkSVPolyBallLine()
{
  this->Input = NULL;
  this->InputCellIds = NULL;
  this->InputCellId = -1;
  this->PolyBallRadiusArrayName = NULL;
  this->LocalCoordinatesArrayName = NULL;
  this->LastPolyBallCellId = -1;
  this->LastPolyBallCellSubId = -1;
  this->LastPolyBallCellPCoord = 0.0;
  this->LastPolyBallCenter[0] = this->LastPolyBallCenter[1] = this->LastPolyBallCenter[2] = 0.0;
  this->LastPolyBallCenterRadius = 0.0;
  this->UseRadiusInformation = 1;
  this->UsePointNormal = 0;
  this->UseLocalCoordinates = 0;
  this->FastEvaluate = 0;
  this->PointLocator = vtkPointLocator::New();

  for (int i=0; i<3; i++)
  {
    this->PointNormal[i] = 0.0;
    this->LastLocalCoordX[i] = 0.0;
    this->LastLocalCoordY[i] = 0.0;
    this->LastLocalCoordZ[i] = 0.0;
  }
}

// ----------------------
// Destructor
// ----------------------
vtkSVPolyBallLine::~vtkSVPolyBallLine()
{
  if (this->Input != NULL)
    {
    this->Input->Delete();
    this->Input = NULL;
    }

  if (this->InputCellIds != NULL)
    {
    this->InputCellIds->Delete();
    this->InputCellIds = NULL;
    }

  if (this->PolyBallRadiusArrayName != NULL)
    {
    delete[] this->PolyBallRadiusArrayName;
    this->PolyBallRadiusArrayName = NULL;
    }

  if (this->LocalCoordinatesArrayName != NULL)
    {
    delete[] this->LocalCoordinatesArrayName;
    this->LocalCoordinatesArrayName = NULL;
    }

  if (this->PointLocator != NULL)
    {
      this->PointLocator->Delete();
    this->PointLocator = NULL;
    }
}

// ----------------------
// ComplexDot
// ----------------------
double vtkSVPolyBallLine::ComplexDot(double x[4], double y[4])
{
  return x[0]*y[0] + x[1]*y[1] + x[2]*y[2] - x[3]*y[3];
}

// ----------------------
// PreprocessInputForFastEvaluate
// ----------------------
void vtkSVPolyBallLine::PreprocessInputForFastEvaluate()
{
  vtkIdType npts, *pts;
  double pt[3];

  this->Input->BuildCells();

  vtkNew(vtkIdList, cellIds);
  if (this->InputCellIds)
  {
    cellIds->DeepCopy(this->InputCellIds);
  }
  else if (this->InputCellId != -1)
  {
    cellIds->InsertNextId(this->InputCellId);
  }
  else
  {
    cellIds->SetNumberOfIds(this->Input->GetNumberOfCells());
    for (int k=0; k<this->Input->GetNumberOfCells(); k++)
    {
      cellIds->SetId(k,k);
    }
  }

  this->CellPointsVector.clear();
  this->CellPointsVector.resize(cellIds->GetNumberOfIds());

  for (int c=0; c<cellIds->GetNumberOfIds(); c++)
  {
    int cellId = cellIds->GetId(c);

    if (this->Input->GetCellType(cellId)!=VTK_LINE && this->Input->GetCellType(cellId)!=VTK_POLY_LINE)
    {
      continue;
    }

    this->Input->GetCellPoints(cellId,npts,pts);

    for (int i=0; i<npts; i++)
    {
      this->CellPointsVector[cellId].push_back(pts[i]);
    }
  }

  vtkDataArray *polyballRadiusArray = this->Input->GetPointData()->GetArray(this->PolyBallRadiusArrayName);

  if (polyballRadiusArray==NULL)
  {
    vtkErrorMacro(<<"PolyBallRadiusArray with name specified does not exist!");
  }

  this->PointsVector.clear();
  this->PointsVector.resize(this->Input->GetNumberOfPoints());

  this->RadiusVector.clear();
  this->RadiusVector.resize(this->Input->GetNumberOfPoints());


  for (int i=0; i<this->Input->GetNumberOfPoints(); i++)
  {
    this->Input->GetPoint(i, pt);

    this->PointsVector[i].x = pt[0];
    this->PointsVector[i].y = pt[1];
    this->PointsVector[i].z = pt[2];

    this->RadiusVector[i] = polyballRadiusArray->GetComponent(i, 0);
  }

  return;
}


// ----------------------
// BuildLocator
// ----------------------
void vtkSVPolyBallLine::BuildLocator()
{
  this->PointLocator->SetDataSet(this->Input);
  this->PointLocator->BuildLocator();
}

// ----------------------
// EvaluateFunction
// ----------------------
double vtkSVPolyBallLine::EvaluateFunction(double x[3])
{
  vtkIdType i, k;
  vtkIdType npts, *pts;
  double polyballFunctionValue, minPolyBallFunctionValue;
  double point0[3], point1[3];
  double radius0, radius1;
  double vector0[4], vector1[4], closestPoint[4];
  double local0[3][3], local1[3][3];
  double localDiffs[3][3], finalLocal[3][3];;
  double t;
  double num, den;
  vtkDataArray *polyballRadiusArray = NULL;
  vtkDataArray *localXArray = NULL;
  vtkDataArray *localYArray = NULL;
  vtkDataArray *localZArray = NULL;
  vtkNew(vtkIdList, tmpList);
  vtkNew(vtkIdList, tmpList2);

  if (!this->Input)
    {
    vtkErrorMacro(<<"No Input specified!");
    return SV_ERROR;
    }

  if (this->Input->GetNumberOfPoints()==0)
    {
    vtkWarningMacro(<<"Empty Input specified!");
    return SV_ERROR;
    }

  if (this->UseRadiusInformation)
    {
    if (!this->PolyBallRadiusArrayName)
      {
      vtkErrorMacro(<<"No PolyBallRadiusArrayName specified!");
      return SV_ERROR;
      }

    if (!this->FastEvaluate)
    {
      polyballRadiusArray = this->Input->GetPointData()->GetArray(this->PolyBallRadiusArrayName);

      if (polyballRadiusArray==NULL)
        {
        vtkErrorMacro(<<"PolyBallRadiusArray with name specified does not exist!");
        return SV_ERROR;
        }
      }
    }

  if (this->UseLocalCoordinates)
    {
    if (!this->LocalCoordinatesArrayName)
      {
      vtkErrorMacro("Must provide local coordinates name if using local coordinates");
      return SV_ERROR;
      }

    std::string localXName = this->LocalCoordinatesArrayName; localXName = localXName+"X";
    localXArray = this->Input->GetPointData()->GetArray(localXName.c_str());
    std::string localYName = this->LocalCoordinatesArrayName; localYName = localYName+"Y";
    localYArray = this->Input->GetPointData()->GetArray(localYName.c_str());
    std::string localZName = this->LocalCoordinatesArrayName; localZName = localZName+"Z";
    localZArray = this->Input->GetPointData()->GetArray(localZName.c_str());
    }

  if (this->Input->GetLines()==NULL)
    {
    vtkWarningMacro(<<"No lines in Input dataset.");
    return SV_ERROR;
    }

  if (!this->FastEvaluate)
  {
    this->Input->BuildCells();
  }
#if (VTK_MAJOR_VERSION <= 5)
  this->Input->Update();
#endif

  minPolyBallFunctionValue = VTK_SV_LARGE_DOUBLE;

  closestPoint[0] = closestPoint[1] = closestPoint[2] = closestPoint[3] = 0.0;

  this->LastPolyBallCellId = -1;
  this->LastPolyBallCellSubId = -1;
  this->LastPolyBallCellPCoord = 0.0;
  this->LastPolyBallCenter[0] = this->LastPolyBallCenter[1] = this->LastPolyBallCenter[2] = 0.0;
  this->LastPolyBallCenterRadius = 0.0;

  vtkIdList* cellIds = vtkIdList::New();

  if (this->InputCellIds)
    {
    cellIds->DeepCopy(this->InputCellIds);
    }
  else if (this->InputCellId != -1)
    {
    cellIds->InsertNextId(this->InputCellId);
    }
  else
    {
    cellIds->SetNumberOfIds(this->Input->GetNumberOfCells());
    for (k=0; k<this->Input->GetNumberOfCells(); k++)
      {
      cellIds->SetId(k,k);
      }
    }

  if (this->FastEvaluate)
  {
    if (this->CellPointsVector.size() != cellIds->GetNumberOfIds())
    {
      vtkErrorMacro("Need to call PreprocessInputForFastEvaluate() prior to EvaluateFunction call if FastEvaluate turned on");
      return SV_ERROR;
    }
    if (this->PointsVector.size() != this->Input->GetNumberOfPoints())
    {
      vtkErrorMacro("Need to call PreprocessInputForFastEvaluate() prior to EvaluateFunction call if FastEvaluate turned on");
      return SV_ERROR;
    }
  }

  for (int c=0; c<cellIds->GetNumberOfIds(); c++)
    {
    int cellId = cellIds->GetId(c);

    if (this->FastEvaluate)
    {
      npts = this->CellPointsVector[cellId].size();
      pts = new vtkIdType[npts];
      for (int i=0; i<npts; i++)
      {
        pts[i] = this->CellPointsVector[cellId][i];
      }
    }
    else
    {
      if (this->Input->GetCellType(cellId)!=VTK_LINE && this->Input->GetCellType(cellId)!=VTK_POLY_LINE)
        {
        continue;
        }

      this->Input->GetCellPoints(cellId,npts,pts);
    }

    for (i=0; i<npts-1; i++)
      {
        int ptId0 = pts[i];
        int ptId1 = pts[i+1];

        if (this->FastEvaluate)
        {
          point0[0] = this->PointsVector[pts[i]].x;
          point0[1] = this->PointsVector[pts[i]].y;
          point0[2] = this->PointsVector[pts[i]].z;

          point1[0] = this->PointsVector[pts[i+1]].x;
          point1[1] = this->PointsVector[pts[i+1]].y;
          point1[2] = this->PointsVector[pts[i+1]].z;
        }
        else
        {
          this->Input->GetPoint(pts[i],point0);
          this->Input->GetPoint(pts[i+1],point1);
        }

      if (this->UseRadiusInformation)
        {
          if (this->FastEvaluate)
          {
            radius0 = this->RadiusVector[ptId0];
            radius1 = this->RadiusVector[ptId1];
          }
          else
          {
            radius0 = polyballRadiusArray->GetComponent(ptId0,0);
            radius1 = polyballRadiusArray->GetComponent(ptId1,0);
          }
        }
      else
        {
        radius0 = 0.0;
        radius1 = 0.0;
        }

      if (this->UseLocalCoordinates)
        {
        if (localXArray != NULL && localYArray != NULL && localZArray != NULL)
          {
          localXArray->GetTuple(ptId0, local0[0]);
          localYArray->GetTuple(ptId0, local0[1]);
          localZArray->GetTuple(ptId0, local0[2]);
          localXArray->GetTuple(ptId1, local1[0]);
          localYArray->GetTuple(ptId1, local1[1]);
          localZArray->GetTuple(ptId1, local1[2]);
          }
        else
          {
          for (int j=0; j<3; j++)
            {
            for (int k=0; k<3; k++)
              {
              local0[j][k] = 0.0;
              local1[j][k] = 0.0;
              }
            }
          }
        }

      vector0[0] = point1[0] - point0[0];
      vector0[1] = point1[1] - point0[1];
      vector0[2] = point1[2] - point0[2];
      vector0[3] = radius1 - radius0;
      vector1[0] = x[0] - point0[0];
      vector1[1] = x[1] - point0[1];
      vector1[2] = x[2] - point0[2];
      vector1[3] = 0.0 - radius0;
      if (this->UseLocalCoordinates)
      {
        for (int j=0; j<3; j++)
          vtkMath::Subtract(local1[j], local0[j], localDiffs[j]);
      }

//       cout<<x[0]<<" "<<x[1]<<" "<<x[2]<<" "<<point0[0]<<" "<<point0[1]<<" "<<point0[2]<<" "<<point1[0]<<" "<<point1[1]<<" "<<point1[2]<<" "<<endl;

      num = this->ComplexDot(vector0,vector1);
      den = this->ComplexDot(vector0,vector0);

      if (fabs(den)<VTK_SV_DOUBLE_TOL)
        {
        continue;
        }

      t = num / den;

      if (t<VTK_SV_DOUBLE_TOL)
        {
        t = 0.0;
        closestPoint[0] = point0[0];
        closestPoint[1] = point0[1];
        closestPoint[2] = point0[2];
        closestPoint[3] = radius0;
        if (this->UseLocalCoordinates)
          {
          for (int j=0; j<3; j++)
            {
            for (int k=0; k<3; k++)
              finalLocal[j][k] = local0[j][k];
            }
          }
        }
      else if (1.0-t<VTK_SV_DOUBLE_TOL)
        {
        t = 1.0;
        closestPoint[0] = point1[0];
        closestPoint[1] = point1[1];
        closestPoint[2] = point1[2];
        closestPoint[3] = radius1;
        if (this->UseLocalCoordinates)
          {
          for (int j=0; j<3; j++)
            {
            for (int k=0; k<3; k++)
              finalLocal[j][k] = local1[j][k];
            }
          }
        }
      else
        {
        closestPoint[0] = point0[0] + t * vector0[0];
        closestPoint[1] = point0[1] + t * vector0[1];
        closestPoint[2] = point0[2] + t * vector0[2];
        closestPoint[3] = radius0 + t * vector0[3];
        if (this->UseLocalCoordinates)
          {
          for (int j=0; j<3; j++)
            {
            for (int k=0; k<3; k++)
              finalLocal[j][k] = local0[j][k] + t * localDiffs[j][k];
            }
          }
        }

      polyballFunctionValue = (x[0]-closestPoint[0])*(x[0]-closestPoint[0]) + (x[1]-closestPoint[1])*(x[1]-closestPoint[1]) + (x[2]-closestPoint[2])*(x[2]-closestPoint[2]) - closestPoint[3]*closestPoint[3];

      if (this->UsePointNormal)
        {
        double dir0[3];
        vtkMath::Subtract(x, closestPoint, dir0);
        vtkMath::Normalize(dir0);
        double align0 = vtkMath::Dot(this->PointNormal, dir0);

        if (align0 <= 0.0)
          {
          // We found a false positive
          //polyballFunctionValue = VTK_SV_LARGE_DOUBLE/2.0;
          //polyballFunctionValue = 100.0*polyballFunctionValue;
          continue;
          }
        }

       if (polyballFunctionValue<minPolyBallFunctionValue)
        {
        minPolyBallFunctionValue = polyballFunctionValue;
        this->LastPolyBallCellId = cellId;
        this->LastPolyBallCellSubId = i;
        this->LastPolyBallCellPCoord = t;
        this->LastPolyBallCenter[0] = closestPoint[0];
        this->LastPolyBallCenter[1] = closestPoint[1];
        this->LastPolyBallCenter[2] = closestPoint[2];
        this->LastPolyBallCenterRadius = closestPoint[3];
        if (this->UseLocalCoordinates)
          {
          for (int j=0; j<3; j++)
            {
            this->LastLocalCoordX[j] = finalLocal[0][j];
            this->LastLocalCoordY[j] = finalLocal[1][j];
            this->LastLocalCoordZ[j] = finalLocal[2][j];
            }
          }
        }
      }

    if (this->FastEvaluate)
    {
      delete [] pts;
    }
    }

  cellIds->Delete();

  return minPolyBallFunctionValue;
}

// ----------------------
// EvaluateGradient
// ----------------------
void vtkSVPolyBallLine::EvaluateGradient(double x[3], double n[3])
{
  vtkWarningMacro("Poly ball gradient computation not yet implemented!");
  // TODO
}

// ----------------------
// PrintSelf
// ----------------------
void vtkSVPolyBallLine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
