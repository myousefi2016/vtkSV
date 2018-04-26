/* Copyright (c) Stanford University, The Regents of the University of
 *               California, and others.
 *
 * All Rights Reserved.
 *
 * See Copyright-SimVascular.txt for additional details.
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
 */
/**
 * \class vtkSVBoundaryMapper
 *
 * \brief This is meant to be a parent class for any sort of boundary mapping
 * that needs to be performed parameterizations. SetBoundaries is pure virtual
 * and meant to be implemented in derived classes.
 *
 * \author Adam Updegrove
 * \author updega2@gmail.com
 * \author UC Berkeley
 * \author shaddenlab.berkeley.edu
 */

#ifndef vtkSVBoundaryMapper_h
#define vtkSVBoundaryMapper_h

#include "vtkSVParameterizationModule.h" // For exports

#include "vtkPolyDataAlgorithm.h"

#include "vtkEdgeTable.h"
#include "vtkIntArray.h"

class VTKSVPARAMETERIZATION_EXPORT vtkSVBoundaryMapper : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSVBoundaryMapper,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /// \brief Get/Set for list of point ids that are corner points
  vtkGetObjectMacro(BoundaryIds, vtkIntArray);
  vtkSetObjectMacro(BoundaryIds, vtkIntArray);
  //@}

  //@{
  /// \brief Get/Set for an edge table to aid mapper
  vtkGetObjectMacro(EdgeTable, vtkEdgeTable);
  vtkSetObjectMacro(EdgeTable, vtkEdgeTable);
  //@}

  //@{
  /// \brief Boolean array indicating boundary nodes
  vtkGetObjectMacro(IsBoundary, vtkDataArray);
  vtkSetObjectMacro(IsBoundary, vtkDataArray);
  //@}

  //@{
  /// \brief Axis of the object to use on orientation with sphee map
  vtkSetVector3Macro(ObjectXAxis, double);
  vtkSetVector3Macro(ObjectZAxis, double);
  //@}

  //@{
  // Internal ids array name, generated by GenerateIdFilter
  vtkGetStringMacro(InternalIdsArrayName);
  vtkSetStringMacro(InternalIdsArrayName);
  //@}

protected:
  vtkSVBoundaryMapper();
  ~vtkSVBoundaryMapper();

  // Usual data generation method
  int RequestData(vtkInformation *vtkNotUsed(request),
		  vtkInformationVector **inputVector,
		  vtkInformationVector *outputVector) override;

  int RemoveInternalIds;

  int PrepFilter(); // Prep work
  int RunFilter(); // Run operation
  int GetBoundaryLoop(); // Get the loop of points on the boundary
  int FindBoundaries(); // Find the boundaries of the surface
  virtual int SetBoundaries() = 0;

  char *InternalIdsArrayName;

  vtkPolyData  *InitialPd;
  vtkPolyData  *BoundaryPd;
  vtkEdgeTable *EdgeTable;
  vtkIntArray  *BoundaryIds;
  vtkDataArray *IsBoundary;

  vtkPolyData  *Boundaries;
  vtkPolyData  *BoundaryLoop;

  double ObjectXAxis[3];
  double ObjectZAxis[3];

private:
  vtkSVBoundaryMapper(const vtkSVBoundaryMapper&);
  void operator=(const vtkSVBoundaryMapper&);
};

#endif
