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

/**
 *  \class  vtkSVGeneralCVT
 *  \brief This is class to perform edge weighted CVT clustering of an input polydata and
 *  generators
 *
 *  \author Adam Updegrove
 *  \author updega2@gmail.com
 *  \author UC Berkeley
 *  \author shaddenlab.berkeley.edu
 */

#ifndef vtkSVCenterlinesEdgeWeightedCVT_h
#define vtkSVCenterlinesEdgeWeightedCVT_h

#include "vtkSVEdgeWeightedCVT.h"
#include "vtkSVSegmentationModule.h" // For exports

#include "vtkPolyData.h"
#include "vtkSVPolyBallLine.h"

class VTKSVSEGMENTATION_EXPORT vtkSVCenterlinesEdgeWeightedCVT : public vtkSVEdgeWeightedCVT
{
public:
  static vtkSVCenterlinesEdgeWeightedCVT* New();
  vtkTypeMacro(vtkSVCenterlinesEdgeWeightedCVT,vtkSVEdgeWeightedCVT);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /// \brief Get/Set macro for array name used by the filter. Must
  //  be present on the centerlines.
  vtkSetStringMacro(CenterlineRadiusArrayName);
  vtkGetStringMacro(CenterlineRadiusArrayName);
  vtkSetStringMacro(GroupIdsArrayName);
  vtkGetStringMacro(GroupIdsArrayName);
  vtkSetStringMacro(BlankingArrayName);
  vtkGetStringMacro(BlankingArrayName);
  //@}

  //@{
  /// \brief Get/Set the radius information
  vtkSetMacro(UseRadiusInformation,int);
  vtkGetMacro(UseRadiusInformation,int);
  vtkBooleanMacro(UseRadiusInformation,int);
  //@}

  //@{
  /// \brief Get/Set the radius information
  vtkSetMacro(UseBifurcationInformation,int);
  vtkGetMacro(UseBifurcationInformation,int);
  vtkBooleanMacro(UseBifurcationInformation,int);
  //@}
  //@{

  /// \brief Get/Set the radius information
  vtkSetMacro(UseCurvatureWeight,int);
  vtkGetMacro(UseCurvatureWeight,int);
  vtkBooleanMacro(UseCurvatureWeight,int);
  //@}
protected:
  vtkSVCenterlinesEdgeWeightedCVT();
  ~vtkSVCenterlinesEdgeWeightedCVT();

  // Derived functions
  int InitializeConnectivity();
  int InitializeGenerators();
  int UpdateGenerators();
  int GetClosestGenerator(const int evalId, int &newGenerator);
  double GetEdgeWeightedDistance(const int generatorId, const int evalId);
  int FindGoodCellNeighbors(const int ptId,
                            vtkIdList *cellIds);

private:
  vtkSVCenterlinesEdgeWeightedCVT(const vtkSVCenterlinesEdgeWeightedCVT&);  // Not implemented.
  void operator=(const vtkSVCenterlinesEdgeWeightedCVT&);  // Not implemented.

  vtkSVPolyBallLine *DistanceFunction;

  std::vector<std::vector<int> > IsGoodNeighborCell;

  int UseRadiusInformation;
  int UseBifurcationInformation;
  int UseCurvatureWeight;

  char *GroupIdsArrayName;
  char *BlankingArrayName;
  char *CenterlineRadiusArrayName;

};

#endif