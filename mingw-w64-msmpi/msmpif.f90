! -*- Mode: F90; -*-
! GFortran does not support DLLIMPORT on COMMON block variables. And even if it
! did, the COMMON block variables that are exported from msmpi.dll have Intel
! Fortran layout (which would be incompatible with GFortran anyway).
!
! To still support using the sentinel variables that msmpi.dll exports in Intel
! Fortran COMMON blocks, replace them in a wrapper using sentinel variables that
! are exported from a small helper DLL.
!
!
       MODULE MPI_CONSTANTS
       IMPLICIT NONE

       INTEGER MPI_STATUS_SIZE
       PARAMETER (MPI_STATUS_SIZE=5)
       INTEGER MPI_STATUS_IGNORE(MPI_STATUS_SIZE)
       INTEGER MPI_STATUSES_IGNORE(MPI_STATUS_SIZE,1)
       INTEGER MPI_ERRCODES_IGNORE(1)
       CHARACTER*1 MPI_ARGVS_NULL(1,1)
       CHARACTER*1 MPI_ARGV_NULL(1)
       INTEGER MPI_BOTTOM, MPI_IN_PLACE
       INTEGER MPI_UNWEIGHTED, MPI_WEIGHTS_EMPTY

!GCC$ ATTRIBUTES DLLEXPORT :: MPI_BOTTOM, MPI_IN_PLACE, MPI_STATUS_IGNORE
!GCC$ ATTRIBUTES DLLEXPORT :: MPI_STATUSES_IGNORE, MPI_ERRCODES_IGNORE
!GCC$ ATTRIBUTES DLLEXPORT :: MPI_UNWEIGHTED
!GCC$ ATTRIBUTES DLLEXPORT :: MPI_WEIGHTS_EMPTY
!GCC$ ATTRIBUTES DLLEXPORT :: MPI_ARGVS_NULL, MPI_ARGV_NULL

       END MODULE MPI_CONSTANTS
