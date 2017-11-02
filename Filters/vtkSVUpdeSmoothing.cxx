/*=========================================================================
 *
 * Copyright (c) 2014-2015 The Regents of the University of California.
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

/** @file vtkSVUpdeSmoothing.cxx
 *  @brief This implements the vtkSVUpdeSmoothing filter
 *
 *  @author Adam Updegrove
 *  @author updega2@gmail.com
 *  @author UC Berkeley
 *  @author shaddenlab.berkeley.edu
 */

#include "vtkSVUpdeSmoothing.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkCellLocator.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkSmoothPolyDataFilter.h"
#include "vtkSVGeneralUtils.h"
#include "vtkSVGlobals.h"
#include "vtkSVMathUtils.h"
#include "vtkFloatArray.h"
#include "vtkPolyDataNormals.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkTriangle.h"
#include "vtkWindowedSincPolyDataFilter.h"

#include <iostream>

// ----------------------
// StandardNewMacro
// ----------------------
vtkStandardNewMacro(vtkSVUpdeSmoothing);

// ----------------------
// Constructor
// ----------------------
vtkSVUpdeSmoothing::vtkSVUpdeSmoothing()
{
    this->NumSmoothOperations = 30;
    this->Alpha = 0.5;
    this->Beta = 0.8;
}

// ----------------------
// Destructor
// ----------------------
vtkSVUpdeSmoothing::~vtkSVUpdeSmoothing()
{
}

// ----------------------
// PrintSelf
// ----------------------
void vtkSVUpdeSmoothing::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of smooth operations: " << this->NumSmoothOperations << "\n";
}

