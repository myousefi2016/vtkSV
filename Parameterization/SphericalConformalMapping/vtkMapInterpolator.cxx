/*=========================================================================
 *
 * Copyright (c) 2014 The Regents of the University of California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *=========================================================================*/

/** @file vtkMapInterpolator.cxx
 *  @brief This implements the vtkMapInterpolator filter as a class
 *
 *  @author Adam Updegrove
 *  @author updega2@gmail.com
 *  @author UC Berkeley
 *  @author shaddenlab.berkeley.edu
 */

#include "vtkMapInterpolator.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellLocator.h"
#include "vtkCenterOfMass.h"
#include "vtkCleanPolyData.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkGradientFilter.h"
#include "vtkS2LandmarkMatcher.h"
#include "vtkLoopSubdivisionFilter.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkSmartPointer.h"
#include "vtkTriangle.h"
#include "vtkWarpVector.h"
#include "vtkXMLPolyDataWriter.h"

#include "math.h"
#include "sparse_matrix.h"

#include <iostream>
#include <cmath>

//---------------------------------------------------------------------------
//vtkCxxRevisionMacro(vtkMapInterpolator, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkMapInterpolator);


//---------------------------------------------------------------------------
vtkMapInterpolator::vtkMapInterpolator()
{
  this->SetNumberOfInputPorts(3);

  this->Verbose               = 1;
  this->NumSourceSubdivisions = 0;
  this->HasBoundary           = 0;

  this->SourceS2Pd = vtkPolyData::New();
  this->TargetPd   = vtkPolyData::New();
  this->TargetS2Pd = vtkPolyData::New();
  this->MappedPd   = vtkPolyData::New();
  this->MappedS2Pd = vtkPolyData::New();

  this->TargetBoundary = vtkIntArray::New();
  this->SourceBoundary = vtkIntArray::New();
}

//---------------------------------------------------------------------------
vtkMapInterpolator::~vtkMapInterpolator()
{
  if (this->SourceS2Pd != NULL)
  {
    this->SourceS2Pd->Delete();
  }
  if (this->TargetPd != NULL)
  {
    this->TargetPd->Delete();
  }
  if (this->TargetS2Pd != NULL)
  {
    this->TargetS2Pd->Delete();
  }
  if (this->MappedPd != NULL)
  {
    this->MappedPd->Delete();
  }
  if (this->MappedS2Pd != NULL)
  {
    this->MappedS2Pd->Delete();
  }
  if (this->TargetBoundary != NULL)
  {
    this->TargetBoundary->Delete();
  }
  if (this->SourceBoundary != NULL)
  {
    this->SourceBoundary->Delete();
  }
}

//---------------------------------------------------------------------------
void vtkMapInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
}

