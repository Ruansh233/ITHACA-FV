/*---------------------------------------------------------------------------*\
     ██╗████████╗██╗  ██╗ █████╗  ██████╗ █████╗       ███████╗██╗   ██╗
     ██║╚══██╔══╝██║  ██║██╔══██╗██╔════╝██╔══██╗      ██╔════╝██║   ██║
     ██║   ██║   ███████║███████║██║     ███████║█████╗█████╗  ██║   ██║
     ██║   ██║   ██╔══██║██╔══██║██║     ██╔══██║╚════╝██╔══╝  ╚██╗ ██╔╝
     ██║   ██║   ██║  ██║██║  ██║╚██████╗██║  ██║      ██║      ╚████╔╝
     ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝      ╚═╝       ╚═══╝

 * In real Time Highly Advanced Computational Applications for Finite Volumes
 * Copyright (C) 2017 by the ITHACA-FV authors
-------------------------------------------------------------------------------
License
    This file is part of ITHACA-FV
    ITHACA-FV is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    ITHACA-FV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License
    along with ITHACA-FV. If not, see <http://www.gnu.org/licenses/>.
Class
    EigenFunctions
Description
    Container that contains a list of modes with additional operations
SourceFiles
    Modes.C
\*---------------------------------------------------------------------------*/

/// \file
/// Header file of the Modes class.
/// \dir
/// Directory containing the header and source files for all the containers defined in ITHACA-FV


#ifndef Modes_H
#define Modes_H
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <Eigen/Eigen>
#include <unsupported/Eigen/SparseExtra>
#include <unsupported/Eigen/CXX11/Tensor>
#pragma GCC diagnostic pop
#include "fvCFD.H"
#include "Foam2Eigen.H"
#include "ITHACAutilities.H"
#include "ITHACAstream.H"


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

/*---------------------------------------------------------------------------*\
  Class Modes Declaration
\*---------------------------------------------------------------------------*/

//--------------------------------------------------------------------------
///
/// @brief      Implementation of a container class derived from PtrList. It is meant to store POD modes.
///
/// @details This a child class of the PtrList class, it is meant to store modes and contains
/// some additional operations to project the modes and to reconstruct a solution from the coefficients.
///
/// @tparam     T    Type of the Geometric Field used inside the List, it can be vector or scalar
///
template<class Type, template<class> class PatchField, class GeoMesh>
class Modes : public PtrList<GeometricField<Type, PatchField, GeoMesh >>
{
    public:

        /// List of Matrices that contains the internalField and the additional matrices for the boundary patches.
        List<Eigen::MatrixXd> EigenModes;

        /// Number of patches
        label NBC;

        /// Method that convert a PtrList of modes into Eigen matrices filling the EigenModes object
        List<Eigen::MatrixXd> toEigen();

        //--------------------------------------------------------------------------
        /// @brief      Function that returns the Modes object as a standard PtrList
        ///
        /// @return     The PtrList containing the modes
        ///
        PtrList<GeometricField<Type, PatchField, GeoMesh >> & toPtrList()
        {
            return static_cast<PtrList<GeometricField<Type, PatchField, GeoMesh >>& >
                   (* this);
        }

        //----------------------------------------------------------------------
        /// @brief      A function that project an FvMatrix (OpenFoam linear
        ///             System) on the modes
        ///
        /// @details    Used to project an FvMatrix that is a standard linear
        ///             system of OpenFoam onto the modes defined inside the
        ///             Modes container. The output is a list of matrices that
        ///             contains in the first element the reduced matrix and in
        ///             the second element the source term.
        ///
        /// @param[in]  Af             The OpenFOAM fvMatrix linear system, it
        ///                            can be vector or scalar matrix
        /// @param[in]  numberOfModes  The number of modes used to project. If
        ///                            not given it will use all the available
        ///                            ones.
        /// @param[in]  projType       The projection type, it can be Galerkin "G" or Petrov-Galerkin "PG"
        ///
        /// @return     A list of Eigen Matrices of dimension 2. The first
        ///             element of the list is the reduced matrix of the linear
        ///             system, the second element is the reduced source term of
        ///             the linear system.
        ///
        List<Eigen::MatrixXd> project(fvMatrix<Type>& Af, label numberOfModes = 0,
                                      word projType = "G");

        //----------------------------------------------------------------------
        /// @brief      A function that project a field on the modes
        ///
        /// @details    Used to project a field onto the modes defined inside
        ///             the Modes container. The output is matrix containing the
        ///             projected field.
        ///
        /// @param[in]  field          The field to be projected, it can be
        ///                            vector or scalar.
        /// @param[in]  numberOfModes  The number of modes used to project. If
        ///                            not given it will use all the available
        ///                            ones.
        /// @param[in]  projType       The projection type, it can be Galerkin "G" or Petrov-Galerkin "PG"
        /// @param      Af             The system matrix used in case of Petrov-Galerkin projection
        ///
        /// @return     An Eigen MatrixXd.
        ///
        Eigen::MatrixXd project(GeometricField<Type, PatchField, GeoMesh>& field,
                                label numberOfModes = 0, word projType = "G", fvMatrix<Type>* Af = NULL);