// ----------------------
// RequestData
// ----------------------
int vtkSVUpdeSmoothing::RequestData(vtkInformation *vtkNotUsed(request),
                                           vtkInformationVector **inputVector,
                                           vtkInformationVector *outputVector)
{
  // get the input and output
  vtkPolyData *input = vtkPolyData::GetData(inputVector[0]);
  vtkPolyData *output = vtkPolyData::GetData(outputVector);

  // Define variables used by the algorithm
  vtkNew(vtkPoints, inpts);
  vtkNew(vtkCellArray, inPolys);
  vtkIdType numPts, numPolys;
  vtkIdType newId, cellId,pointId;

  //Get input points, polys and set the up in the vtkPolyData mesh
  inpts = input->GetPoints();
  inPolys = input->GetPolys();

  //Get the number of Polys for scalar  allocation
  numPolys = input->GetNumberOfPolys();
  numPts = input->GetNumberOfPoints();

  //Check the input to make sure it is there
  if (numPolys < 1)
  {
      vtkDebugMacro("No input!");
      return SV_OK;
  }

  //vtkNew(vtkCellLocator, locator);
  //locator->SetDataSet(input);
  //locator->BuildLocator();
  //
  //double closestPt[3];
  //vtkIdType closestCell;
  //int subId;
  //double distance;
  //vtkNew(vtkGenericCell, genericCell);

  input->BuildLinks();
  vtkNew(vtkPolyData, tmp);
  tmp->DeepCopy(input);

  for (int i=0; i<this->NumSmoothOperations; i++)
  {
    vtkNew(vtkSmoothPolyDataFilter, smoother);
    smoother->SetInputData(tmp);
    smoother->SetNumberOfIterations(1000);
    smoother->Update();

    vtkNew(vtkPolyData, smoothedPd);
    smoothedPd->DeepCopy(smoother->GetOutput());

    int numPoints = tmp->GetNumberOfPoints();

    smoothedPd->BuildLinks();

    for (int i=0; i<numPoints; i++)
    {
      vtkNew(vtkIdList, usedPtList);
      vtkNew(vtkIdList, pointCellIds);
      input->GetPointCells(i, pointCellIds);

      double oAvgVec[3];
      oAvgVec[0] = 0.0; oAvgVec[1] = 0.0; oAvgVec[2] = 0.0;
      double mAvgVec[3];
      mAvgVec[0] = 0.0; mAvgVec[1] = 0.0; mAvgVec[2] = 0.0;
      double nAvgVec[3];
      nAvgVec[0] = 0.0; nAvgVec[1] = 0.0; nAvgVec[2] = 0.0;

      for (int j=0; j<pointCellIds->GetNumberOfIds(); j++)
      {
        int cellId = pointCellIds->GetId(j);
        vtkIdType npts, *pts;

        input->GetCellPoints(cellId, npts, pts);

        double oPtCoords[3][3], mPtCoords[3][3], nPtCoords[3][3];
        for (int k=0; k<3; k++)
        {
          input->GetPoint(pts[k], oPtCoords[k]);
          tmp->GetPoint(pts[k], mPtCoords[k]);
          smoothedPd->GetPoint(pts[k], nPtCoords[k]);
        }

        double oNormal[3], mNormal[3], nNormal[3];
        vtkTriangle::ComputeNormal(oPtCoords[0], oPtCoords[1], oPtCoords[2], oNormal);
        vtkTriangle::ComputeNormal(mPtCoords[0], mPtCoords[1], mPtCoords[2], mNormal);
        vtkTriangle::ComputeNormal(nPtCoords[0], nPtCoords[1], nPtCoords[2], nNormal);

        for (int k=0; k<3; k++)
        {
          oAvgVec[k] += oNormal[k];
          mAvgVec[k] += mNormal[k];
          nAvgVec[k] += nNormal[k];
        }

        //for (int k=0; k<npts; k++)
        //{
        //  if (usedPtList->IsId(pts[k]) == -1 && pts[k] != i)
        //  {
        //    usedPtList->InsertNextId(pts[k]);

        //    double origPt[3];
        //    input->GetPoint(pts[k], origPt);

        //    double modPt[3];
        //    tmp->GetPoint(pts[k], modPt);

        //    double newPt[3];
        //    smoothedPd->GetPoint(pts[k], newPt);

        //    double vec[3];
        //    for (int l=0; l<3; l++)
        //    {
        //      vec[l] = newPt[l] - (this->Alpha * origPt[l] + ((1 - this->Alpha) * modPt[l]));
        //      oAvgVec[l] += vec[l];
        //    }
        //  }
        //}

      }

      //vtkMath::MultiplyScalar(oAvgVec, -1.0/usedPtList->GetNumberOfIds());
      vtkMath::MultiplyScalar(oAvgVec, 1.0/pointCellIds->GetNumberOfIds());
      vtkMath::Normalize(oAvgVec);
      vtkMath::MultiplyScalar(mAvgVec, 1.0/pointCellIds->GetNumberOfIds());
      vtkMath::Normalize(mAvgVec);
      vtkMath::MultiplyScalar(nAvgVec, 1.0/pointCellIds->GetNumberOfIds());
      vtkMath::Normalize(nAvgVec);


      //locator->FindClosestPoint(newPt, closestPt, genericCell, closestCell, subId,
      //                          distance);

      double oPt[3];
      input->GetPoint(i, oPt);

      double mPt[3];
      tmp->GetPoint(i, mPt);

      double nPt[3];
      smoothedPd->GetPoint(i, nPt);

      double oVec[3];
      for (int j=0; j<3; j++)
        oVec[j] = nPt[j] - (this->Alpha * oPt[j] + ((1 - this->Alpha) * mPt[j]));

      double normalDot = vtkMath::Dot(nAvgVec, oVec);
      vtkMath::MultiplyScalar(nAvgVec, normalDot);

      double tangVec[3];
      vtkMath::Subtract(oVec, nAvgVec, tangVec);

      //double addVec[3];
      //for (int j=0; j<3; j++)
      //  addVec[j] = this->Beta * oVec[j] + ((1 - this->Beta) * oAvgVec[j]);

      //double finalPt[3];
      //for (int j=0; j<3; j++)
      //  finalPt[j] = nPt[j] - addVec[j];

      double finalPt[3];
      vtkMath::Add(oPt, tangVec, finalPt);

      tmp->GetPoints()->SetPoint(i, finalPt);
    }
  }

  output->DeepCopy(tmp);

  return SV_OK;
}


// ----------------------
// RunFilter
// ----------------------
int vtkSVUpdeSmoothing::RunFilter(vtkPolyData *original, vtkPolyData *output)
{
  return SV_OK;
}