// Generate Separated Surfaces with Region ID Numbers
//---------------------------------------------------------------------------
int vtkMapInterpolator::RequestData(
                                 vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector)
{
  // get the input and output
  vtkPolyData *input1 = vtkPolyData::GetData(inputVector[0]);
  vtkPolyData *input2 = vtkPolyData::GetData(inputVector[1]);
  vtkPolyData *input3 = vtkPolyData::GetData(inputVector[2]);
  vtkPolyData *output = vtkPolyData::GetData(outputVector);

  //Copy the input to operate on
  this->SourceS2Pd->DeepCopy(input1);
  this->TargetPd->DeepCopy(input2);
  this->TargetS2Pd->DeepCopy(input3);

  vtkIdType numSourcePolys = this->SourceS2Pd->GetNumberOfPolys();
  //Check the input to make sure it is there
  if (numSourcePolys < 1)
  {
    vtkDebugMacro("No input!");
    return 0;
  }
  vtkIdType numTargetPolys = this->TargetPd->GetNumberOfPolys();
  //Check the input to make sure it is there
  if (numTargetPolys < 1)
  {
    vtkDebugMacro("No input!");
    return 0;
  }

  if (this->MatchBoundaries() != 1)
  {
    vtkErrorMacro("Error matching the boundaries of the surfaces");
    return 0;
  }

  if (this->SubdivideAndInterpolate() != 1)
  {
    return 0;
  }

  output->DeepCopy(this->MappedPd);
  output->GetPointData()->PassData(input1->GetPointData());
  output->GetCellData()->PassData(input1->GetCellData());
  if (this->PDCheckArrayName(output, 0 ,"Normals") == 1)
  {
    output->GetPointData()->RemoveArray("Normals");
  }
  if (this->PDCheckArrayName(output, 1,"cellNormals") == 1)
  {
    output->GetCellData()->RemoveArray("cellNormals");
  }
  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkMapInterpolator::SubdivideAndInterpolate()
{
  if (this->NumSourceSubdivisions != 0)
  {
    vtkSmartPointer<vtkLoopSubdivisionFilter> subdivider =
      vtkSmartPointer<vtkLoopSubdivisionFilter>::New();
    subdivider->SetInputData(this->SourceS2Pd);
    subdivider->SetNumberOfSubdivisions(this->NumSourceSubdivisions);
    subdivider->Update();
    this->MappedS2Pd->DeepCopy(subdivider->GetOutput());
  }
  else
  {
    this->MappedS2Pd->DeepCopy(this->SourceS2Pd);
  }

  if (this->InterpolateMapOntoSource(this->MappedS2Pd,
                                     this->TargetS2Pd,
                                     this->TargetPd,
                                     this->MappedPd) != 1)
  {
    vtkErrorMacro("Error interpolating onto original target surface");
    return 0;
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkMapInterpolator::InterpolateMapOntoSource(vtkPolyData *mappedSourcePd,
                                                 vtkPolyData *mappedTargetPd,
                                                 vtkPolyData *originalTargetPd,
                                                 vtkPolyData *sourceToTargetPd)
{
  vtkSmartPointer<vtkCellLocator> locator =
    vtkSmartPointer<vtkCellLocator>::New();

  locator->SetDataSet(mappedTargetPd);
  locator->BuildLocator();

  sourceToTargetPd->DeepCopy(mappedSourcePd);
  int numPts = mappedSourcePd->GetNumberOfPoints();
  for (int i=0; i<numPts; i++)
  {
    double pt[3];
    mappedSourcePd->GetPoint(i, pt);

    double closestPt[3];
    vtkIdType closestCell;
    int subId;
    double distance;
    vtkSmartPointer<vtkGenericCell> genericCell =
      vtkSmartPointer<vtkGenericCell>::New();
    locator->FindClosestPoint(pt, closestPt, genericCell, closestCell, subId,
                              distance);

    vtkIdType npts, *pts;
    mappedTargetPd->GetCellPoints(closestCell, npts, pts);
    double pt0[3], pt1[3], pt2[3], a0, a1, a2;
    mappedTargetPd->GetPoint(pts[0], pt0);
    mappedTargetPd->GetPoint(pts[1], pt1);
    mappedTargetPd->GetPoint(pts[2], pt2);
    double area = 0.0;
    vtkMapInterpolator::GetTriangleUV(closestPt, pt0, pt1, pt2, a0, a1, a2);

    double realPt0[3], realPt1[3], realPt2[3];
    originalTargetPd->GetPoint(pts[0], realPt0);
    originalTargetPd->GetPoint(pts[1], realPt1);
    originalTargetPd->GetPoint(pts[2], realPt2);
    double newPoint[3];
    for (int j=0; j<3; j++)
    {
      newPoint[j] = a0*realPt0[j] + a1*realPt1[j] + a2*realPt2[j];
    }
    sourceToTargetPd->GetPoints()->InsertPoint(i, newPoint);
  }

  return 1;
}

int Sign(double testVal)
{
  int asign;
  bool is_negative = testVal < 0;
  if (is_negative)
  {
    asign = -1;
  }
  else
  {
    asign = 1;
  }
  return asign;
}


int vtkMapInterpolator::GetTriangleUV(double f[3], double pt0[3],
                                      double pt1[3], double pt2[3],
					                            double &a0, double &a1, double &a2)
{
  double f0[3], f1[3], f2[3], v0[3], v1[3];
  for (int i=0; i<3; i++)
  {
    v0[i] = pt0[i] - pt1[i];
    v1[i] = pt0[i] - pt2[i];
    f0[i] = pt0[i] - f[i];
    f1[i] = pt1[i] - f[i];
    f2[i] = pt2[i] - f[i];
  }

  double vArea[3], vA0[3], vA1[3], vA2[3];
  vtkMath::Cross(v0, v1, vArea);
  vtkMath::Cross(f1, f2, vA0);
  vtkMath::Cross(f2, f0, vA1);
  vtkMath::Cross(f0, f1, vA2);

  double area = vtkMath::Norm(vArea);
  a0 = vtkMath::Norm(vA0)/area;// * Sign(vtkMath::Dot(vArea, vA0));
  a1 = vtkMath::Norm(vA1)/area;// * Sign(vtkMath::Dot(vArea, vA1));
  a2 = vtkMath::Norm(vA2)/area;// * Sign(vtkMath::Dot(vArea, vA2));
  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkMapInterpolator::ComputeArea(double pt0[], double pt1[],
                                    double pt2[], double &area)
{
  area = 0.0;
  area += (pt0[0]*pt1[1])-(pt1[0]*pt0[1]);
  area += (pt1[0]*pt2[1])-(pt2[0]*pt1[1]);
  area += (pt2[0]*pt0[1])-(pt0[0]*pt2[1]);
  area *= 0.5;

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkMapInterpolator::MatchBoundaries()
{
  if (this->FindBoundary(this->TargetS2Pd, this->TargetBoundary) != 1)
  {
    return 0;
  }
  if (this->FindBoundary(this->SourceS2Pd, this->SourceBoundary) != 1)
  {
    return 0;
  }

  if (this->HasBoundary == 1)
  {
    if (this->MoveBoundaryPoints() != 1)
    {
      return 0;
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkMapInterpolator::FindBoundary(vtkPolyData *pd, vtkIntArray *isBoundary)
{
  int numPoints = pd->GetNumberOfPoints();
  int numCells = pd->GetNumberOfCells();

  for (int i=0; i<numPoints; i++)
  {
    isBoundary->InsertValue(i, 0);
  }
  for (int i=0; i<numCells; i++)
  {
    vtkIdType npts, *pts;
    pd->GetCellPoints(i, npts, pts);
    //if (npts != 3)
    //{
    //  return 0;
    //}
    for (int j=0; j<npts; j++)
    {
      vtkIdType p0, p1;
      p0 = pts[j];
      p1 = pts[(j+1)%npts];

      vtkSmartPointer<vtkIdList> edgeNeighbor =
        vtkSmartPointer<vtkIdList>::New();
      pd->GetCellEdgeNeighbors(i, p0, p1, edgeNeighbor);

      if (edgeNeighbor->GetNumberOfIds() == 0)
      {
        isBoundary->InsertValue(p0, 1);
        isBoundary->InsertValue(p1, 1);
        this->HasBoundary = 1;
      }
    }
  }
  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkMapInterpolator::MoveBoundaryPoints()
{
  int numPoints = this->SourceS2Pd->GetNumberOfPoints();
  vtkSmartPointer<vtkCellLocator> locator =
    vtkSmartPointer<vtkCellLocator>::New();

  locator->SetDataSet(this->TargetS2Pd);
  locator->BuildLocator();

  for (int i=0; i<numPoints; i++)
  {
    if (this->SourceBoundary->GetValue(i) == 1)
    {
      double pt[3];
      this->SourceS2Pd->GetPoint(i, pt);
      double closestPt[3];
      vtkIdType closestCell;
      int subId;
      double distance;
      vtkSmartPointer<vtkGenericCell> genericCell =
	      vtkSmartPointer<vtkGenericCell>::New();
      locator->FindClosestPoint(pt, closestPt, genericCell, closestCell, subId,
				distance);
      double newPt[3];
      if (this->GetPointOnTargetBoundary(i, closestCell, newPt) != 1)
      {
	return 0;
      }
      this->SourceS2Pd->GetPoints()->SetPoint(i, newPt);
    }
  }
  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkMapInterpolator::GetPointOnTargetBoundary(int srcPtId, int targCellId, double returnPt[])
{
  double srcPt[3];
  this->SourceS2Pd->GetPoint(srcPtId, srcPt);
  vtkSmartPointer<vtkIdList> boundaryPts =
    vtkSmartPointer<vtkIdList>::New();
  int numBoundaryPts = this->BoundaryPointsOnCell(this->TargetS2Pd, targCellId, boundaryPts, this->TargetBoundary);

  if (numBoundaryPts == 1)
  {
    int ptId = boundaryPts->GetId(0);
    this->TargetS2Pd->GetPoint(ptId, returnPt);
  }
  else if (numBoundaryPts == 2)
  {
    int ptId0 = boundaryPts->GetId(0);
    int ptId1 = boundaryPts->GetId(1);
    double pt0[3], pt1[3];
    this->TargetS2Pd->GetPoint(ptId0, pt0);
    this->TargetS2Pd->GetPoint(ptId1, pt1);
    this->GetProjectedPoint(pt0, pt1, srcPt, returnPt);
  }
  else if (numBoundaryPts == 3)
  {
    int ptId0;
    int ptId1;
    this->GetClosestTwoPoints(this->TargetS2Pd, srcPt, boundaryPts, ptId0, ptId1);
    double pt0[3], pt1[3];
    this->TargetS2Pd->GetPoint(ptId0, pt0);
    this->TargetS2Pd->GetPoint(ptId1, pt1);
    this->GetProjectedPoint(pt0, pt1, srcPt, returnPt);
  }
  else
  {
    vtkErrorMacro("Boundaries do not match well enough");
    return 0;
  }
  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkMapInterpolator::BoundaryPointsOnCell(vtkPolyData *pd, int targCellId, vtkIdList *boundaryPts, vtkIntArray *isBoundary)
{
  int numBounds = 0;
  vtkIdType npts, *pts;
  pd->GetCellPoints(targCellId, npts, pts);
  boundaryPts->Reset();
  for (int j=0; j<npts; j++)
  {
    if (isBoundary->GetValue(pts[j]) == 1)
    {
      boundaryPts->InsertNextId(pts[j]);
      numBounds++;
    }
  }
  if (numBounds == 2)
  {
    vtkSmartPointer<vtkIdList> edgeNeighbor =
      vtkSmartPointer<vtkIdList>::New();
    int p0 = boundaryPts->GetId(0);
    int p1 = boundaryPts->GetId(1);
    pd->GetCellEdgeNeighbors(targCellId, p0, p1, edgeNeighbor);

    if (edgeNeighbor->GetNumberOfIds() != 0)
    {
      int newCell = edgeNeighbor->GetId(0);
      numBounds = this->BoundaryPointsOnCell(pd, newCell, boundaryPts, isBoundary);
    }
  }

  return numBounds;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkMapInterpolator::GetProjectedPoint(double pt0[], double pt1[], double projPt[], double returnPt[])
{
  double vec0[3];
  double vec1[3];
  for (int i=0; i<3; i++)
  {
    vec0[i] = pt1[i] - pt0[i];
    vec1[i] = projPt[i] - pt0[i];
  }
  double proj = vtkMath::Dot(vec0, vec1);

  double lineVec[3], perpVec[3];
  double norm = vtkMath::Dot(vec0, vec0);
  for (int i=0; i<3; i++)
  {
    returnPt[i] = pt0[i] + proj/norm * vec0[i];
  }
  return 1;
}

//---------------------------------------------------------------------------
/**
 * @brief
 * @param *pd
 * @return
 */
int vtkMapInterpolator::GetClosestTwoPoints(vtkPolyData *pd, double projPt[], vtkIdList *boundaryPts, int &ptId0, int &ptId1)
{
  double dist[3];
  for (int i=0; i<3; i++)
  {
    int ptId = boundaryPts->GetId(i);
    double pt[3];
    pd->GetPoint(ptId, pt);
    dist[i] = std::sqrt(std::pow(projPt[0]-pt[0], 2.0) +
                        std::pow(projPt[1]-pt[1], 2.0) +
                        std::pow(projPt[2]-pt[2], 2.0));

  }

  if (dist[0] > dist[1])
  {
    ptId0 = boundaryPts->GetId(1);
    if (dist[0] > dist[2])
    {
      ptId1 = boundaryPts->GetId(2);
    }
    else
    {
      ptId1 = boundaryPts->GetId(0);
    }
  }
  else
  {
    ptId0 = boundaryPts->GetId(0);
    if (dist[1] > dist[2])
    {
      ptId1 = boundaryPts->GetId(2);
    }
    else
    {
      ptId1 = boundaryPts->GetId(1);
    }
  }
  return 1;
}

// ----------------------
// PDCheckArrayName
// ----------------------
/**
 * @brief Function to check is array with name exists in cell or point data
 * @param object this is the object to check if the array exists
 * @param datatype this is point or cell. point =0,cell=1
 * @param arrayname this is the name of the array to check
 * @reutrn this returns 1 if the array exists and zero if it doesn't
 * or the function does not return properly.
 */

int vtkMapInterpolator::PDCheckArrayName(vtkPolyData *object,int datatype,std::string arrayname )
{
  vtkIdType i;
  int numArrays;
  int exists =0;

  if (datatype == 0)
  {
    numArrays = object->GetPointData()->GetNumberOfArrays();
    for (i=0;i<numArrays;i++)
    {
      if (!strcmp(object->GetPointData()->GetArrayName(i),arrayname.c_str()))
      {
	exists =1;
      }
    }
  }
  else
  {
    numArrays = object->GetCellData()->GetNumberOfArrays();
    for (i=0;i<numArrays;i++)
    {
      if (!strcmp(object->GetCellData()->GetArrayName(i),arrayname.c_str()))
      {
	exists =1;
      }
    }
  }

  return exists;
}