        //----------------------------------------------------------------------
        /// @brief      A function that project a list of fields on the modes
        ///
        /// @details    Used to project a list of fields onto the modes defined
        ///             inside the Modes container. The output is a list of
        ///             matrices containing the projected field.
        ///
        /// @param[in]  fields         The fields list to be projected, it can
        ///                            be vector or scalar.
        /// @param[in]  numberOfModes  The number of modes used to project. If
        ///                            not given it will use all the available
        ///                            ones.
        /// @param[in]  projType       The projection type, it can be Galerkin "G", Petrov-Galerkin "PG" or "F" Frobenius
        /// @param[in]  Af             The system matrix used in case of Petrov-Galerkin projection
        ///
        /// @return     An Eigen MatricxXd list.
        ///
        Eigen::MatrixXd project(PtrList<GeometricField<Type, PatchField, GeoMesh >> &
                                fields,
                                label numberOfModes = 0, word projType = "G", fvMatrix<Type>* Af = NULL);

        //----------------------------------------------------------------------
        /// @brief      A function that project and reconstruct a snapshot on the modes
        ///
        /// @param      field          The field one wants to project
        /// @param[in]  numberOfModes  The number of modes
        /// @param[in]  projType       The projection type, it can be Galerkin "G", Petrov-Galerkin "PG" or "F" Frobenius
        /// @param      Af             The system matrix used in case of Petrov-Galerkin projection
        ///
        /// @return     The projected snapshot
        ///
        GeometricField<Type, PatchField, GeoMesh> projectSnapshot(
            GeometricField<Type, PatchField, GeoMesh>& field,
            label numberOfModes = 0, word projType = "G", fvMatrix<Type>* Af = NULL);

        //----------------------------------------------------------------------
        /// @brief      Function to reconstruct the solution starting from the
        ///             coefficients, in this case the field is passed as
        ///             input/output
        ///
        /// @param[in/out]  inputField  The input/output field
        /// @param[in]  Coeff       The coefficients of the POD expansion
        /// @param[in]  Name        The name of the field you want to return
        ///
        /// @return     A volVectorField or volScalarField given by the
        ///             reconstruction.
        ///
        GeometricField<Type, PatchField, GeoMesh> reconstruct(
            GeometricField<Type, PatchField, GeoMesh>& inputField, Eigen::MatrixXd Coeff,
            word Name);

        //----------------------------------------------------------------------
        /// @brief      Function to reconstruct a list of fields starting from a list of
        ///             coefficients, in this case the list of fields is filled by reconstructing
        ///             each field from a coefficients matrix inside the list passed
        ///
        /// @param      inputField  The input field
        /// @param[in]  Coeff       The list of coefficients of the POD expansion
        /// @param[in]  Name        The name
        ///
        /// @return     A list of volVectorFields or volScalarFields given by the
        ///             reconstruction.
        ///
        PtrList<GeometricField<Type, PatchField, GeoMesh >> reconstruct(
            GeometricField<Type, PatchField, GeoMesh>& inputField,
            List < Eigen::MatrixXd> Coeff,
            word Name);

        //----------------------------------------------------------------------
        /// @brief      Function to project a list of fields into the modes manifold
        ///
        /// @param[in]  snapshots  The list of snapshots to be projected
        /// @param[in]  numberOfModes       The number of modes you want to use
        /// @param[in]  innerProduct        The inner product to be applied
        ///
        void projectSnapshots(PtrList<GeometricField<Type, PatchField, GeoMesh >>
                              snapshots, PtrList<GeometricField<Type, PatchField, GeoMesh >>& projSnapshots,
                              label numberOfModes = 0, word innerProduct = "L2");

        //----------------------------------------------------------------------
        /// @brief      Function to project a list of fields into the modes manifold by the use of all the modes
        ///
        /// @param[in]  snapshots  The list of snapshots to be projected
        /// @param[in]  innerProduct        The inner product to be applied
        ///
        void projectSnapshots(PtrList<GeometricField<Type, PatchField, GeoMesh >>
                              snapshots, PtrList<GeometricField<Type, PatchField, GeoMesh >>& projSnapshots,
                              word innerProduct = "L2");

        //----------------------------------------------------------------------
        /// @brief      Function to project a list of fields into the modes manifold
        ///
        /// @param[in]  snapshots  The list of snapshots to be projected
        /// @param[in]  Volumes       The mesh volumes for the inner product
        /// @param[in]  numberOfModes       The number of modes you want to use
        /// @param[in]  innerProduct        The inner product to be applied
        ///
        void projectSnapshots(PtrList<GeometricField<Type, PatchField, GeoMesh >>
                              snapshots, PtrList<GeometricField<Type, PatchField, GeoMesh >>& projSnapshots,
                              PtrList<volScalarField> Volumes,
                              label numberOfModes = 0, word innerProduct = "L2");

        //----------------------------------------------------------------------
        /// @brief      Function to project a list of fields into the modes manifold by the use of all the modes
        ///
        /// @param[in]  snapshots  The list of snapshots to be projected
        /// @param[in]  Volumes       The mesh volumes for the inner product
        /// @param[in]  innerProduct        The inner product to be applied
        ///
        void projectSnapshots(PtrList<GeometricField<Type, PatchField, GeoMesh >>
                              snapshots, PtrList<GeometricField<Type, PatchField, GeoMesh >>& projSnapshots,
                              PtrList<volScalarField> Volumes,
                              word innerProduct);

        void operator=(const PtrList<GeometricField<Type, PatchField, GeoMesh >> &
                       modes);
};

typedef Modes<scalar, fvPatchField, volMesh> volScalarModes;
typedef Modes<vector, fvPatchField, volMesh> volVectorModes;
typedef Modes<scalar, fvsPatchField, surfaceMesh> surfaceScalarModes;

#endif


